// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ScenarioA/RelayInterestTypes.h"
#include "RelaySupplyCrateActor.generated.h"

UCLASS()
class IRISDEMO_API ARelaySupplyCrateActor : public AActor
{
	GENERATED_BODY()

public:
	ARelaySupplyCrateActor();

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetCrateId() const { return CrateId; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetZoneId() const { return ZoneId; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	int32 GetOwningSquadId() const { return OwningSquadId; }

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
	int32 GetStockCount() const { return StockCount; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario A")
	bool IsReserved() const { return bReserved; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario A")
	void ConfigureCrate(int32 NewCrateId, int32 NewZoneId, int32 NewOwningSquadId, FName NewDebugName, bool bNewScenarioAEnabled);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario A")
	void SetCrateState(int32 NewStockCount, bool bNewReserved);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_LastUpdateSequence();

private:
	void RefreshVisualState();

	void LogCrateState(const TCHAR* Reason) const;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario A")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario A")
	TObjectPtr<class UStaticMeshComponent> CrateMesh;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario A")
	TObjectPtr<class UTextRenderComponent> CrateLabel;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	int32 CrateId = INDEX_NONE;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	int32 ZoneId = 0;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	int32 OwningSquadId = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	ERelayInterestCategory InterestCategory = ERelayInterestCategory::SupplyDetail;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	ERelayInterestDetailLevel InterestDetailLevel = ERelayInterestDetailLevel::Detail;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	FName DebugName = FName(TEXT("SupplyCrate"));

	UPROPERTY(ReplicatedUsing = OnRep_LastUpdateSequence, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 LastUpdateSequence = 0;

	UPROPERTY(Replicated, EditAnywhere, Category = "Iris Relay|Scenario A")
	bool bScenarioAEnabled = true;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	int32 StockCount = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario A")
	bool bReserved = false;
};
