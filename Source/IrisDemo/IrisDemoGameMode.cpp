// Copyright Epic Games, Inc. All Rights Reserved.

#include "IrisDemoGameMode.h"

#include "GameFramework/PlayerController.h"
#include "Iris/ReplicationSystem/Filtering/NetObjectFilter.h"
#include "Iris/ReplicationSystem/NetObjectGroupHandle.h"
#include "Iris/ReplicationSystem/ObjectReplicationBridge.h"
#include "Iris/ReplicationSystem/ReplicationSystem.h"
#include "IrisDemo.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Net/Iris/ReplicationSystem/ReplicationSystemUtil.h"
#include "Engine/NetConnection.h"
#include "ScenarioA/RelayDroneActor.h"
#include "ScenarioA/RelayOperationalSummaryActor.h"
#include "ScenarioA/RelayPlayerState.h"
#include "ScenarioA/RelaySensorActor.h"
#include "ScenarioA/RelaySupplyCrateActor.h"
#include "TimerManager.h"

namespace
{
constexpr int32 ScenarioAZoneCount = 3;
const TCHAR* ScenarioAGameModeConfigSection = TEXT("/Script/IrisDemo.IrisDemoGameMode");
const TCHAR* ScenarioAEnableRoleFilteringConfigKey = TEXT("bScenarioAEnableRoleFiltering");

enum class EScenarioAOptionSource : uint8
{
	None,
	UrlOptions,
	CommandLine
};

const TCHAR* GetScenarioAOptionSourceLabel(EScenarioAOptionSource Source)
{
	switch (Source)
	{
	case EScenarioAOptionSource::UrlOptions:
		return TEXT("UrlOptions");
	case EScenarioAOptionSource::CommandLine:
		return TEXT("CommandLine");
	case EScenarioAOptionSource::None:
	default:
		return TEXT("Default");
	}
}

bool TryReadScenarioAUrlOption(const FString& Options, const TCHAR* OptionName, FString& OutValue)
{
	const FString OptionKey(OptionName);
	TArray<FString> OptionTokens;
	Options.ParseIntoArray(OptionTokens, TEXT("?"), true);

	for (const FString& OptionToken : OptionTokens)
	{
		FString Key;
		FString Value;
		if (OptionToken.Split(TEXT("="), &Key, &Value) && Key == OptionKey)
		{
			OutValue = Value;
			return true;
		}
	}

	return false;
}

bool TryReadScenarioAOptionValue(const FString& Options, const TCHAR* OptionName, FString& OutValue, EScenarioAOptionSource& OutSource)
{
	if (TryReadScenarioAUrlOption(Options, OptionName, OutValue))
	{
		OutSource = EScenarioAOptionSource::UrlOptions;
		return true;
	}

	const FString CommandLineOption = FString::Printf(TEXT("-%s="), OptionName);
	if (FParse::Value(FCommandLine::Get(), *CommandLineOption, OutValue))
	{
		OutSource = EScenarioAOptionSource::CommandLine;
		return true;
	}

	OutSource = EScenarioAOptionSource::None;
	return false;
}

bool TryReadScenarioAIntOption(const FString& Options, const TCHAR* OptionName, int32& OutValue, EScenarioAOptionSource& OutSource)
{
	FString RawValue;
	if (!TryReadScenarioAOptionValue(Options, OptionName, RawValue, OutSource))
	{
		return false;
	}

	OutValue = FCString::Atoi(*RawValue);
	return true;
}

bool TryReadScenarioAFloatOption(const FString& Options, const TCHAR* OptionName, float& OutValue, EScenarioAOptionSource& OutSource)
{
	FString RawValue;
	if (!TryReadScenarioAOptionValue(Options, OptionName, RawValue, OutSource))
	{
		return false;
	}

	OutValue = FCString::Atof(*RawValue);
	return true;
}

bool TryReadScenarioABoolOption(const FString& Options, const TCHAR* OptionName, bool& OutValue, EScenarioAOptionSource& OutSource)
{
	FString RawValue;
	if (!TryReadScenarioAOptionValue(Options, OptionName, RawValue, OutSource))
	{
		return false;
	}

	OutValue = RawValue.Equals(TEXT("1")) || RawValue.Equals(TEXT("true"), ESearchCase::IgnoreCase) || RawValue.Equals(TEXT("yes"), ESearchCase::IgnoreCase);
	return true;
}

bool TryReadScenarioABoolConfig(const TCHAR* SectionName, const TCHAR* OptionName, bool& OutValue)
{
	return GConfig && GConfig->GetBool(SectionName, OptionName, OutValue, GGameIni);
}

int32 NormalizeScenarioAZoneId(int32 ZoneId)
{
	return FMath::Clamp(ZoneId, 0, ScenarioAZoneCount - 1);
}

FName GetScenarioADetailFilterGroupName(int32 ZoneId)
{
	return FName(*FString::Printf(TEXT("ScenarioA_Detail_Zone_%d"), ZoneId));
}

FName GetScenarioASummaryFilterGroupName()
{
	return FName(TEXT("ScenarioA_OperationalSummary"));
}

UE::Net::FNetObjectGroupHandle GetOrCreateScenarioAExclusionGroup(UReplicationSystem& ReplicationSystem, FName GroupName)
{
	UE::Net::FNetObjectGroupHandle GroupHandle = ReplicationSystem.FindGroup(GroupName);
	if (ReplicationSystem.IsValidGroup(GroupHandle))
	{
		return GroupHandle;
	}

	GroupHandle = ReplicationSystem.CreateGroup(GroupName);
	if (!ReplicationSystem.IsValidGroup(GroupHandle))
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A filtering group creation failed: Group=%s"), *GroupName.ToString());
		return UE::Net::FNetObjectGroupHandle::GetInvalid();
	}

