// Copyright Epic Games, Inc. All Rights Reserved.


#include "IrisDemoPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "HAL/FileManager.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "HAL/PlatformMisc.h"
#include "IrisDemo.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "ScenarioA/RelayDroneActor.h"
#include "ScenarioA/RelayOperationalSummaryActor.h"
#include "ScenarioA/RelayPlayerState.h"
#include "ScenarioA/RelaySensorActor.h"
#include "ScenarioA/RelaySupplyCrateActor.h"
#include "ScenarioC/RelayCargoItem.h"
#include "ScenarioC/RelayCargoStationActor.h"
#include "TimerManager.h"
#include "Widgets/Input/SVirtualJoystick.h"

namespace
{
	constexpr int32 ScenarioASnapshotZoneCount = 3;

	struct FScenarioAClientActorStats
	{
		int32 Total = 0;
		int32 UnknownZoneTotal = 0;
		int32 LastSequence = 0;
		TArray<int32> ZoneTotals;

		FScenarioAClientActorStats()
		{
			ZoneTotals.Init(0, ScenarioASnapshotZoneCount);
		}
	};

	struct FScenarioASummaryStats
	{
		int32 Total = 0;
		int32 LastSequence = 0;
	};

	struct FScenarioCCargoStats
	{
		int32 StationTotal = 0;
		int32 CargoItemTotal = 0;
		int32 StationLastSequence = 0;
		int32 CargoItemLastSequence = 0;
	};

	template <typename TActorType>
	FScenarioAClientActorStats CollectScenarioAClientActorStats(UWorld* World)
	{
		FScenarioAClientActorStats Stats;
		if (!World)
		{
			return Stats;
		}

		for (TActorIterator<TActorType> It(World); It; ++It)
		{
			const TActorType* Actor = *It;
			if (!IsValid(Actor))
			{
				continue;
			}

			++Stats.Total;
			Stats.LastSequence = FMath::Max(Stats.LastSequence, Actor->GetLastUpdateSequence());

			const int32 ZoneId = Actor->GetZoneId();
			if (Stats.ZoneTotals.IsValidIndex(ZoneId))
			{
				++Stats.ZoneTotals[ZoneId];
			}
			else
			{
				++Stats.UnknownZoneTotal;
			}
		}

		return Stats;
	}

	FScenarioASummaryStats CollectScenarioASummaryStats(UWorld* World)
	{
		FScenarioASummaryStats Stats;
		if (!World)
		{
			return Stats;
		}

		for (TActorIterator<ARelayOperationalSummaryActor> It(World); It; ++It)
		{
			const ARelayOperationalSummaryActor* Summary = *It;
			if (!IsValid(Summary))
			{
				continue;
			}

			++Stats.Total;
			Stats.LastSequence = FMath::Max(Stats.LastSequence, Summary->GetLastUpdateSequence());
		}

		return Stats;
	}

	FScenarioCCargoStats CollectScenarioCCargoStats(UWorld* World)
	{
		FScenarioCCargoStats Stats;
		if (!World)
		{
			return Stats;
		}

		for (TActorIterator<ARelayCargoStationActor> It(World); It; ++It)
		{
			const ARelayCargoStationActor* CargoStation = *It;
			if (!IsValid(CargoStation))
			{
				continue;
			}

			++Stats.StationTotal;
			Stats.StationLastSequence = FMath::Max(Stats.StationLastSequence, CargoStation->GetLastUpdateSequence());

			const URelayCargoItem* CargoItem = CargoStation->GetCargoItem();
			if (IsValid(CargoItem))
			{
				++Stats.CargoItemTotal;
				Stats.CargoItemLastSequence = FMath::Max(Stats.CargoItemLastSequence, CargoItem->GetLastUpdateSequence());
			}
		}

		return Stats;
	}

	FString FormatScenarioAZoneTotals(const FScenarioAClientActorStats& Stats)
	{
		return FString::Printf(TEXT("Z0=%d,Z1=%d,Z2=%d,Unknown=%d"),
			Stats.ZoneTotals.IsValidIndex(0) ? Stats.ZoneTotals[0] : 0,
			Stats.ZoneTotals.IsValidIndex(1) ? Stats.ZoneTotals[1] : 0,
			Stats.ZoneTotals.IsValidIndex(2) ? Stats.ZoneTotals[2] : 0,
			Stats.UnknownZoneTotal);
	}


