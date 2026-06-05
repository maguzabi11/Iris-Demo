// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ScenarioA/RelayOperatorRole.h"
#include "TimerManager.h"
#include "IrisDemoGameMode.generated.h"

class ARelaySensorActor;
class ARelayDroneActor;
class ARelayOperationalSummaryActor;
class ARelaySupplyCrateActor;
class ARelayCargoStationActor;

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
	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;

	virtual void BeginPlay() override;
	
	virtual void PostLogin(APlayerController* NewPlayer) override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;


private:
	void EnsureScenarioAPlayerStateClass();

	ERelayOperatorRole GetNextScenarioARole();

	int32 GetScenarioAZoneForRole(ERelayOperatorRole OperatorRole);

	void SpawnScenarioASensors();

	void UpdateScenarioASensors();

	void SpawnScenarioADrones();

	void UpdateScenarioADrones();

	void SpawnScenarioASupplyCrates();

	void UpdateScenarioASupplyCrates();

	void SpawnScenarioAOperationalSummary();

	void UpdateScenarioAOperationalSummary();

	void SpawnScenarioCCargoStations();

	void UpdateScenarioCCargoStations();

	void QueueScenarioAFilterRefresh();

	void ApplyScenarioARoleBasedFiltering();

	void ApplyScenarioAOptions(const FString& Options);

	void LogScenarioABaselineConfig() const;

	void LogScenarioABaselineComplete() const;

	void WriteScenarioARunMetadata() const;

	void StartScenarioANetworkMetrics();

	void StopScenarioANetworkMetrics();

	void LogScenarioANetworkMetricsSnapshot() const;

	void RequestScenarioAAutoExit();

	int32 NextScenarioARoleIndex = 0;

	int32 NextScenarioAFieldAgentZoneId = 0;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A")
	int32 ScenarioASeed = 1001;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A")
	TSubclassOf<ARelaySensorActor> ScenarioASensorClass;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0"))
	int32 ScenarioASensorCount = 60;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0.1"))
	float ScenarioASensorUpdateInterval = 2.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0.0"))
	float ScenarioARunDuration = 95.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0.0"))
	float ScenarioANetworkMetricsInterval = 1.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0.0"))
	float ScenarioANetworkMetricsStartDelay = 35.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0.0"))
	float ScenarioANetworkMetricsDuration = 60.0f;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A")
	bool bScenarioAEnableRoleFiltering = false;

	bool bScenarioAAutoExit = false;

	float ScenarioAAutoExitGraceSeconds = 5.0f;

	FString ScenarioARunId;

	FString ScenarioARunIdSource = TEXT("Default");

	FString ScenarioARoleFilteringSource = TEXT("Default");

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A")
	TSubclassOf<ARelayDroneActor> ScenarioADroneClass;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0"))
	int32 ScenarioADroneCount = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A")
	TSubclassOf<ARelaySupplyCrateActor> ScenarioASupplyCrateClass;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A", meta = (ClampMin = "0"))
	int32 ScenarioASupplyCrateCount = 30;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario A")
	TSubclassOf<ARelayOperationalSummaryActor> ScenarioAOperationalSummaryClass;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario C")
	bool bScenarioCEnableCargoSubobjects = true;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario C")
	TSubclassOf<ARelayCargoStationActor> ScenarioCCargoStationClass;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario C", meta = (ClampMin = "0"))
	int32 ScenarioCCargoStationCount = 1;

	UPROPERTY(EditDefaultsOnly, Category = "Iris Relay|Scenario C", meta = (ClampMin = "0"))
	int32 ScenarioCCargoItemCount = 1;

	UPROPERTY()
	TArray<TObjectPtr<ARelaySensorActor>> ScenarioASensors;

	UPROPERTY()
	TArray<TObjectPtr<ARelayDroneActor>> ScenarioADrones;

	UPROPERTY()
	TArray<TObjectPtr<ARelaySupplyCrateActor>> ScenarioASupplyCrates;

	UPROPERTY()
	TObjectPtr<ARelayOperationalSummaryActor> ScenarioAOperationalSummary;

	UPROPERTY()
	TArray<TObjectPtr<ARelayCargoStationActor>> ScenarioCCargoStations;

	FTimerHandle ScenarioASensorUpdateTimerHandle;

	FTimerHandle ScenarioABaselineCompleteTimerHandle;

	FTimerHandle ScenarioAFilterRefreshTimerHandle;

	FTimerHandle ScenarioANetworkMetricsStartTimerHandle;

	FTimerHandle ScenarioANetworkMetricsTimerHandle;

	FTimerHandle ScenarioANetworkMetricsStopTimerHandle;

	FTimerHandle ScenarioAAutoExitTimerHandle;

	double ScenarioANetworkMetricsWindowStartTime = 0.0;
};
