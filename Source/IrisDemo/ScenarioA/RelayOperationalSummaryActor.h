// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScenarioA/RelayInterestTypes.h"
#include "RelayOperationalSummaryActor.generated.h"

UCLASS()
class IRISDEMO_API ARelayOperationalSummaryActor : public AActor
{
	GENERATED_BODY()

public:
	ARelayOperationalSummaryActor();

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetSummaryId() const { return SummaryId; }

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
	int32 GetKnownAlertCount() const { return KnownAlertCount; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetKnownDroneCount() const { return KnownDroneCount; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetKnownSupplyCount() const { return KnownSupplyCount; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario A")
	void ConfigureSummary(int32 NewSummaryId, FName NewDebugName, bool bNewScenarioAEnabled);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario A")
	void SetSummaryState(int32 NewKnownAlertCount, int32 NewKnownDroneCount, int32 NewKnownSupplyCount);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_LastUpdateSequence();

private:
	void RefreshVisualState();

	void LogSummaryState(const TCHAR* Reason) const;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario A")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario A")
	TObjectPtr<class UStaticMeshComponent> SummaryMesh;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario A")
	TObjectPtr<class UTextRenderComponent> SummaryLabel;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	int32 SummaryId = INDEX_NONE;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	ERelayInterestCategory InterestCategory = ERelayInterestCategory::OperationalSummary;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	ERelayInterestDetailLevel InterestDetailLevel = ERelayInterestDetailLevel::Summary;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	FName DebugName = FName(TEXT("OperationalSummary"));

	UPROPERTY(ReplicatedUsing = OnRep_LastUpdateSequence, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 LastUpdateSequence = 0;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	bool bScenarioAEnabled = true;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 KnownAlertCount = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 KnownDroneCount = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 KnownSupplyCount = 0;
};