	if (!ReplicationSystem.AddExclusionFilterGroup(GroupHandle))
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A filtering group registration failed: Group=%s"), *GroupName.ToString());
		return UE::Net::FNetObjectGroupHandle::GetInvalid();
	}

	ReplicationSystem.SetGroupFilterStatus(GroupHandle, UE::Net::ENetFilterStatus::Disallow);
	return GroupHandle;
}

void AddActorToScenarioAFilterGroup(UReplicationSystem& ReplicationSystem, UObjectReplicationBridge& ReplicationBridge,
	const AActor* Actor, UE::Net::FNetObjectGroupHandle GroupHandle, FName GroupName)
{
	if (!Actor || !ReplicationSystem.IsValidGroup(GroupHandle))
	{
		return;
	}

	const UE::Net::FNetRefHandle RefHandle = ReplicationBridge.GetReplicatedRefHandle(Actor);
	if (!RefHandle.IsValid())
	{
		UE_LOG(LogIrisDemo, Verbose, TEXT("Scenario A filtering group pending handle: Actor=%s Group=%s"), *GetNameSafe(Actor), *GroupName.ToString());
		return;
	}

	ReplicationSystem.AddToGroup(GroupHandle, RefHandle);
}

bool ShouldScenarioAConnectionReceiveDetailZone(ERelayOperatorRole Role, int32 AssignedZoneId, int32 ActorZoneId)
{
	switch (Role)
	{
	case ERelayOperatorRole::Commander:
		return true;
	case ERelayOperatorRole::FieldAgent:
		return AssignedZoneId == ActorZoneId;
	case ERelayOperatorRole::Spectator:
	default:
		return false;
	}
}

bool ShouldScenarioAConnectionReceiveSummary(ERelayOperatorRole Role)
{
	return Role == ERelayOperatorRole::Commander || Role == ERelayOperatorRole::Spectator;
}

FString EscapeScenarioAJsonString(const FString& Value)
{
	FString Escaped;
	Escaped.Reserve(Value.Len());

	for (int32 Index = 0; Index < Value.Len(); ++Index)
	{
		const TCHAR Character = Value[Index];
		switch (Character)
		{
		case TCHAR('\\'):
			Escaped += TEXT("\\\\");
			break;
		case TCHAR('"'):
			Escaped += TEXT("\\\"");
			break;
		case TCHAR('\n'):
			Escaped += TEXT("\\n");
			break;
		case TCHAR('\r'):
			Escaped += TEXT("\\r");
			break;
		case TCHAR('\t'):
			Escaped += TEXT("\\t");
			break;
		default:
			Escaped.AppendChar(Character);
			break;
		}
	}

	return Escaped;
}

FString QuoteScenarioAJsonString(const FString& Value)
{
	return FString::Printf(TEXT("\"%s\""), *EscapeScenarioAJsonString(Value));
}

FString GetScenarioANetModeLabel(const UWorld* World)
{
	if (!World)
	{
		return TEXT("Unknown");
	}

	switch (World->GetNetMode())
	{
	case NM_Standalone:
		return TEXT("Standalone");
	case NM_DedicatedServer:
		return TEXT("DedicatedServer");
	case NM_ListenServer:
		return TEXT("ListenServer");
	case NM_Client:
		return TEXT("Client");
	default:
		return TEXT("Unknown");
	}
}
}

AIrisDemoGameMode::AIrisDemoGameMode()
{
	EnsureScenarioAPlayerStateClass();
	ScenarioASensorClass = ARelaySensorActor::StaticClass();
	ScenarioADroneClass = ARelayDroneActor::StaticClass();
	ScenarioASupplyCrateClass = ARelaySupplyCrateActor::StaticClass();
	ScenarioAOperationalSummaryClass = ARelayOperationalSummaryActor::StaticClass();
}

void AIrisDemoGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	EnsureScenarioAPlayerStateClass();
	ApplyScenarioAOptions(Options);

	Super::InitGame(MapName, Options, ErrorMessage);
}

void AIrisDemoGameMode::EnsureScenarioAPlayerStateClass()
{
	if (!PlayerStateClass || !PlayerStateClass->IsChildOf(ARelayPlayerState::StaticClass()))
	{
		PlayerStateClass = ARelayPlayerState::StaticClass();
	}
}

void AIrisDemoGameMode::ApplyScenarioAOptions(const FString& Options)
{
	ApplyIrisReplicationCommandLineOverride();

	EScenarioAOptionSource SeedSource = EScenarioAOptionSource::None;
	EScenarioAOptionSource SensorCountSource = EScenarioAOptionSource::None;
	EScenarioAOptionSource DroneCountSource = EScenarioAOptionSource::None;
	EScenarioAOptionSource SupplyCrateCountSource = EScenarioAOptionSource::None;
	EScenarioAOptionSource UpdateIntervalSource = EScenarioAOptionSource::None;
	EScenarioAOptionSource RunDurationSource = EScenarioAOptionSource::None;
	EScenarioAOptionSource RunIdSource = EScenarioAOptionSource::None;
	EScenarioAOptionSource RoleFilteringRuntimeSource = EScenarioAOptionSource::None;

	FString RoleFilteringSource = TEXT("Default");
	if (TryReadScenarioABoolConfig(ScenarioAGameModeConfigSection, ScenarioAEnableRoleFilteringConfigKey, bScenarioAEnableRoleFiltering))
	{
		RoleFilteringSource = TEXT("DefaultGame.ini");
	}

	TryReadScenarioAIntOption(Options, TEXT("ScenarioASeed"), ScenarioASeed, SeedSource);
	TryReadScenarioAIntOption(Options, TEXT("ScenarioASensorCount"), ScenarioASensorCount, SensorCountSource);
	TryReadScenarioAIntOption(Options, TEXT("ScenarioADroneCount"), ScenarioADroneCount, DroneCountSource);
	TryReadScenarioAIntOption(Options, TEXT("ScenarioASupplyCrateCount"), ScenarioASupplyCrateCount, SupplyCrateCountSource);
	TryReadScenarioAFloatOption(Options, TEXT("ScenarioAUpdateInterval"), ScenarioASensorUpdateInterval, UpdateIntervalSource);
	TryReadScenarioAFloatOption(Options, TEXT("ScenarioARunDuration"), ScenarioARunDuration, RunDurationSource);

	FString RawRunId;
	if (TryReadScenarioAOptionValue(Options, TEXT("ScenarioARunId"), RawRunId, RunIdSource))
	{
		ScenarioARunId = MakeScenarioARunId(RawRunId);
		ScenarioARunIdSource = GetScenarioAOptionSourceLabel(RunIdSource);
	}
	else
	{
		ScenarioARunId = GetScenarioARunId();
		ScenarioARunIdSource = TEXT("Generated");
	}

	if (TryReadScenarioABoolOption(Options, TEXT("ScenarioAEnableRoleFiltering"), bScenarioAEnableRoleFiltering, RoleFilteringRuntimeSource))
	{
		RoleFilteringSource = GetScenarioAOptionSourceLabel(RoleFilteringRuntimeSource);
	}
	ScenarioARoleFilteringSource = RoleFilteringSource;

	ScenarioASeed = FMath::Max(0, ScenarioASeed);
	ScenarioASensorCount = FMath::Max(0, ScenarioASensorCount);
	ScenarioADroneCount = FMath::Max(0, ScenarioADroneCount);
	ScenarioASupplyCrateCount = FMath::Max(0, ScenarioASupplyCrateCount);
	ScenarioASensorUpdateInterval = FMath::Max(0.1f, ScenarioASensorUpdateInterval);
	ScenarioARunDuration = FMath::Max(0.0f, ScenarioARunDuration);

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A options applied: UrlOptions=\"%s\" CommandLine=\"%s\" IrisCmdline=%s IrisMode=%s RunId=%s RunIdSource=%s RoleFiltering=%s RoleFilteringSource=%s Sources=[Seed:%s SensorCount:%s DroneCount:%s SupplyCrateCount:%s UpdateInterval:%s RunDuration:%s]"),
		*Options,
		FCommandLine::Get(),
		*GetIrisReplicationCommandLineOverrideLabel(),
		*GetIrisReplicationModeLabel(GetWorld()),
		*ScenarioARunId,
		*ScenarioARunIdSource,
		bScenarioAEnableRoleFiltering ? TEXT("Enabled") : TEXT("Disabled"),
		*RoleFilteringSource,
		GetScenarioAOptionSourceLabel(SeedSource),
		GetScenarioAOptionSourceLabel(SensorCountSource),
		GetScenarioAOptionSourceLabel(DroneCountSource),
		GetScenarioAOptionSourceLabel(SupplyCrateCountSource),
		GetScenarioAOptionSourceLabel(UpdateIntervalSource),
		GetScenarioAOptionSourceLabel(RunDurationSource));
}

void AIrisDemoGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnScenarioASensors();
	SpawnScenarioADrones();
	SpawnScenarioASupplyCrates();
	SpawnScenarioAOperationalSummary();
	LogScenarioABaselineConfig();
	WriteScenarioARunMetadata();
	QueueScenarioAFilterRefresh();

	if ((ScenarioASensors.Num() > 0 || ScenarioADrones.Num() > 0 || ScenarioASupplyCrates.Num() > 0) && ScenarioASensorUpdateInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			ScenarioASensorUpdateTimerHandle,
			this,
			&AIrisDemoGameMode::UpdateScenarioASensors,
			ScenarioASensorUpdateInterval,
			true);
	}

	if (ScenarioARunDuration > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			ScenarioABaselineCompleteTimerHandle,
			this,
			&AIrisDemoGameMode::LogScenarioABaselineComplete,
			ScenarioARunDuration,
			false);
	}
}

void AIrisDemoGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ScenarioASensorUpdateTimerHandle);
	GetWorldTimerManager().ClearTimer(ScenarioABaselineCompleteTimerHandle);
	GetWorldTimerManager().ClearTimer(ScenarioAFilterRefreshTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AIrisDemoGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	if (ARelayPlayerState* RelayPlayerState = NewPlayer->GetPlayerState<ARelayPlayerState>())
	{
		const ERelayOperatorRole AssignedRole = GetNextScenarioARole();
		RelayPlayerState->SetOperatorRole(AssignedRole);
		RelayPlayerState->SetAssignedZoneId(GetScenarioAZoneForRole(AssignedRole));
		QueueScenarioAFilterRefresh();
		return;
	}

	UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A role assignment skipped: PlayerStateClass=%s Player=%s"),
		PlayerStateClass ? *PlayerStateClass->GetName() : TEXT("None"),
		*NewPlayer->GetName());
}

ERelayOperatorRole AIrisDemoGameMode::GetNextScenarioARole()
{
	static constexpr ERelayOperatorRole RoleOrder[] = {
		ERelayOperatorRole::Commander,
		ERelayOperatorRole::FieldAgent,
		ERelayOperatorRole::Spectator
	};

	const ERelayOperatorRole AssignedRole = RoleOrder[NextScenarioARoleIndex % UE_ARRAY_COUNT(RoleOrder)];
	++NextScenarioARoleIndex;

	return AssignedRole;
}

int32 AIrisDemoGameMode::GetScenarioAZoneForRole(ERelayOperatorRole OperatorRole)
{
	if (OperatorRole != ERelayOperatorRole::FieldAgent)
	{
		return INDEX_NONE;
	}

	const int32 AssignedZoneId = NextScenarioAFieldAgentZoneId % ScenarioAZoneCount;
	++NextScenarioAFieldAgentZoneId;
	return AssignedZoneId;
}

