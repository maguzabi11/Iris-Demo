// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioC/RelayCargoStationActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "IrisDemo.h"
#include "Net/UnrealNetwork.h"
#include "ScenarioC/RelayCargoInventoryComponent.h"
#include "ScenarioC/RelayCargoItem.h"
#include "UObject/ConstructorHelpers.h"

ARelayCargoStationActor::ARelayCargoStationActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	bReplicateUsingRegisteredSubObjectList = true;
	SetNetUpdateFrequency(2.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	StationMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationMesh"));
	StationMesh->SetupAttachment(SceneRoot);
	StationMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StationMesh->SetRelativeScale3D(FVector(0.75f, 0.75f, 0.2f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		StationMesh->SetStaticMesh(CubeMesh.Object);
	}

	StationLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("StationLabel"));
	StationLabel->SetupAttachment(SceneRoot);
	StationLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	StationLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	StationLabel->SetRelativeLocation(FVector(0.0, 0.0, 85.0));
	StationLabel->SetRelativeRotation(FRotator(60.0, 0.0, 0.0));
	StationLabel->SetTextRenderColor(FColor::Cyan);
	StationLabel->SetWorldSize(30.0f);

	CargoInventory = CreateDefaultSubobject<URelayCargoInventoryComponent>(TEXT("CargoInventory"));

	RefreshVisualState();
}

URelayCargoItem* ARelayCargoStationActor::GetCargoItem(int32 ItemIndex) const
{
	return CargoInventory ? CargoInventory->GetCargoItem(ItemIndex) : nullptr;
}

void ARelayCargoStationActor::ConfigureStation(int32 NewStationId, FName NewDebugName, int32 CargoItemCount)
{
	if (!HasAuthority())
	{
		return;
	}

	StationId = NewStationId;
	DebugName = NewDebugName;
	++LastUpdateSequence;

	if (CargoInventory)
	{
		CargoInventory->InitializeInventory(StationId, CargoItemCount);
	}

	RefreshVisualState();
	LogCargoStationState(TEXT("configured"));
}

void ARelayCargoStationActor::UpdateCargoItem()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!CargoInventory || CargoInventory->GetCargoItemCount() <= 0)
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario C cargo update skipped: Station=%s Reason=NoCargoItem"), *GetName());
		return;
	}

	const int32 NextSequence = LastUpdateSequence + 1;

	CargoInventory->UpdateCargoItems();
	LastUpdateSequence = NextSequence;

	RefreshVisualState();
	LogCargoStationState(TEXT("updated"));
}

void ARelayCargoStationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);
}

void ARelayCargoStationActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARelayCargoStationActor, StationId);
	DOREPLIFETIME(ARelayCargoStationActor, DebugName);
	DOREPLIFETIME(ARelayCargoStationActor, LastUpdateSequence);
}

void ARelayCargoStationActor::OnRep_LastUpdateSequence()
{
	RefreshVisualState();
	LogCargoStationState(TEXT("replicated"));
}

void ARelayCargoStationActor::RefreshVisualState()
{
	if (StationLabel)
	{
		const URelayCargoItem* CargoItem = GetCargoItem();
		const int32 CargoItemCount = CargoInventory ? CargoInventory->GetCargoItemCount() : 0;
		const int32 MaxItemSequence = CargoInventory ? CargoInventory->GetMaxCargoItemSequence() : 0;
		const FString ItemText = CargoItem
			? FString::Printf(TEXT("Items %d | MaxSeq %d\n%s S%d D%d C%d"),
				CargoItemCount,
				MaxItemSequence,
				*CargoItem->GetItemTag().ToString(),
				CargoItem->GetStackCount(),
				CargoItem->GetDurability(),
				CargoItem->GetCharge())
			: TEXT("No cargo item");

		StationLabel->SetText(FText::FromString(FString::Printf(TEXT("%s\nID %d | Seq %d\n%s"),
			*DebugName.ToString(),
			StationId,
			LastUpdateSequence,
			*ItemText)));
	}
}

void ARelayCargoStationActor::LogCargoStationState(const TCHAR* Reason) const
{
	UE_LOG(LogIrisDemo, Log, TEXT("Relay cargo station %s: StationId=%d DebugName=%s Sequence=%d CargoItemCount=%d CargoMaxSequence=%d Actor=%s"),
		Reason,
		StationId,
		*DebugName.ToString(),
		LastUpdateSequence,
		CargoInventory ? CargoInventory->GetCargoItemCount() : 0,
		CargoInventory ? CargoInventory->GetMaxCargoItemSequence() : 0,
		*GetName());
}