	FString FormatScenarioACsvCell(const FString& Value)
	{
		if (!Value.Contains(TEXT(",")) && !Value.Contains(TEXT("\"")) && !Value.Contains(TEXT("\n")) && !Value.Contains(TEXT("\r")))
		{
			return Value;
		}

		FString Escaped = Value;
		Escaped.ReplaceInline(TEXT("\""), TEXT("\"\""));
		return FString::Printf(TEXT("\"%s\""), *Escaped);
	}

	FString GetScenarioAClientSnapshotCsvHeader()
	{
		return TEXT("Timestamp,RunId,Source,Mode,NetMode,Role,Zone,Controller,SensorCount,DroneCount,SupplyCrateCount,SummaryCount,DetailActorTotal,SensorZ0,SensorZ1,SensorZ2,SensorUnknown,DroneZ0,DroneZ1,DroneZ2,DroneUnknown,SupplyCrateZ0,SupplyCrateZ1,SupplyCrateZ2,SupplyCrateUnknown,SensorLastSequence,DroneLastSequence,SupplyCrateLastSequence,SummaryLastSequence,DetailMaxSequence,CargoStationCount,CargoItemCount,CargoStationLastSequence,CargoItemLastSequence");
	}

	int32 GetScenarioAZoneTotal(const FScenarioAClientActorStats& Stats, int32 ZoneId)
	{
		return Stats.ZoneTotals.IsValidIndex(ZoneId) ? Stats.ZoneTotals[ZoneId] : 0;
	}
}

void AIrisDemoPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (IsLocalPlayerController() && ShouldUseTouchControls())
	{
		// spawn the mobile controls widget
		MobileControlsWidget = CreateWidget<UUserWidget>(this, MobileControlsWidgetClass);

		if (MobileControlsWidget)
		{
			// add the controls to the player screen
			MobileControlsWidget->AddToPlayerScreen(0);

		} else {

			UE_LOG(LogIrisDemo, Error, TEXT("Could not spawn mobile controls widget."));

		}

	}

	LogRelayRole();
	ScheduleScenarioAAutoBaselineSnapshot();
}

void AIrisDemoPlayerController::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	LogRelayRole();
}

void AIrisDemoPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// only add IMCs for local player controllers
	if (IsLocalPlayerController())
	{
		// Add Input Mapping Contexts
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
		{
			for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
			{
				Subsystem->AddMappingContext(CurrentContext, 0);
			}

			// only add these IMCs if we're not using mobile touch input
			if (!ShouldUseTouchControls())
			{
				for (UInputMappingContext* CurrentContext : MobileExcludedMappingContexts)
				{
					Subsystem->AddMappingContext(CurrentContext, 0);
				}
			}
		}
	}
}

void AIrisDemoPlayerController::IrisRelayLogBaselineSnapshot()
{
	LogScenarioABaselineSnapshot(TEXT("Console"));
}

void AIrisDemoPlayerController::ScheduleScenarioAAutoBaselineSnapshot()
{
	if (!IsLocalPlayerController() || bScenarioAAutoSnapshotLogged)
	{
		return;
	}

	float AutoSnapshotDelay = 61.0f;
	FParse::Value(FCommandLine::Get(), TEXT("-ScenarioAAutoSnapshotDelay="), AutoSnapshotDelay);
	FParse::Value(FCommandLine::Get(), TEXT("-ScenarioAAutoExitGraceSeconds="), ScenarioAAutoExitGraceSeconds);
	ScenarioAAutoExitGraceSeconds = FMath::Max(0.0f, ScenarioAAutoExitGraceSeconds);

	FString RawAutoExit;
	bScenarioAAutoExit = FParse::Value(FCommandLine::Get(), TEXT("-ScenarioAAutoExit="), RawAutoExit)
		&& (RawAutoExit.Equals(TEXT("1")) || RawAutoExit.Equals(TEXT("true"), ESearchCase::IgnoreCase) || RawAutoExit.Equals(TEXT("yes"), ESearchCase::IgnoreCase));

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	if (AutoSnapshotDelay <= 0.0f)
	{
		if (bScenarioAAutoExit)
		{
			ScheduleScenarioAAutoExit(ScenarioAAutoExitGraceSeconds, TEXT("ScheduleScenarioAAutoBaselineSnapshot: AutoSnapshotDelay<=0.0f"));
		}
		return;
	}

	World->GetTimerManager().SetTimer(
		ScenarioAAutoSnapshotTimerHandle,
		this,
		&AIrisDemoPlayerController::LogScenarioAAutoBaselineSnapshot,
		AutoSnapshotDelay,
		false);

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A auto client snapshot scheduled: Delay=%.2f AutoExit=%s Grace=%.2f Controller=%s"),
		AutoSnapshotDelay,
		bScenarioAAutoExit ? TEXT("Enabled") : TEXT("Disabled"),
		ScenarioAAutoExitGraceSeconds,
		*GetName());

	if (bScenarioAAutoExit)
	{
		ScheduleScenarioAAutoExit(AutoSnapshotDelay + ScenarioAAutoExitGraceSeconds, TEXT("ScheduleScenarioAAutoBaselineSnapshot"));
	}
}

