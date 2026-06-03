// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "RelayCargoStationActor.generated.h"

class URelayCargoItem;

UCLASS()
class IRISDEMO_API ARelayCargoStationActor : public AActor
{
	GENERATED_BODY()

public:
	ARelayCargoStationActor();

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	int32 GetStationId() const { return StationId; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	FName GetDebugName() const { return DebugName; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	int32 GetLastUpdateSequence() const { return LastUpdateSequence; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	URelayCargoItem* GetCargoItem() const { return CargoItem; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario C")
	void ConfigureStation(int32 NewStationId, FName NewDebugName);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario C")
	void UpdateCargoItem();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CargoItem();

	UFUNCTION()
	void OnRep_LastUpdateSequence();

private:
	void CreateCargoItem();

	void RefreshVisualState();

	void LogCargoStationState(const TCHAR* Reason) const;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario C")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario C")
	TObjectPtr<class UStaticMeshComponent> StationMesh;

	UPROPERTY(VisibleAnywhere, Category = "Iris Relay|Scenario C")
	TObjectPtr<class UTextRenderComponent> StationLabel;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	int32 StationId = INDEX_NONE;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	FName DebugName = FName(TEXT("CargoStation"));

	UPROPERTY(ReplicatedUsing = OnRep_LastUpdateSequence, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	int32 LastUpdateSequence = 0;

	UPROPERTY(ReplicatedUsing = OnRep_CargoItem, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	TObjectPtr<URelayCargoItem> CargoItem;
};
