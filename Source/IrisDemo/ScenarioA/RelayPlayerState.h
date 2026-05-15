// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ScenarioA/RelayOperatorRole.h"
#include "RelayPlayerState.generated.h"

UCLASS()
class IRISDEMO_API ARelayPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	ARelayPlayerState();

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Role")
	ERelayOperatorRole GetOperatorRole() const { return OperatorRole; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Role")
	FString GetOperatorRoleName() const;

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetAssignedZoneId() const { return AssignedZoneId; }

	void SetOperatorRole(ERelayOperatorRole NewRole);

	void SetAssignedZoneId(int32 NewAssignedZoneId);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_OperatorRole();

	UFUNCTION()
	void OnRep_AssignedZoneId();

private:
	UPROPERTY(ReplicatedUsing = OnRep_OperatorRole, VisibleAnywhere, Category = "Iris Relay|Role")
	ERelayOperatorRole OperatorRole = ERelayOperatorRole::Unassigned;

	UPROPERTY(ReplicatedUsing = OnRep_AssignedZoneId, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 AssignedZoneId = INDEX_NONE;
};
