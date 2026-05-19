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
	template <typename TActorType>
	int32 CountScenarioAActors(UWorld* World)
	{
		if (!World)
		{
			return 0;
		}

		int32 Count = 0;
		for (TActorIterator<TActorType> It(World); It; ++It)
		{
			if (IsValid(*It))
			{
				++Count;
			}
		}

		return Count;
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

	const int32 SensorCount = CountScenarioAActors<ARelaySensorActor>(World);
	const int32 DroneCount = CountScenarioAActors<ARelayDroneActor>(World);
	const int32 SupplyCrateCount = CountScenarioAActors<ARelaySupplyCrateActor>(World);
	const int32 SummaryCount = CountScenarioAActors<ARelayOperationalSummaryActor>(World);
	const int32 DetailActorTotal = SensorCount + DroneCount + SupplyCrateCount;

	const ARelayPlayerState* RelayPlayerState = GetPlayerState<ARelayPlayerState>();
	const FString RoleName = RelayPlayerState ? RelayPlayerState->GetOperatorRoleName() : TEXT("Unassigned");
	const int32 AssignedZoneId = RelayPlayerState ? RelayPlayerState->GetAssignedZoneId() : INDEX_NONE;

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A baseline client snapshot: Source=%s Mode=%s Role=%s Zone=%d Controller=%s SensorCount=%d DroneCount=%d SupplyCrateCount=%d SummaryCount=%d DetailActorTotal=%d"),
		Source ? Source : TEXT("Unknown"),
		*GetIrisReplicationModeLabel(),
		*RoleName,
		AssignedZoneId,
		*GetName(),
		SensorCount,
		DroneCount,
		SupplyCrateCount,
		SummaryCount,
		DetailActorTotal);
}