void AIrisDemoPlayerController::LogScenarioAAutoBaselineSnapshot()
{
	if (bScenarioAAutoSnapshotLogged)
	{
		return;
	}

	bScenarioAAutoSnapshotLogged = true;
	LogScenarioABaselineSnapshot(TEXT("Auto"));

	if (bScenarioAAutoExit && !bScenarioAAutoExitScheduled)
	{
		ScheduleScenarioAAutoExit(ScenarioAAutoExitGraceSeconds, TEXT("LogScenarioAAutoBaselineSnapshot"));
	}
}

void AIrisDemoPlayerController::ScheduleScenarioAAutoExit(float DelaySeconds, const TCHAR* Caller)
{
	if (bScenarioAAutoExitScheduled)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	bScenarioAAutoExitScheduled = true;
	const float ClampedDelaySeconds = FMath::Max(0.0f, DelaySeconds);
	World->GetTimerManager().SetTimer(
		ScenarioAAutoExitTimerHandle,
		this,
		&AIrisDemoPlayerController::RequestScenarioAAutoExit,
		ClampedDelaySeconds,
		false);

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A client auto exit scheduled: Delay=%.2f Controller=%s Caller=%s"),
		ClampedDelaySeconds,
		*GetName(),
		Caller);
}

void AIrisDemoPlayerController::RequestScenarioAAutoExit()
{
	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A client auto exit requested: Controller=%s"), *GetName());
	ConsoleCommand(TEXT("quit"), true);
	FPlatformMisc::RequestExit(false);
}

bool AIrisDemoPlayerController::ShouldUseTouchControls() const
{
	// are we on a mobile platform? Should we force touch?
	return SVirtualJoystick::ShouldDisplayTouchInterface() || bForceTouchControls;
}

void AIrisDemoPlayerController::LogRelayRole() const
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	const ARelayPlayerState* RelayPlayerState = GetPlayerState<ARelayPlayerState>();
	if (!RelayPlayerState)
	{
		UE_LOG(LogIrisDemo, Log, TEXT("Local relay role pending: Controller=%s"), *GetName());
		return;
	}

	UE_LOG(LogIrisDemo, Log, TEXT("Local relay role ready: Controller=%s Role=%s Zone=%d"),
		*GetName(),
		*RelayPlayerState->GetOperatorRoleName(),
		RelayPlayerState->GetAssignedZoneId());
}

