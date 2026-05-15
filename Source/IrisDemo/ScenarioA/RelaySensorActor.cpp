// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioA/RelaySensorActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "IrisDemo.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ARelaySensorActor::ARelaySensorActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(2.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	SensorMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SensorMesh"));
	SensorMesh->SetupAttachment(SceneRoot);
	SensorMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SensorMesh->SetRelativeScale3D(FVector(0.65, 0.65, 0.35));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		SensorMesh->SetStaticMesh(CylinderMesh.Object);
	}

	SensorLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("SensorLabel"));
	SensorLabel->SetupAttachment(SceneRoot);
	SensorLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	SensorLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	SensorLabel->SetRelativeLocation(FVector(0.0, 0.0, 80.0));
	SensorLabel->SetRelativeRotation(FRotator(60.0, 0.0, 0.0));
	SensorLabel->SetTextRenderColor(FColor::Cyan);
	SensorLabel->SetWorldSize(36.0f);
	RefreshVisualState();
}

void ARelaySensorActor::ConfigureSensor(int32 NewSensorId, int32 NewZoneId, FName NewDebugName, bool bNewScenarioAEnabled)
{
	if (!HasAuthority())
	{
		return;
	}

	SensorId = NewSensorId;
	ZoneId = NewZoneId;
	InterestCategory = ERelayInterestCategory::SensorDetail;
	InterestDetailLevel = ERelayInterestDetailLevel::Detail;
	DebugName = NewDebugName;
	bScenarioAEnabled = bNewScenarioAEnabled;
	++LastUpdateSequence;

	RefreshVisualState();
	LogSensorState(TEXT("configured"));
}

void ARelaySensorActor::SetSensorState(int32 NewAlertLevel, bool bNewTriggered)
{
	if (!HasAuthority())
	{
		return;
	}

	AlertLevel = NewAlertLevel;
	bTriggered = bNewTriggered;
	++LastUpdateSequence;

	RefreshVisualState();
	LogSensorState(TEXT("updated"));
}

void ARelaySensorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARelaySensorActor, SensorId);
	DOREPLIFETIME(ARelaySensorActor, ZoneId);
	DOREPLIFETIME(ARelaySensorActor, InterestCategory);
	DOREPLIFETIME(ARelaySensorActor, InterestDetailLevel);
	DOREPLIFETIME(ARelaySensorActor, DebugName);
	DOREPLIFETIME(ARelaySensorActor, LastUpdateSequence);
	DOREPLIFETIME(ARelaySensorActor, bScenarioAEnabled);
	DOREPLIFETIME(ARelaySensorActor, AlertLevel);
	DOREPLIFETIME(ARelaySensorActor, bTriggered);
}

void ARelaySensorActor::OnRep_LastUpdateSequence()
{
	RefreshVisualState();
	LogSensorState(TEXT("replicated"));
}

void ARelaySensorActor::RefreshVisualState()
{
	if (SensorMesh)
	{
		const float HeightScale = 0.35f + static_cast<float>(AlertLevel) * 0.08f;
		SensorMesh->SetRelativeScale3D(FVector(0.65f, 0.65f, HeightScale));
		SensorMesh->SetVisibility(bScenarioAEnabled);
	}

	if (SensorLabel)
	{
		const FString LabelText = FString::Printf(
			TEXT("%s\nID %d | Z%d | A%d"),
			*DebugName.ToString(),
			SensorId,
			ZoneId,
			AlertLevel);
		SensorLabel->SetText(FText::FromString(LabelText));
		SensorLabel->SetTextRenderColor(bTriggered ? FColor::Red : FColor::Cyan);
		SensorLabel->SetVisibility(bScenarioAEnabled);
	}
}

void ARelaySensorActor::LogSensorState(const TCHAR* Reason) const
{
	UE_LOG(LogIrisDemo, Log, TEXT("Relay sensor %s: SensorId=%d DebugName=%s Zone=%d Category=%s DetailLevel=%s Sequence=%d Enabled=%s AlertLevel=%d Triggered=%s Actor=%s"),
		Reason,
		SensorId,
		*DebugName.ToString(),
		ZoneId,
		*StaticEnum<ERelayInterestCategory>()->GetNameStringByValue(static_cast<int64>(InterestCategory)),
		*StaticEnum<ERelayInterestDetailLevel>()->GetNameStringByValue(static_cast<int64>(InterestDetailLevel)),
		LastUpdateSequence,
		bScenarioAEnabled ? TEXT("true") : TEXT("false"),
		AlertLevel,
		bTriggered ? TEXT("true") : TEXT("false"),
		*GetName());
}
