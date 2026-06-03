// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioC/RelayCargoStationActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "IrisDemo.h"
#include "Net/UnrealNetwork.h"
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

	RefreshVisualState();
}

void ARelayCargoStationActor::ConfigureStation(int32 NewStationId, FName NewDebugName)
{
	if (!HasAuthority())
	{
		return;
	}

	StationId = NewStationId;
	DebugName = NewDebugName;
	++LastUpdateSequence;

	CreateCargoItem();
	RefreshVisualState();
	LogCargoStationState(TEXT("configured"));
}

void ARelayCargoStationActor::UpdateCargoItem()
{
	if (!HasAuthority())
	{
		return;
	}

	if (!CargoItem)
	{
		CreateCargoItem();
	}

	if (!CargoItem)
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario C cargo update skipped: Station=%s Reason=NoCargoItem"), *GetName());
		return;
	}

	const int32 NextSequence = LastUpdateSequence + 1;
	const int32 NextStackCount = CargoItem->GetStackCount() <= 1 ? 6 : CargoItem->GetStackCount() - 1;
	const int32 NextDurability = CargoItem->GetDurability() <= 10 ? 100 : CargoItem->GetDurability() - 7;
	const int32 NextCharge = (CargoItem->GetCharge() + 17) % 101;
	const ERelayCargoItemState NextState = NextStackCount <= 1 ? ERelayCargoItemState::Reserved : ERelayCargoItemState::Stored;

	CargoItem->SetCargoState(NextStackCount, NextDurability, NextCharge, NextState);
	LastUpdateSequence = NextSequence;

	RefreshVisualState();
	LogCargoStationState(TEXT("updated"));
}

void ARelayCargoStationActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (CargoItem && IsUsingRegisteredSubObjectList())
	{
		RemoveReplicatedSubObject(CargoItem);
	}

	Super::EndPlay(EndPlayReason);
}

void ARelayCargoStationActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARelayCargoStationActor, StationId);
	DOREPLIFETIME(ARelayCargoStationActor, DebugName);
	DOREPLIFETIME(ARelayCargoStationActor, LastUpdateSequence);
	DOREPLIFETIME(ARelayCargoStationActor, CargoItem);
}

void ARelayCargoStationActor::OnRep_CargoItem()
{
	RefreshVisualState();
	LogCargoStationState(TEXT("cargo item pointer replicated"));
}

void ARelayCargoStationActor::OnRep_LastUpdateSequence()
{
	RefreshVisualState();
	LogCargoStationState(TEXT("replicated"));
}

void ARelayCargoStationActor::CreateCargoItem()
{
	if (!HasAuthority() || CargoItem)
	{
		return;
	}

	CargoItem = NewObject<URelayCargoItem>(this, URelayCargoItem::StaticClass(), TEXT("ScenarioC_CargoItem"));
	if (!CargoItem)
	{
		UE_LOG(LogIrisDemo, Warning, TEXT("Scenario C cargo item creation failed: Station=%s"), *GetName());
		return;
	}

	CargoItem->ConfigureItem(StationId, FName(TEXT("RelayMedKit")), 6, 100, 25);
	AddReplicatedSubObject(CargoItem);

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario C cargo item registered: Station=%s Item=%s RegisteredList=%s"),
		*GetName(),
		*GetNameSafe(CargoItem),
		IsUsingRegisteredSubObjectList() ? TEXT("true") : TEXT("false"));
}

void ARelayCargoStationActor::RefreshVisualState()
{
	if (StationLabel)
	{
		const FString ItemText = CargoItem
			? FString::Printf(TEXT("%s S%d D%d C%d"),
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
	UE_LOG(LogIrisDemo, Log, TEXT("Relay cargo station %s: StationId=%d DebugName=%s Sequence=%d CargoItem=%s Actor=%s"),
		Reason,
		StationId,
		*DebugName.ToString(),
		LastUpdateSequence,
		*GetNameSafe(CargoItem),
		*GetName());
}