void AIrisDemoGameMode::SpawnScenarioASensors()
{
	if (!HasAuthority() || !ScenarioASensorClass || ScenarioASensorCount <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ScenarioASensors.Reset();

	for (int32 SensorIndex = 0; SensorIndex < ScenarioASensorCount; ++SensorIndex)
	{
		const int32 ZoneId = SensorIndex % 3;
		const FVector SpawnLocation(
			static_cast<double>(ZoneId) * 450.0,
			static_cast<double>(SensorIndex / 3) * 350.0,
			120.0);
		const FRotator SpawnRotation = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ARelaySensorActor* Sensor = World->SpawnActor<ARelaySensorActor>(
			ScenarioASensorClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParameters);

		if (!Sensor)
		{
			UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A sensor spawn failed: Index=%d Zone=%d"), SensorIndex, ZoneId);
			continue;
		}

		const int32 InitialAlertLevel = (ScenarioASeed + SensorIndex + ZoneId) % 5;
		Sensor->ConfigureSensor(SensorIndex, ZoneId, FName(*FString::Printf(TEXT("Sensor_%02d"), SensorIndex)), true);
		Sensor->SetSensorState(InitialAlertLevel, InitialAlertLevel >= 3);
		ScenarioASensors.Add(Sensor);
	}

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A sensors spawned: Count=%d"), ScenarioASensors.Num());
}

void AIrisDemoGameMode::UpdateScenarioASensors()
{
	if (!HasAuthority())
	{
		return;
	}

	for (int32 SensorIndex = 0; SensorIndex < ScenarioASensors.Num(); ++SensorIndex)
	{
		ARelaySensorActor* Sensor = ScenarioASensors[SensorIndex];
		if (!IsValid(Sensor) || !Sensor->IsScenarioAEnabled())
		{
			continue;
		}

		const int32 NextAlertLevel = (Sensor->GetAlertLevel() + 1 + SensorIndex) % 5;
		const bool bNextTriggered = NextAlertLevel >= 3;
		Sensor->SetSensorState(NextAlertLevel, bNextTriggered);
	}

	UpdateScenarioADrones();
	UpdateScenarioASupplyCrates();
	UpdateScenarioAOperationalSummary();
}

void AIrisDemoGameMode::SpawnScenarioADrones()
{
	if (!HasAuthority() || !ScenarioADroneClass || ScenarioADroneCount <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ScenarioADrones.Reset();

	for (int32 DroneIndex = 0; DroneIndex < ScenarioADroneCount; ++DroneIndex)
	{
		const int32 ZoneId = DroneIndex % 3;
		const FVector SpawnLocation(
			static_cast<double>(ZoneId) * 450.0,
			-300.0,
			260.0 + static_cast<double>(DroneIndex) * 30.0);
		const FRotator SpawnRotation(0.0, 45.0 * static_cast<double>(DroneIndex), 0.0);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ARelayDroneActor* Drone = World->SpawnActor<ARelayDroneActor>(
			ScenarioADroneClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParameters);

		if (!Drone)
		{
			UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A drone spawn failed: Index=%d Zone=%d"), DroneIndex, ZoneId);
			continue;
		}

		const int32 InitialBattery = FMath::Clamp(100 - DroneIndex * 15 - (ScenarioASeed % 8), 1, 100);
		Drone->ConfigureDrone(DroneIndex, ZoneId, FName(*FString::Printf(TEXT("Drone_%02d"), DroneIndex)), true);
		Drone->SetDroneState(InitialBattery, EDroneRelayState::Patrol);
		ScenarioADrones.Add(Drone);
	}

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A drones spawned: Count=%d"), ScenarioADrones.Num());
}

void AIrisDemoGameMode::UpdateScenarioADrones()
{
	if (!HasAuthority())
	{
		return;
	}

	for (int32 DroneIndex = 0; DroneIndex < ScenarioADrones.Num(); ++DroneIndex)
	{
		ARelayDroneActor* Drone = ScenarioADrones[DroneIndex];
		if (!IsValid(Drone) || !Drone->IsScenarioAEnabled())
		{
			continue;
		}

		const int32 NextBattery = (Drone->GetBatteryPercent() <= 10) ? 100 : Drone->GetBatteryPercent() - (5 + DroneIndex);
		EDroneRelayState NextState = EDroneRelayState::Patrol;
		if (NextBattery <= 15)
		{
			NextState = EDroneRelayState::Disabled;
		}
		else if (NextBattery <= 35)
		{
			NextState = EDroneRelayState::Returning;
		}
		else if ((DroneIndex + NextBattery) % 3 == 0)
		{
			NextState = EDroneRelayState::Investigating;
		}

		Drone->SetDroneState(NextBattery, NextState);
	}
}

void AIrisDemoGameMode::SpawnScenarioASupplyCrates()
{
	if (!HasAuthority() || !ScenarioASupplyCrateClass || ScenarioASupplyCrateCount <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ScenarioASupplyCrates.Reset();

	for (int32 CrateIndex = 0; CrateIndex < ScenarioASupplyCrateCount; ++CrateIndex)
	{
		const int32 ZoneId = CrateIndex % 3;
		const int32 OwningSquadId = CrateIndex % 2;
		const FVector SpawnLocation(
			static_cast<double>(ZoneId) * 450.0,
			650.0,
			90.0);
		const FRotator SpawnRotation(0.0, 30.0 * static_cast<double>(CrateIndex), 0.0);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ARelaySupplyCrateActor* SupplyCrate = World->SpawnActor<ARelaySupplyCrateActor>(
			ScenarioASupplyCrateClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParameters);

		if (!SupplyCrate)
		{
			UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A supply crate spawn failed: Index=%d Zone=%d Squad=%d"), CrateIndex, ZoneId, OwningSquadId);
			continue;
		}

		const int32 InitialStockCount = 12 + CrateIndex * 4 + (ScenarioASeed % 3);
		const bool bInitialReserved = ((ScenarioASeed + CrateIndex) % 2) == 0;
		SupplyCrate->ConfigureCrate(CrateIndex, ZoneId, OwningSquadId, FName(*FString::Printf(TEXT("Crate_%02d"), CrateIndex)), true);
		SupplyCrate->SetCrateState(InitialStockCount, bInitialReserved);
		ScenarioASupplyCrates.Add(SupplyCrate);
	}

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A supply crates spawned: Count=%d"), ScenarioASupplyCrates.Num());
}

void AIrisDemoGameMode::UpdateScenarioASupplyCrates()
{
	if (!HasAuthority())
	{
		return;
	}

	for (int32 CrateIndex = 0; CrateIndex < ScenarioASupplyCrates.Num(); ++CrateIndex)
	{
		ARelaySupplyCrateActor* SupplyCrate = ScenarioASupplyCrates[CrateIndex];
		if (!IsValid(SupplyCrate) || !SupplyCrate->IsScenarioAEnabled())
		{
			continue;
		}

		const int32 NextStockCount = (SupplyCrate->GetStockCount() <= 2) ? 16 + CrateIndex : SupplyCrate->GetStockCount() - (1 + CrateIndex);
		const bool bNextReserved = ((SupplyCrate->GetLastUpdateSequence() + CrateIndex) % 2) == 0;
		SupplyCrate->SetCrateState(NextStockCount, bNextReserved);
	}
}

void AIrisDemoGameMode::SpawnScenarioAOperationalSummary()
{
	if (!HasAuthority() || !ScenarioAOperationalSummaryClass)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.Owner = this;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	ScenarioAOperationalSummary = World->SpawnActor<ARelayOperationalSummaryActor>(
		ScenarioAOperationalSummaryClass,
		FVector(450.0, 1050.0, 150.0),
		FRotator::ZeroRotator,
		SpawnParameters);

	if (!ScenarioAOperationalSummary)
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A operational summary spawn failed"));
		return;
	}

	ScenarioAOperationalSummary->ConfigureSummary(0, FName(TEXT("OperationalSummary")), true);
	UpdateScenarioAOperationalSummary();

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A operational summary spawned: Count=1"));
}

void AIrisDemoGameMode::UpdateScenarioAOperationalSummary()
{
	if (!HasAuthority() || !IsValid(ScenarioAOperationalSummary) || !ScenarioAOperationalSummary->IsScenarioAEnabled())
	{
		return;
	}

	int32 KnownAlertCount = 0;
	for (const ARelaySensorActor* Sensor : ScenarioASensors)
	{
		if (IsValid(Sensor) && Sensor->IsTriggered())
		{
			++KnownAlertCount;
		}
	}

	int32 KnownDroneCount = 0;
	for (const ARelayDroneActor* Drone : ScenarioADrones)
	{
		if (IsValid(Drone) && Drone->IsScenarioAEnabled())
		{
			++KnownDroneCount;
		}
	}

	int32 KnownSupplyCount = 0;
	for (const ARelaySupplyCrateActor* SupplyCrate : ScenarioASupplyCrates)
	{
		if (IsValid(SupplyCrate) && SupplyCrate->IsScenarioAEnabled() && SupplyCrate->GetStockCount() > 0)
		{
			++KnownSupplyCount;
		}
	}

	ScenarioAOperationalSummary->SetSummaryState(KnownAlertCount, KnownDroneCount, KnownSupplyCount);
}

void AIrisDemoGameMode::QueueScenarioAFilterRefresh()
{
	if (!HasAuthority() || !bScenarioAEnableRoleFiltering)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ApplyScenarioARoleBasedFiltering();

	World->GetTimerManager().SetTimer(
		ScenarioAFilterRefreshTimerHandle,
		this,
		&AIrisDemoGameMode::ApplyScenarioARoleBasedFiltering,
		0.2f,
		false);
}

void AIrisDemoGameMode::ApplyScenarioARoleBasedFiltering()
{
	if (!HasAuthority() || !bScenarioAEnableRoleFiltering)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (!IsUsingIrisReplication(World))
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A role filtering skipped: Mode=%s Reason=IrisDisabled"),
			*GetIrisReplicationModeLabel(World));
		return;
	}

	UReplicationSystem* ReplicationSystem = UE::Net::FReplicationSystemUtil::GetReplicationSystem(World);
	if (!ReplicationSystem)
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A role filtering skipped: Mode=%s Reason=NoReplicationSystem"),
			*GetIrisReplicationModeLabel(World));
		return;
	}

	UObjectReplicationBridge* ReplicationBridge = ReplicationSystem->GetReplicationBridge();
	if (!ReplicationBridge)
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A role filtering skipped: Mode=%s Reason=NoReplicationBridge"),
			*GetIrisReplicationModeLabel(World));
		return;
	}

	UE::Net::FNetObjectGroupHandle DetailZoneGroups[ScenarioAZoneCount];
	for (int32 ZoneId = 0; ZoneId < ScenarioAZoneCount; ++ZoneId)
	{
		DetailZoneGroups[ZoneId] = GetOrCreateScenarioAExclusionGroup(*ReplicationSystem, GetScenarioADetailFilterGroupName(ZoneId));
	}

	const UE::Net::FNetObjectGroupHandle SummaryGroup = GetOrCreateScenarioAExclusionGroup(*ReplicationSystem, GetScenarioASummaryFilterGroupName());

	for (const ARelaySensorActor* Sensor : ScenarioASensors)
	{
		if (IsValid(Sensor))
		{
			const int32 ZoneId = NormalizeScenarioAZoneId(Sensor->GetZoneId());
			AddActorToScenarioAFilterGroup(*ReplicationSystem, *ReplicationBridge, Sensor, DetailZoneGroups[ZoneId], GetScenarioADetailFilterGroupName(ZoneId));
		}
	}

	for (const ARelayDroneActor* Drone : ScenarioADrones)
	{
		if (IsValid(Drone))
		{
			const int32 ZoneId = NormalizeScenarioAZoneId(Drone->GetZoneId());
			AddActorToScenarioAFilterGroup(*ReplicationSystem, *ReplicationBridge, Drone, DetailZoneGroups[ZoneId], GetScenarioADetailFilterGroupName(ZoneId));
		}
	}

	for (const ARelaySupplyCrateActor* SupplyCrate : ScenarioASupplyCrates)
	{
		if (IsValid(SupplyCrate))
		{
			const int32 ZoneId = NormalizeScenarioAZoneId(SupplyCrate->GetZoneId());
			AddActorToScenarioAFilterGroup(*ReplicationSystem, *ReplicationBridge, SupplyCrate, DetailZoneGroups[ZoneId], GetScenarioADetailFilterGroupName(ZoneId));
		}
	}

	AddActorToScenarioAFilterGroup(*ReplicationSystem, *ReplicationBridge, ScenarioAOperationalSummary, SummaryGroup, GetScenarioASummaryFilterGroupName());

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PlayerController = It->Get();
		if (!PlayerController)
		{
			continue;
		}

		const UNetConnection* NetConnection = PlayerController->GetNetConnection();
		if (!NetConnection || !NetConnection->GetConnectionHandle().IsValid())
		{
			UE_LOG(LogIrisDemo, Verbose, TEXT("Scenario A role filtering skipped local controller: Controller=%s"),
				*GetNameSafe(PlayerController));
			continue;
		}

		const ARelayPlayerState* RelayPlayerState = PlayerController->GetPlayerState<ARelayPlayerState>();
		const ERelayOperatorRole OperatorRole = RelayPlayerState ? RelayPlayerState->GetOperatorRole() : ERelayOperatorRole::Unassigned;
		const int32 AssignedZoneId = RelayPlayerState ? RelayPlayerState->GetAssignedZoneId() : INDEX_NONE;
		const uint32 ConnectionId = NetConnection->GetConnectionHandle().GetParentConnectionId();

		for (int32 ZoneId = 0; ZoneId < ScenarioAZoneCount; ++ZoneId)
		{
			if (!ReplicationSystem->IsValidGroup(DetailZoneGroups[ZoneId]))
			{
				continue;
			}

			const UE::Net::ENetFilterStatus DetailStatus = ShouldScenarioAConnectionReceiveDetailZone(OperatorRole, AssignedZoneId, ZoneId)
				? UE::Net::ENetFilterStatus::Allow
				: UE::Net::ENetFilterStatus::Disallow;
			ReplicationSystem->SetGroupFilterStatus(DetailZoneGroups[ZoneId], ConnectionId, DetailStatus);
		}

		if (ReplicationSystem->IsValidGroup(SummaryGroup))
		{
			const UE::Net::ENetFilterStatus SummaryStatus = ShouldScenarioAConnectionReceiveSummary(OperatorRole)
				? UE::Net::ENetFilterStatus::Allow
				: UE::Net::ENetFilterStatus::Disallow;
			ReplicationSystem->SetGroupFilterStatus(SummaryGroup, ConnectionId, SummaryStatus);
		}

		UE_LOG(LogIrisDemo, Log, TEXT("Scenario A role filtering applied: Mode=%s ConnectionId=%u Controller=%s Role=%s AssignedZone=%d Summary=%s DetailZones=[Z0:%s Z1:%s Z2:%s]"),
			*GetIrisReplicationModeLabel(World),
			ConnectionId,
			*GetNameSafe(PlayerController),
			RelayPlayerState ? *RelayPlayerState->GetOperatorRoleName() : TEXT("Unassigned"),
			AssignedZoneId,
			ShouldScenarioAConnectionReceiveSummary(OperatorRole) ? TEXT("Allow") : TEXT("Disallow"),
			ShouldScenarioAConnectionReceiveDetailZone(OperatorRole, AssignedZoneId, 0) ? TEXT("Allow") : TEXT("Disallow"),
			ShouldScenarioAConnectionReceiveDetailZone(OperatorRole, AssignedZoneId, 1) ? TEXT("Allow") : TEXT("Disallow"),
			ShouldScenarioAConnectionReceiveDetailZone(OperatorRole, AssignedZoneId, 2) ? TEXT("Allow") : TEXT("Disallow"));
	}
}

