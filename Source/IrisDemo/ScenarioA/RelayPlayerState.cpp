// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioA/RelayPlayerState.h"

#include "IrisDemo.h"
#include "Net/UnrealNetwork.h"

ARelayPlayerState::ARelayPlayerState()
{
	bReplicates = true;
}

FString ARelayPlayerState::GetOperatorRoleName() const
{
	const UEnum* RoleEnum = StaticEnum<ERelayOperatorRole>();
	return RoleEnum ? RoleEnum->GetNameStringByValue(static_cast<int64>(OperatorRole)) : TEXT("Unknown");
}

void ARelayPlayerState::SetOperatorRole(ERelayOperatorRole NewRole)
{
	if (!HasAuthority() || OperatorRole == NewRole)
	{
		return;
	}

	OperatorRole = NewRole;

	UE_LOG(LogIrisDemo, Log, TEXT("Relay role assigned: PlayerState=%s Role=%s"),
		*GetName(),
		*GetOperatorRoleName());
}

void ARelayPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARelayPlayerState, OperatorRole);
}

void ARelayPlayerState::OnRep_OperatorRole()
{
	UE_LOG(LogIrisDemo, Log, TEXT("Relay role replicated: PlayerState=%s Role=%s"),
		*GetName(),
		*GetOperatorRoleName());
}
