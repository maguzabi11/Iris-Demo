// Copyright Epic Games, Inc. All Rights Reserved.

#include "IrisDemoGameMode.h"

#include "GameFramework/PlayerController.h"
#include "IrisDemo.h"
#include "ScenarioA/RelayPlayerState.h"

AIrisDemoGameMode::AIrisDemoGameMode()
{
	EnsureScenarioAPlayerStateClass();
}

void AIrisDemoGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	EnsureScenarioAPlayerStateClass();

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
