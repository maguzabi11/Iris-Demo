// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioC/RelayCargoItem.h"

#include "Iris/ReplicationSystem/ReplicationFragmentUtil.h"
#include "IrisDemo.h"
#include "Net/UnrealNetwork.h"

void URelayCargoItem::RegisterReplicationFragments(UE::Net::FFragmentRegistrationContext& Context, UE::Net::EFragmentRegistrationFlags RegistrationFlags)
{
	UE::Net::FReplicationFragmentUtil::CreateAndRegisterFragmentsForObject(this, Context, RegistrationFlags);
}

void URelayCargoItem::ConfigureItem(int32 NewItemId, FName NewItemTag, int32 NewStackCount, int32 NewDurability, int32 NewCharge)
{
	ItemId = NewItemId;
	ItemTag = NewItemTag;
	StackCount = FMath::Max(0, NewStackCount);
	Durability = FMath::Clamp(NewDurability, 0, 100);
	Charge = FMath::Clamp(NewCharge, 0, 100);
	ItemState = ERelayCargoItemState::Stored;
	++LastUpdateSequence;

	LogCargoItemState(TEXT("configured"));
}

void URelayCargoItem::SetCargoState(int32 NewStackCount, int32 NewDurability, int32 NewCharge, ERelayCargoItemState NewItemState)
{
	StackCount = FMath::Max(0, NewStackCount);
	Durability = FMath::Clamp(NewDurability, 0, 100);
	Charge = FMath::Clamp(NewCharge, 0, 100);
	ItemState = NewItemState;
	++LastUpdateSequence;

	LogCargoItemState(TEXT("updated"));
}

void URelayCargoItem::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(URelayCargoItem, ItemId);
	DOREPLIFETIME(URelayCargoItem, ItemTag);
	DOREPLIFETIME(URelayCargoItem, StackCount);
	DOREPLIFETIME(URelayCargoItem, Durability);
	DOREPLIFETIME(URelayCargoItem, Charge);
	DOREPLIFETIME(URelayCargoItem, ItemState);
	DOREPLIFETIME(URelayCargoItem, LastUpdateSequence);
}

void URelayCargoItem::OnRep_LastUpdateSequence()
{
	LogCargoItemState(TEXT("replicated"));
}

void URelayCargoItem::LogCargoItemState(const TCHAR* Reason) const
{
	UE_LOG(LogIrisDemo, Log, TEXT("Relay cargo item %s: ItemId=%d Tag=%s Stack=%d Durability=%d Charge=%d State=%s Sequence=%d Object=%s Outer=%s"),
		Reason,
		ItemId,
		*ItemTag.ToString(),
		StackCount,
		Durability,
		Charge,
		*StaticEnum<ERelayCargoItemState>()->GetNameStringByValue(static_cast<int64>(ItemState)),
		LastUpdateSequence,
		*GetName(),
		*GetNameSafe(GetOuter()));
}
