// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioC/RelayCargoInventoryComponent.h"

#include "GameFramework/Actor.h"
#include "IrisDemo.h"
#include "Net/UnrealNetwork.h"
#include "ScenarioC/RelayCargoItem.h"

URelayCargoInventoryComponent::URelayCargoInventoryComponent()
{
	SetIsReplicatedByDefault(true);
}

int32 URelayCargoInventoryComponent::GetMaxCargoItemSequence() const
{
	int32 MaxSequence = 0;
	for (const URelayCargoItem* CargoItem : CargoItems)
	{
		if (IsValid(CargoItem))
		{
			MaxSequence = FMath::Max(MaxSequence, CargoItem->GetLastUpdateSequence());
		}
	}

	return MaxSequence;
}

URelayCargoItem* URelayCargoInventoryComponent::GetCargoItem(int32 ItemIndex) const
{
	return CargoItems.IsValidIndex(ItemIndex) ? CargoItems[ItemIndex] : nullptr;
}

void URelayCargoInventoryComponent::InitializeInventory(int32 StationId, int32 ItemCount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority() || CargoItems.Num() > 0)
	{
		return;
	}

	const int32 ClampedItemCount = FMath::Max(0, ItemCount);
	CargoItems.Reserve(ClampedItemCount);

	for (int32 ItemIndex = 0; ItemIndex < ClampedItemCount; ++ItemIndex)
	{
		const FName ObjectName(*FString::Printf(TEXT("ScenarioC_CargoItem_%02d"), ItemIndex));
		URelayCargoItem* CargoItem = NewObject<URelayCargoItem>(GetOwner(), URelayCargoItem::StaticClass(), ObjectName);
		if (!CargoItem)
		{
			UE_LOG(LogIrisDemo, Warning, TEXT("Scenario C cargo item creation failed: Owner=%s Index=%d"), *GetNameSafe(GetOwner()), ItemIndex);
			continue;
		}

		const int32 ItemId = StationId * 100 + ItemIndex;
		const FName ItemTag(*FString::Printf(TEXT("RelayMedKit_%02d"), ItemIndex));
		CargoItem->ConfigureItem(ItemId, ItemTag, 6 + ItemIndex, 100, (25 + ItemIndex * 11) % 101);
		CargoItems.Add(CargoItem);
		RegisterCargoItem(CargoItem);
	}

	LogInventoryState(TEXT("initialized"));
}

void URelayCargoInventoryComponent::UpdateCargoItems()
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	for (int32 ItemIndex = 0; ItemIndex < CargoItems.Num(); ++ItemIndex)
	{
		URelayCargoItem* CargoItem = CargoItems[ItemIndex];
		if (!IsValid(CargoItem))
		{
			continue;
		}

		const int32 NextStackCount = CargoItem->GetStackCount() <= 1 ? 6 + ItemIndex : CargoItem->GetStackCount() - 1;
		const int32 NextDurability = CargoItem->GetDurability() <= 10 ? 100 : CargoItem->GetDurability() - (7 + ItemIndex);
		const int32 NextCharge = (CargoItem->GetCharge() + 17 + ItemIndex) % 101;
		const ERelayCargoItemState NextState = NextStackCount <= 1 ? ERelayCargoItemState::Reserved : ERelayCargoItemState::Stored;

		CargoItem->SetCargoState(NextStackCount, NextDurability, NextCharge, NextState);
	}

	LogInventoryState(TEXT("updated"));
}

void URelayCargoInventoryComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterCargoItems();

	Super::EndPlay(EndPlayReason);
}

void URelayCargoInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URelayCargoInventoryComponent, CargoItems);
}

void URelayCargoInventoryComponent::OnRep_CargoItems()
{
	LogInventoryState(TEXT("item array replicated"));
}

void URelayCargoInventoryComponent::RegisterCargoItem(URelayCargoItem* CargoItem)
{
	AActor* Owner = GetOwner();
	if (!Owner || !CargoItem)
	{
		return;
	}

	Owner->AddReplicatedSubObject(CargoItem);

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario C cargo item registered: Owner=%s Inventory=%s Item=%s RegisteredList=%s"),
		*GetNameSafe(Owner),
		*GetNameSafe(this),
		*GetNameSafe(CargoItem),
		Owner->IsUsingRegisteredSubObjectList() ? TEXT("true") : TEXT("false"));
}

void URelayCargoInventoryComponent::UnregisterCargoItems()
{
	AActor* Owner = GetOwner();
	if (!Owner || !Owner->IsUsingRegisteredSubObjectList())
	{
		return;
	}

	for (URelayCargoItem* CargoItem : CargoItems)
	{
		if (CargoItem)
		{
			Owner->RemoveReplicatedSubObject(CargoItem);
		}
	}
}

void URelayCargoInventoryComponent::LogInventoryState(const TCHAR* Reason) const
{
	UE_LOG(LogIrisDemo, Log, TEXT("Relay cargo inventory %s: Owner=%s ItemCount=%d MaxItemSequence=%d Component=%s"),
		Reason,
		*GetNameSafe(GetOwner()),
		CargoItems.Num(),
		GetMaxCargoItemSequence(),
		*GetNameSafe(this));
}
