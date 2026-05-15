// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RelaySensorActor.generated.h"

UCLASS()
class IRISDEMO_API ARelaySensorActor : public AActor
{
	GENERATED_BODY()

public:
	ARelaySensorActor();

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetZoneId() const { return ZoneId; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	FName GetDebugName() const { return DebugName; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetLastUpdateSequence() const { return LastUpdateSequence; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	bool IsScenarioAEnabled() const { return bScenarioAEnabled; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetAlertLevel() const { return AlertLevel; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	bool IsTriggered() const { return bTriggered; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario A")
	void ConfigureSensor(int32 NewZoneId, FName NewDebugName, bool bNewScenarioAEnabled);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario A")
	void SetSensorState(int32 NewAlertLevel, bool bNewTriggered);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_LastUpdateSequence();

private:
	void LogSensorState(const TCHAR* Reason) const;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	int32 ZoneId = 0;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	FName DebugName = FName(TEXT("Sensor"));

	UPROPERTY(ReplicatedUsing = OnRep_LastUpdateSequence, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 LastUpdateSequence = 0;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	bool bScenarioAEnabled = true;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 AlertLevel = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	bool bTriggered = false;
};
