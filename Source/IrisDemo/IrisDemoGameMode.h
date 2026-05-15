// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ScenarioA/RelayOperatorRole.h"
#include "TimerManager.h"
#include "IrisDemoGameMode.generated.h"

class ARelaySensorActor;
class ARelayDroneActor;
class ARelaySupplyCrateActor;

/**
 *  Simple GameMode for a third person game
 */
UCLASS(abstract)
class AIrisDemoGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	
	/** Constructor */
	AIrisDemoGameMode();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual void PostLogin(APlayerController* NewPlayer) override;

private:
	void EnsureScenarioAPlayerStateClass();

	ERelayOperatorRole GetNextScenarioARole();

	void SpawnScenarioASensors();

	void UpdateScenarioASensors();

	void SpawnScenarioADrones();

	void UpdateScenarioADrones();

	void SpawnScenarioASupplyCrates();

	void UpdateScenarioASupplyCrates();

	void ApplyScenarioAOptions(const FString& Options);

	void LogScenarioABaselineConfig() const;

	void LogScenarioABaselineComplete() const;

	int32 NextScenarioARoleIndex = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A")
	int32 ScenarioASeed = 1001;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A")
	TSubclassOf<ARelaySensorActor> ScenarioASensorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0"))
	int32 ScenarioASensorCount = 6;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0.1"))
	float ScenarioASensorUpdateInterval = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0.0"))
	float ScenarioARunDuration = 30.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A")
	TSubclassOf<ARelayDroneActor> ScenarioADroneClass;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0"))
	int32 ScenarioADroneCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A")
	TSubclassOf<ARelaySupplyCrateActor> ScenarioASupplyCrateClass;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0"))
	int32 ScenarioASupplyCrateCount = 3;

	UPROPERTY()
	TArray<TObjectPtr<ARelaySensorActor>> ScenarioASensors;

	UPROPERTY()
	TArray<TObjectPtr<ARelayDroneActor>> ScenarioADrones;

	UPROPERTY()
	TArray<TObjectPtr<ARelaySupplyCrateActor>> ScenarioASupplyCrates;

	FTimerHandle ScenarioASensorUpdateTimerHandle;

	FTimerHandle ScenarioABaselineCompleteTimerHandle;
};