void AIrisDemoGameMode::LogScenarioABaselineConfig() const
{
	const int32 DetailActorTotal = ScenarioASensors.Num() + ScenarioADrones.Num() + ScenarioASupplyCrates.Num();
	const int32 SummaryActorTotal = IsValid(ScenarioAOperationalSummary) ? 1 : 0;
	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A baseline config: Mode=%s Seed=%d SensorCount=%d DroneCount=%d SupplyCrateCount=%d SummaryCount=%d DetailActorTotal=%d UpdateInterval=%.2f RunDuration=%.2f RoleFiltering=%s ExpectedPreFilterClientDetailActors=%d"),
		*GetIrisReplicationModeLabel(GetWorld()),
		ScenarioASeed,
		ScenarioASensors.Num(),
		ScenarioADrones.Num(),
		ScenarioASupplyCrates.Num(),
		SummaryActorTotal,
		DetailActorTotal,
		ScenarioASensorUpdateInterval,
		ScenarioARunDuration,
		bScenarioAEnableRoleFiltering ? TEXT("Enabled") : TEXT("Disabled"),
		DetailActorTotal);
}

void AIrisDemoGameMode::LogScenarioABaselineComplete() const
{
	const int32 DetailActorTotal = ScenarioASensors.Num() + ScenarioADrones.Num() + ScenarioASupplyCrates.Num();
	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A baseline server window complete: Mode=%s Seed=%d Duration=%.2f DetailActorTotal=%d"),
		*GetIrisReplicationModeLabel(GetWorld()),
		ScenarioASeed,
		ScenarioARunDuration,
		DetailActorTotal);
}

