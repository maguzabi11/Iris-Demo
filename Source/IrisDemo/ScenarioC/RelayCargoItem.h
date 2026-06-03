// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RelayCargoItem.generated.h"

UENUM(BlueprintType)
enum class ERelayCargoItemState : uint8
{
	Stored,
	Reserved,
	Consumed
};

UCLASS(BlueprintType)
class IRISDEMO_API URelayCargoItem : public UObject
{
	GENERATED_BODY()

public:
	virtual bool IsSupportedForNetworking() const override { return true; }

	virtual void RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags) override;

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	int32 GetItemId() const { return ItemId; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	FName GetItemTag() const { return ItemTag; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	int32 GetStackCount() const { return StackCount; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	int32 GetDurability() const { return Durability; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	int32 GetCharge() const { return Charge; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	ERelayCargoItemState GetItemState() const { return ItemState; }

	UFUNCTION(BlueprintPure, Category = "Iris Relay|Scenario C")
	int32 GetLastUpdateSequence() const { return LastUpdateSequence; }

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario C")
	void ConfigureItem(int32 NewItemId, FName NewItemTag, int32 NewStackCount, int32 NewDurability, int32 NewCharge);

	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Iris Relay|Scenario C")
	void SetCargoState(int32 NewStackCount, int32 NewDurability, int32 NewCharge, ERelayCargoItemState NewItemState);

protected:
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION()
	void OnRep_LastUpdateSequence();

private:
	void LogCargoItemState(const TCHAR* Reason) const;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	int32 ItemId = INDEX_NONE;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	FName ItemTag = FName(TEXT("CargoItem"));

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	int32 StackCount = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	int32 Durability = 100;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	int32 Charge = 0;

	UPROPERTY(Replicated, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	ERelayCargoItemState ItemState = ERelayCargoItemState::Stored;

	UPROPERTY(ReplicatedUsing = OnRep_LastUpdateSequence, VisibleAnywhere, Category = "Iris Relay|Scenario C")
	int32 LastUpdateSequence = 0;
};
