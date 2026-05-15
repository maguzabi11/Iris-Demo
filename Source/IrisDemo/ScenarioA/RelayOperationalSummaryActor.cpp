// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioA/RelayOperationalSummaryActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "IrisDemo.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ARelayOperationalSummaryActor::ARelayOperationalSummaryActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(1.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SummaryMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SummaryMesh"));
	SummaryMesh->SetupAttachment(SceneRoot);
	SummaryMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SummaryMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, 0.18f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		SummaryMesh->SetStaticMesh(SphereMesh.Object);
	}

	SummaryLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SummaryLabel"));
	SummaryLabel->SetupAttachment(SceneRoot);
	SummaryLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	SummaryLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	SummaryLabel->SetRelativeLocation(FVector(0.0, 0.0, 85.0));
	SummaryLabel->SetRelativeRotation(FRotator(60.0, 0.0, 0.0));
	SummaryLabel->SetTextRenderColor(FColor::White);
	SummaryLabel->SetWorldSize(34.0f);
	RefreshVisualState();
}

void ARelayOperationalSummaryActor::ConfigureSummary(int32 NewSummaryId, FName NewDebugName, bool bNewScenarioAEnabled)
{
	if (!HasAuthority())
	{
		return;
	}

	SummaryId = NewSummaryId;
	InterestCategory = ERelayInterestCategory::OperationalSummary;
	InterestDetailLevel = ERelayInterestDetailLevel::Summary;
	DebugName = NewDebugName;
	bScenarioAEnabled = bNewScenarioAEnabled;
	++LastUpdateSequence;

	RefreshVisualState();
	LogSummaryState(TEXT("configured"));
}

void ARelayOperationalSummaryActor::SetSummaryState(int32 NewKnownAlertCount, int32 NewKnownDroneCount, int32 NewKnownSupplyCount)
{
	if (!HasAuthority())
	{
		return;
	}

	KnownAlertCount = FMath::Max(0, NewKnownAlertCount);
	KnownDroneCount = FMath::Max(0, NewKnownDroneCount);
	KnownSupplyCount = FMath::Max(0, NewKnownSupplyCount);
	++LastUpdateSequence;

	RefreshVisualState();
	LogSummaryState(TEXT("updated"));
}

void ARelayOperationalSummaryActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARelayOperationalSummaryActor, SummaryId);
	DOREPLIFETIME(ARelayOperationalSummaryActor, InterestCategory);
	DOREPLIFETIME(ARelayOperationalSummaryActor, InterestDetailLevel);
	DOREPLIFETIME(ARelayOperationalSummaryActor, DebugName);
	DOREPLIFETIME(ARelayOperationalSummaryActor, LastUpdateSequence);
	DOREPLIFETIME(ARelayOperationalSummaryActor, bScenarioAEnabled);
	DOREPLIFETIME(ARelayOperationalSummaryActor, KnownAlertCount);
	DOREPLIFETIME(ARelayOperationalSummaryActor, KnownDroneCount);
	DOREPLIFETIME(ARelayOperationalSummaryActor, KnownSupplyCount);
}

void ARelayOperationalSummaryActor::OnRep_LastUpdateSequence()
{
	RefreshVisualState();
	LogSummaryState(TEXT("replicated"));
}

void ARelayOperationalSummaryActor::RefreshVisualState()
{
	if (SummaryMesh)
	{
		const float AlertScale = 0.18f + static_cast<float>(FMath::Min(KnownAlertCount, 12)) * 0.025f;
		SummaryMesh->SetRelativeScale3D(FVector(0.8f, 0.8f, AlertScale));
		SummaryMesh->SetVisibility(bScenarioAEnabled);
	}

	if (SummaryLabel)
	{
		const FString LabelText = FString::Printf(
			TEXT("%s\nID %d | Alerts %d\nDrones %d | Supply %d"),
			*DebugName.ToString(),
			SummaryId,
			KnownAlertCount,
			KnownDroneCount,
			KnownSupplyCount);
		SummaryLabel->SetText(FText::FromString(LabelText));
		SummaryLabel->SetTextRenderColor(KnownAlertCount > 0 ? FColor::Magenta : FColor::White);
		SummaryLabel->SetVisibility(bScenarioAEnabled);
	}
}

void ARelayOperationalSummaryActor::LogSummaryState(const TCHAR* Reason) const
{
	UE_LOG(LogIrisDemo, Log, TEXT("Relay summary %s: SummaryId=%d DebugName=%s Category=%s DetailLevel=%s Sequence=%d Enabled=%s Alerts=%d Drones=%d Supply=%d Actor=%s"),
		Reason,
		SummaryId,
		*DebugName.ToString(),
		*StaticEnum<ERelayInterestCategory>()->GetNameStringByValue(static_cast<int64>(InterestCategory)),
		*StaticEnum<ERelayInterestDetailLevel>()->GetNameStringByValue(static_cast<int64>(InterestDetailLevel)),
		LastUpdateSequence,
		bScenarioAEnabled ? TEXT("true") : TEXT("false"),
		KnownAlertCount,
		KnownDroneCount,
		KnownSupplyCount,
		*GetName());
}