void AIrisDemoGameMode::WriteScenarioARunMetadata() const
{
	const UWorld* World = GetWorld();
	const int32 SummaryActorTotal = IsValid(ScenarioAOperationalSummary) ? 1 : 0;
	const int32 DetailActorTotal = ScenarioASensors.Num() + ScenarioADrones.Num() + ScenarioASupplyCrates.Num();
	const FString RunId = MakeScenarioARunId(ScenarioARunId);
	const FString RunDirectory = GetScenarioARunDirectory(RunId);

	IFileManager::Get().MakeDirectory(*RunDirectory, true);

	const FString Json = FString::Printf(TEXT(
		"{\n"
		"  \"schemaVersion\": 1,\n"
		"  \"runId\": %s,\n"
		"  \"timestamp\": %s,\n"
		"  \"map\": %s,\n"
		"  \"serverType\": %s,\n"
		"  \"mode\": %s,\n"
		"  \"irisCommandLineOverride\": %s,\n"
		"  \"roleFiltering\": %s,\n"
		"  \"roleFilteringSource\": %s,\n"
		"  \"seed\": %d,\n"
		"  \"sensorCount\": %d,\n"
		"  \"droneCount\": %d,\n"
		"  \"supplyCrateCount\": %d,\n"
		"  \"summaryCount\": %d,\n"
		"  \"detailActorTotal\": %d,\n"
		"  \"updateInterval\": %.3f,\n"
		"  \"runDuration\": %.3f,\n"
		"  \"runIdSource\": %s,\n"
		"  \"commandLine\": %s\n"
		"}\n"),
		*QuoteScenarioAJsonString(RunId),
		*QuoteScenarioAJsonString(FDateTime::Now().ToIso8601()),
		*QuoteScenarioAJsonString(World ? World->GetMapName() : TEXT("Unknown")),
		*QuoteScenarioAJsonString(GetScenarioANetModeLabel(World)),
		*QuoteScenarioAJsonString(GetIrisReplicationModeLabel(World)),
		*QuoteScenarioAJsonString(GetIrisReplicationCommandLineOverrideLabel()),
		bScenarioAEnableRoleFiltering ? TEXT("true") : TEXT("false"),
		*QuoteScenarioAJsonString(ScenarioARoleFilteringSource),
		ScenarioASeed,
		ScenarioASensors.Num(),
		ScenarioADrones.Num(),
		ScenarioASupplyCrates.Num(),
		SummaryActorTotal,
		DetailActorTotal,
		ScenarioASensorUpdateInterval,
		ScenarioARunDuration,
		*QuoteScenarioAJsonString(ScenarioARunIdSource),
		*QuoteScenarioAJsonString(FCommandLine::Get()));

	const FString RunMetadataPath = FPaths::Combine(RunDirectory, TEXT("run.json"));
	if (FFileHelper::SaveStringToFile(Json, *RunMetadataPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM))
	{
		UE_LOG(LogIrisDemo, Log, TEXT("Scenario A run metadata saved: RunId=%s Path=%s"), *RunId, *RunMetadataPath);
	}
	else
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A run metadata save failed: RunId=%s Path=%s"), *RunId, *RunMetadataPath);
	}
}
