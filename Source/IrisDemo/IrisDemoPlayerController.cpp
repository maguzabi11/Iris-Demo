// Copyright Epic Games, Inc. All Rights Reserved.


#include "IrisDemoPlayerController.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "InputMappingContext.h"
#include "Blueprint/UserWidget.h"
#include "IrisDemo.h"
#include "ScenarioA/RelayPlayerState.h"
#include "Widgets/Input/SVirtualJoystick.h"

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

	UE_LOG(LogIrisDemo, Log, TEXT("Local relay role ready: Controller=%s Role=%s"),
		*GetName(),
		*RelayPlayerState->GetOperatorRoleName());
}
