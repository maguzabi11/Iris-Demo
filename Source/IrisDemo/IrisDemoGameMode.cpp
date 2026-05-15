// Copyright Epic Games, Inc. All Rights Reserved.

#include "IrisDemoGameMode.h"

#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "IrisDemo.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "ScenarioA/RelayDroneActor.h"
#include "ScenarioA/RelayPlayerState.h"
#include "ScenarioA/RelaySensorActor.h"
#include "ScenarioA/RelaySupplyCrateActor.h"
#include "TimerManager.h"

namespace
{
FString GetIrisReplicationModeLabel()
{
	const IConsoleVariable* UseIrisCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("net.Iris.UseIrisReplication"));
	if (!UseIrisCVar)
	{
		return TEXT("Unknown");
	}

	return UseIrisCVar->GetInt() != 0 ? TEXT("Iris") : TEXT("Generic");
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

bool TryReadScenarioAOptionValue(const FString& Options, const TCHAR* OptionName, FString& OutValue)
{
	if (TryReadScenarioAUrlOption(Options, OptionName, OutValue))
	{
		return true;
	}

	const FString CommandLineOption = FString::Printf(TEXT("-%s="), OptionName);
	return FParse::Value(FCommandLine::Get(), *CommandLineOption, OutValue);
}

bool TryReadScenarioAIntOption(const FString& Options, const TCHAR* OptionName, int32& OutValue)
{
	FString RawValue;
	if (!TryReadScenarioAOptionValue(Options, OptionName, RawValue))
	{
		return false;
	}

	OutValue = FCString::Atoi(*RawValue);
	return true;
}

bool TryReadScenarioAFloatOption(const FString& Options, const TCHAR* OptionName, float& OutValue)
{
	FString RawValue;
	if (!TryReadScenarioAOptionValue(Options, OptionName, RawValue))
	{
		return false;
	}

	OutValue = FCString::Atof(*RawValue);
	return true;
}
}

AIrisDemoGameMode::AIrisDemoGameMode()
{
	EnsureScenarioAPlayerStateClass();
	ScenarioASensorClass = ARelaySensorActor::StaticClass();
	ScenarioADroneClass = ARelayDroneActor::StaticClass();
	ScenarioASupplyCrateClass = ARelaySupplyCrateActor::StaticClass();
}

void AIrisDemoGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnScenarioASensors();
	SpawnScenarioADrones();
	SpawnScenarioASupplyCrates();
	LogScenarioABaselineConfig();

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

	Super::EndPlay(EndPlayReason);
}

void AIrisDemoGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	EnsureScenarioAPlayerStateClass();
	ApplyScenarioAOptions(Options);

	Super::InitGame(MapName, Options, ErrorMessage);
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
		RelayPlayerState->SetOperatorRole(GetNextScenarioARole());
		return;
	}

	UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A role assignment skipped: PlayerStateClass=%s Player=%s"),
		PlayerStateClass ? *PlayerStateClass->GetName() : TEXT("None"),
		*NewPlayer->GetName());
}

void AIrisDemoGameMode::EnsureScenarioAPlayerStateClass()
{
	if (!PlayerStateClass || !PlayerStateClass->IsChildOf(ARelayPlayerState::StaticClass()))
	{
		PlayerStateClass = ARelayPlayerState::StaticClass();
	}
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

void AIrisDemoGameMode::ApplyScenarioAOptions(const FString& Options)
{
	TryReadScenarioAIntOption(Options, TEXT("ScenarioASeed"), ScenarioASeed);
	TryReadScenarioAIntOption(Options, TEXT("ScenarioASensorCount"), ScenarioASensorCount);
	TryReadScenarioAIntOption(Options, TEXT("ScenarioADroneCount"), ScenarioADroneCount);
	TryReadScenarioAIntOption(Options, TEXT("ScenarioASupplyCrateCount"), ScenarioASupplyCrateCount);
	TryReadScenarioAFloatOption(Options, TEXT("ScenarioAUpdateInterval"), ScenarioASensorUpdateInterval);
	TryReadScenarioAFloatOption(Options, TEXT("ScenarioARunDuration"), ScenarioARunDuration);

	ScenarioASeed = FMath::Max(0, ScenarioASeed);
	ScenarioASensorCount = FMath::Max(0, ScenarioASensorCount);
	ScenarioADroneCount = FMath::Max(0, ScenarioADroneCount);
	ScenarioASupplyCrateCount = FMath::Max(0, ScenarioASupplyCrateCount);
	ScenarioASensorUpdateInterval = FMath::Max(0.1f, ScenarioASensorUpdateInterval);
	ScenarioARunDuration = FMath::Max(0.0f, ScenarioARunDuration);
}

void AIrisDemoGameMode::LogScenarioABaselineConfig() const
{
	const int32 DetailActorTotal = ScenarioASensors.Num() + ScenarioADrones.Num() + ScenarioASupplyCrates.Num();
	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A baseline config: Mode=%s Seed=%d SensorCount=%d DroneCount=%d SupplyCrateCount=%d DetailActorTotal=%d UpdateInterval=%.2f RunDuration=%.2f ExpectedPreFilterClientDetailActors=%d"),
		*GetIrisReplicationModeLabel(),
		ScenarioASeed,
		ScenarioASensors.Num(),
		ScenarioADrones.Num(),
		ScenarioASupplyCrates.Num(),
		DetailActorTotal,
		ScenarioASensorUpdateInterval,
		ScenarioARunDuration,
		DetailActorTotal);
}

void AIrisDemoGameMode::LogScenarioABaselineComplete() const
{
	const int32 DetailActorTotal = ScenarioASensors.Num() + ScenarioADrones.Num() + ScenarioASupplyCrates.Num();
	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A baseline server window complete: Mode=%s Seed=%d Duration=%.2f DetailActorTotal=%d"),
		*GetIrisReplicationModeLabel(),
		ScenarioASeed,
		ScenarioARunDuration,
		DetailActorTotal);
}
