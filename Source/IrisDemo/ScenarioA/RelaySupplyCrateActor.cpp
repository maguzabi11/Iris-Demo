// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioA/RelaySupplyCrateActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "IrisDemo.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ARelaySupplyCrateActor::ARelaySupplyCrateActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(2.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	CrateMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CrateMesh"));
	CrateMesh->SetupAttachment(SceneRoot);
	CrateMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CrateMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.35f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		CrateMesh->SetStaticMesh(CubeMesh.Object);
	}

	CrateLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("CrateLabel"));
	CrateLabel->SetupAttachment(SceneRoot);
	CrateLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	CrateLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	CrateLabel->SetRelativeLocation(FVector(0.0, 0.0, 75.0));
	CrateLabel->SetRelativeRotation(FRotator(60.0, 0.0, 0.0));
	CrateLabel->SetTextRenderColor(FColor::Yellow);
	CrateLabel->SetWorldSize(30.0f);
	RefreshVisualState();
}

void ARelaySupplyCrateActor::ConfigureCrate(int32 NewCrateId, int32 NewZoneId, int32 NewOwningSquadId, FName NewDebugName, bool bNewScenarioAEnabled)
{
	if (!HasAuthority())
	{
		return;
	}

	CrateId = NewCrateId;
	ZoneId = NewZoneId;
	OwningSquadId = NewOwningSquadId;
	InterestCategory = ERelayInterestCategory::SupplyDetail;
	InterestDetailLevel = ERelayInterestDetailLevel::Detail;
	DebugName = NewDebugName;
	bScenarioAEnabled = bNewScenarioAEnabled;
	++LastUpdateSequence;

	RefreshVisualState();
	LogCrateState(TEXT("configured"));
}

void ARelaySupplyCrateActor::SetCrateState(int32 NewStockCount, bool bNewReserved)
{
	if (!HasAuthority())
	{
		return;
	}

	StockCount = FMath::Max(0, NewStockCount);
	bReserved = bNewReserved;
	++LastUpdateSequence;

	RefreshVisualState();
	LogCrateState(TEXT("updated"));
}

void ARelaySupplyCrateActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARelaySupplyCrateActor, CrateId);
	DOREPLIFETIME(ARelaySupplyCrateActor, ZoneId);
	DOREPLIFETIME(ARelaySupplyCrateActor, OwningSquadId);
	DOREPLIFETIME(ARelaySupplyCrateActor, InterestCategory);
	DOREPLIFETIME(ARelaySupplyCrateActor, InterestDetailLevel);
	DOREPLIFETIME(ARelaySupplyCrateActor, DebugName);
	DOREPLIFETIME(ARelaySupplyCrateActor, LastUpdateSequence);
	DOREPLIFETIME(ARelaySupplyCrateActor, bScenarioAEnabled);
	DOREPLIFETIME(ARelaySupplyCrateActor, StockCount);
	DOREPLIFETIME(ARelaySupplyCrateActor, bReserved);
}

void ARelaySupplyCrateActor::OnRep_LastUpdateSequence()
{
	RefreshVisualState();
	LogCrateState(TEXT("replicated"));
}

void ARelaySupplyCrateActor::RefreshVisualState()
{
	if (CrateMesh)
	{
		const float StockScale = 0.25f + static_cast<float>(FMath::Min(StockCount, 20)) / 40.0f;
		CrateMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, StockScale));
		CrateMesh->SetVisibility(bScenarioAEnabled);
	}

	if (CrateLabel)
	{
		const FString LabelText = FString::Printf(
			TEXT("%s\nID %d | Z%d | S%d\nStock %d%s"),
			*DebugName.ToString(),
			CrateId,
			ZoneId,
			OwningSquadId,
			StockCount,
			bReserved ? TEXT(" | Reserved") : TEXT(""));
		CrateLabel->SetText(FText::FromString(LabelText));
		CrateLabel->SetTextRenderColor(bReserved ? FColor::Orange : FColor::Yellow);
		CrateLabel->SetVisibility(bScenarioAEnabled);
	}
}

void ARelaySupplyCrateActor::LogCrateState(const TCHAR* Reason) const
{
	UE_LOG(LogIrisDemo, Log, TEXT("Relay supply crate %s: CrateId=%d DebugName=%s Zone=%d Squad=%d Category=%s DetailLevel=%s Sequence=%d Enabled=%s Stock=%d Reserved=%s Actor=%s"),
		Reason,
		CrateId,
		*DebugName.ToString(),
		ZoneId,
		OwningSquadId,
		*StaticEnum<ERelayInterestCategory>()->GetNameStringByValue(static_cast<int64>(InterestCategory)),
		*StaticEnum<ERelayInterestDetailLevel>()->GetNameStringByValue(static_cast<int64>(InterestDetailLevel)),
		LastUpdateSequence,
		bScenarioAEnabled ? TEXT("true") : TEXT("false"),
		StockCount,
		bReserved ? TEXT("true") : TEXT("false"),
		*GetName());
}
