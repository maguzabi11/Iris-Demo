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
	// --- 초기화·옵션 ---
	void EnsureScenarioAPlayerStateClass();

	void ApplyScenarioAOptions(const FString& Options);

	// --- 시뮬레이션: 액터 스폰·갱신·타이머 ---
	void SpawnScenarioActors();

	void StartScenarioTimers();

	float GetScenarioAAutoExitDelay() const;

	void TickScenarioSimulation();

	template <typename TActor>
	TActor* SpawnScenarioActor(TSubclassOf<TActor> ActorClass, const FVector& Location, const FRotator& Rotation);

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

	// --- 역할 할당 ---
	ERelayOperatorRole GetNextScenarioARole();

	int32 GetScenarioAZoneForRole(ERelayOperatorRole OperatorRole);

	// --- Iris 필터링 ---
	void QueueScenarioAFilterRefresh();

	void ApplyScenarioARoleBasedFiltering();

	// --- 측정·기록 ---
	void LogScenarioABaselineConfig() const;

	void LogScenarioABaselineComplete() const;

	void WriteScenarioARunMetadata() const;

	void StartScenarioANetworkMetrics();

	void StopScenarioANetworkMetrics();

	void LogScenarioANetworkMetricsSnapshot() const;

	void RequestScenarioAAutoExit();

	// --- 설정 (UPROPERTY·실행 옵션) ---
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

	// --- 스폰된 액터 ---
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

	// --- 런타임 상태 ---
	int32 NextScenarioARoleIndex = 0;

	int32 NextScenarioAFieldAgentZoneId = 0;

	double ScenarioANetworkMetricsWindowStartTime = 0.0;

	// --- 타이머 핸들 ---
	FTimerHandle ScenarioSimulationTickTimerHandle;

	FTimerHandle ScenarioABaselineCompleteTimerHandle;

	FTimerHandle ScenarioAFilterRefreshTimerHandle;

	FTimerHandle ScenarioANetworkMetricsStartTimerHandle;

	FTimerHandle ScenarioANetworkMetricsTimerHandle;

	FTimerHandle ScenarioANetworkMetricsStopTimerHandle;

	FTimerHandle ScenarioAAutoExitTimerHandle;
};
