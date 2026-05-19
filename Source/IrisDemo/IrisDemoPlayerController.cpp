// Copyright Epic Games, Inc. All Rights Reserved.


#include "IrisDemoPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "EngineUtils.h"
#include "IrisDemo.h"
#include "ScenarioA/RelayDroneActor.h"
#include "ScenarioA/RelayOperationalSummaryActor.h"
#include "ScenarioA/RelayPlayerState.h"
#include "ScenarioA/RelaySensorActor.h"
#include "ScenarioA/RelaySupplyCrateActor.h"
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

	FString FormatScenarioAZoneTotals(const FScenarioAClientActorStats& Stats)
	{
		return FString::Printf(TEXT("Z0=%d,Z1=%d,Z2=%d,Unknown=%d"),
			Stats.ZoneTotals.IsValidIndex(0) ? Stats.ZoneTotals[0] : 0,
			Stats.ZoneTotals.IsValidIndex(1) ? Stats.ZoneTotals[1] : 0,
			Stats.ZoneTotals.IsValidIndex(2) ? Stats.ZoneTotals[2] : 0,
			Stats.UnknownZoneTotal);
	}
}

void AIrisDemoPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// only spawn touch controls on local player controllers
	if (ShouldUseTouchControls() && IsLocalPlayerController())
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
	const int32 DetailActorTotal = SensorStats.Total + DroneStats.Total + SupplyCrateStats.Total;
	const int32 DetailLastSequence = FMath::Max3(SensorStats.LastSequence, DroneStats.LastSequence, SupplyCrateStats.LastSequence);

	const ARelayPlayerState* RelayPlayerState = GetPlayerState<ARelayPlayerState>();
	const FString RoleName = RelayPlayerState ? RelayPlayerState->GetOperatorRoleName() : TEXT("Unassigned");
	const int32 AssignedZoneId = RelayPlayerState ? RelayPlayerState->GetAssignedZoneId() : INDEX_NONE;

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A baseline client snapshot: Source=%s Mode=%s Role=%s Zone=%d Controller=%s SensorCount=%d DroneCount=%d SupplyCrateCount=%d SummaryCount=%d DetailActorTotal=%d SensorZones=[%s] DroneZones=[%s] SupplyCrateZones=[%s] LastSequences=[Sensor:%d Drone:%d SupplyCrate:%d Summary:%d DetailMax:%d]"),
		Source ? Source : TEXT("Unknown"),
		*GetIrisReplicationModeLabel(),
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
		DetailLastSequence);
}
