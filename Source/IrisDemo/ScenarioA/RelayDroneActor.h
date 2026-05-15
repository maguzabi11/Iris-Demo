// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScenarioA/RelayInterestTypes.h"
#include "RelayDroneActor.generated.h"

UENUM(BlueprintType)
enum class EDroneRelayState : uint8
{
	Patrol,
	Investigating,
	Returning,
	Disabled
};

UCLASS()
class IRISDEMO_API ARelayDroneActor : public AActor
{
	GENERATED_BODY()

public:
	ARelayDroneActor();

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetDroneId() const { return DroneId; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetZoneId() const { return ZoneId; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	ERelayInterestCategory GetInterestCategory() const { return InterestCategory; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	ERelayInterestDetailLevel GetInterestDetailLevel() const { return InterestDetailLevel; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	FName GetDebugName() const { return DebugName; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetLastUpdateSequence() const { return LastUpdateSequence; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	bool IsScenarioAEnabled() const { return bScenarioAEnabled; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetBatteryPercent() const { return BatteryPercent; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	EDroneRelayState GetDroneState() const { return DroneState; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario A")
	void ConfigureDrone(int32 NewDroneId, int32 NewZoneId, FName NewDebugName, bool bNewScenarioAEnabled);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario A")
	void SetDroneState(int32 NewBatteryPercent, EDroneRelayState NewDroneState);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_LastUpdateSequence();

private:
	void RefreshVisualState();

	void LogDroneState(const TCHAR* Reason) const;

	static const TCHAR* LexToString(EDroneRelayState State);

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario A")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario A")
	TObjectPtr<class UStaticMeshComponent> DroneMesh;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario A")
	TObjectPtr<class UTextRenderComponent> DroneLabel;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	int32 DroneId = INDEX_NONE;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	int32 ZoneId = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	ERelayInterestCategory InterestCategory = ERelayInterestCategory::DroneDetail;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	ERelayInterestDetailLevel InterestDetailLevel = ERelayInterestDetailLevel::Detail;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	FName DebugName = FName(TEXT("Drone"));

	UPROPERTY(ReplicatedUsing = OnRep_LastUpdateSequence, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 LastUpdateSequence = 0;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	bool bScenarioAEnabled = true;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 BatteryPercent = 100;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	EDroneRelayState DroneState = EDroneRelayState::Patrol;
};
