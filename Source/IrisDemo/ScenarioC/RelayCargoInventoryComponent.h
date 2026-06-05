// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RelayCargoInventoryComponent.generated.h"

class URelayCargoItem;

UCLASS(ClassGroup = (IrisRelay), meta = (BlueprintSpawnableComponent))
class IRISDEMO_API URelayCargoInventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	URelayCargoInventoryComponent();

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	int32 GetCargoItemCount() const { return CargoItems.Num(); }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	int32 GetMaxCargoItemSequence() const;

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	URelayCargoItem* GetCargoItem(int32 ItemIndex) const;

	const TArray<TObjectPtr<URelayCargoItem>>& GetCargoItems() const { return CargoItems; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario C")
	void InitializeInventory(int32 StationId, int32 ItemCount);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario C")
	void UpdateCargoItems();

protected:
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_CargoItems();

private:
	void RegisterCargoItem(URelayCargoItem* CargoItem);

	void UnregisterCargoItems();

	void LogInventoryState(const TCHAR* Reason) const;

	UPROPERTY(ReplicatedUsing = OnRep_CargoItems, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	TArray<TObjectPtr<URelayCargoItem>> CargoItems;
};