void AIrisDemoPlayerController::LogScenarioABaselineSnapshot(const TCHAR* Source) const
{
	if (!IsLocalPlayerController())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FScenarioAClientActorStats SensorStats = CollectScenarioAClientActorStats<ARelaySensorActor>(World);
	const FScenarioAClientActorStats DroneStats = CollectScenarioAClientActorStats<ARelayDroneActor>(World);
	const FScenarioAClientActorStats SupplyCrateStats = CollectScenarioAClientActorStats<ARelaySupplyCrateActor>(World);
	const FScenarioASummaryStats SummaryStats = CollectScenarioASummaryStats(World);
	const FScenarioCCargoStats CargoStats = CollectScenarioCCargoStats(World);
	const int32 DetailActorTotal = SensorStats.Total + DroneStats.Total + SupplyCrateStats.Total;
	const int32 DetailLastSequence = FMath::Max3(SensorStats.LastSequence, DroneStats.LastSequence, SupplyCrateStats.LastSequence);

	const ARelayPlayerState* RelayPlayerState = GetPlayerState<ARelayPlayerState>();
	const FString RoleName = RelayPlayerState ? RelayPlayerState->GetOperatorRoleName() : TEXT("Unassigned");
	const int32 AssignedZoneId = RelayPlayerState ? RelayPlayerState->GetAssignedZoneId() : INDEX_NONE;

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A baseline client snapshot: Source=%s Mode=%s Role=%s Zone=%d Controller=%s SensorCount=%d DroneCount=%d SupplyCrateCount=%d SummaryCount=%d DetailActorTotal=%d SensorZones=[%s] DroneZones=[%s] SupplyCrateZones=[%s] LastSequences=[Sensor:%d Drone:%d SupplyCrate:%d Summary:%d DetailMax:%d] ScenarioC=[CargoStationCount:%d CargoItemCount:%d StationLastSequence:%d ItemLastSequence:%d]"),
		Source ? Source : TEXT("Unknown"),
		*GetIrisReplicationModeLabel(World),
		*RoleName,
		AssignedZoneId,
		*GetName(),
		SensorStats.Total,
		DroneStats.Total,
		SupplyCrateStats.Total,
		SummaryStats.Total,
		DetailActorTotal,
		*FormatScenarioAZoneTotals(SensorStats),
		*FormatScenarioAZoneTotals(DroneStats),
		*FormatScenarioAZoneTotals(SupplyCrateStats),
		SensorStats.LastSequence,
		DroneStats.LastSequence,
		SupplyCrateStats.LastSequence,
		SummaryStats.LastSequence,
		DetailLastSequence,
		CargoStats.StationTotal,
		CargoStats.CargoItemTotal,
		CargoStats.StationLastSequence,
		CargoStats.CargoItemLastSequence);

	const FString RunId = GetScenarioARunId();
	const FString RunDirectory = GetScenarioARunDirectory(RunId);
	IFileManager::Get().MakeDirectory(*RunDirectory, true);

	const FString SnapshotCsvPath = FPaths::Combine(RunDirectory, TEXT("client_snapshots.csv"));
	const bool bWriteHeader = !IFileManager::Get().FileExists(*SnapshotCsvPath);
	const FString CsvRow = FString::Printf(TEXT("%s,%s,%s,%s,%s,%s,%d,%s,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d%s"),
		*FormatScenarioACsvCell(FDateTime::Now().ToIso8601()),
		*FormatScenarioACsvCell(RunId),
		*FormatScenarioACsvCell(Source ? Source : TEXT("Unknown")),
		*FormatScenarioACsvCell(GetIrisReplicationModeLabel(World)),
		*FormatScenarioACsvCell(GetScenarioANetModeLabel(World)),
		*FormatScenarioACsvCell(RoleName),
		AssignedZoneId,
		*FormatScenarioACsvCell(GetName()),
		SensorStats.Total,
		DroneStats.Total,
		SupplyCrateStats.Total,
		SummaryStats.Total,
		DetailActorTotal,
		GetScenarioAZoneTotal(SensorStats, 0),
		GetScenarioAZoneTotal(SensorStats, 1),
		GetScenarioAZoneTotal(SensorStats, 2),
		SensorStats.UnknownZoneTotal,
		GetScenarioAZoneTotal(DroneStats, 0),
		GetScenarioAZoneTotal(DroneStats, 1),
		GetScenarioAZoneTotal(DroneStats, 2),
		DroneStats.UnknownZoneTotal,
		GetScenarioAZoneTotal(SupplyCrateStats, 0),
		GetScenarioAZoneTotal(SupplyCrateStats, 1),
		GetScenarioAZoneTotal(SupplyCrateStats, 2),
		SupplyCrateStats.UnknownZoneTotal,
		SensorStats.LastSequence,
		DroneStats.LastSequence,
		SupplyCrateStats.LastSequence,
		SummaryStats.LastSequence,
		DetailLastSequence,
		CargoStats.StationTotal,
		CargoStats.CargoItemTotal,
		CargoStats.StationLastSequence,
		CargoStats.CargoItemLastSequence,
		LINE_TERMINATOR);

	const FString CsvText = bWriteHeader
		? FString::Printf(TEXT("%s%s%s"), *GetScenarioAClientSnapshotCsvHeader(), LINE_TERMINATOR, *CsvRow)
		: CsvRow;

	if (FFileHelper::SaveStringToFile(CsvText, *SnapshotCsvPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM, &IFileManager::Get(), FILEWRITE_Append))
	{
		UE_LOG(LogIrisDemo, Log, TEXT("Scenario A client snapshot csv appended: RunId=%s Path=%s"), *RunId, *SnapshotCsvPath);
	}
	else
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A client snapshot csv append failed: RunId=%s Path=%s"), *RunId, *SnapshotCsvPath);
	}
}
