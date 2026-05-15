// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioA/RelayDroneActor.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "IrisDemo.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

ARelayDroneActor::ARelayDroneActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(2.0f);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	DroneMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DroneMesh"));
	DroneMesh->SetupAttachment(SceneRoot);
	DroneMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DroneMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.18f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		DroneMesh->SetStaticMesh(CubeMesh.Object);
	}

	DroneLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DroneLabel"));
	DroneLabel->SetupAttachment(SceneRoot);
	DroneLabel->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
	DroneLabel->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
	DroneLabel->SetRelativeLocation(FVector(0.0, 0.0, 70.0));
	DroneLabel->SetRelativeRotation(FRotator(60.0, 0.0, 0.0));
	DroneLabel->SetTextRenderColor(FColor::Green);
	DroneLabel->SetWorldSize(30.0f);
	RefreshVisualState();
}

void ARelayDroneActor::ConfigureDrone(int32 NewDroneId, int32 NewZoneId, FName NewDebugName, bool bNewScenarioAEnabled)
{
	if (!HasAuthority())
	{
		return;
	}

	DroneId = NewDroneId;
	ZoneId = NewZoneId;
	DebugName = NewDebugName;
	bScenarioAEnabled = bNewScenarioAEnabled;
	++LastUpdateSequence;

	RefreshVisualState();
	LogDroneState(TEXT("configured"));
}

void ARelayDroneActor::SetDroneState(int32 NewBatteryPercent, EDroneRelayState NewDroneState)
{
	if (!HasAuthority())
	{
		return;
	}

	BatteryPercent = FMath::Clamp(NewBatteryPercent, 0, 100);
	DroneState = NewDroneState;
	++LastUpdateSequence;

	RefreshVisualState();
	LogDroneState(TEXT("updated"));
}

void ARelayDroneActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARelayDroneActor, DroneId);
	DOREPLIFETIME(ARelayDroneActor, ZoneId);
	DOREPLIFETIME(ARelayDroneActor, DebugName);
	DOREPLIFETIME(ARelayDroneActor, LastUpdateSequence);
	DOREPLIFETIME(ARelayDroneActor, bScenarioAEnabled);
	DOREPLIFETIME(ARelayDroneActor, BatteryPercent);
	DOREPLIFETIME(ARelayDroneActor, DroneState);
}

void ARelayDroneActor::OnRep_LastUpdateSequence()
{
	RefreshVisualState();
	LogDroneState(TEXT("replicated"));
}

void ARelayDroneActor::RefreshVisualState()
{
	if (DroneMesh)
	{
		const float BatteryScale = FMath::Max(0.25f, static_cast<float>(BatteryPercent) / 100.0f);
		DroneMesh->SetRelativeScale3D(FVector(0.55f, 0.55f, 0.12f + BatteryScale * 0.16f));
		DroneMesh->SetVisibility(bScenarioAEnabled);
	}

	if (DroneLabel)
	{
		const FString LabelText = FString::Printf(
			TEXT("%s\nID %d | Z%d | %d%%\n%s"),
			*DebugName.ToString(),
			DroneId,
			ZoneId,
			BatteryPercent,
			LexToString(DroneState));
		DroneLabel->SetText(FText::FromString(LabelText));
		DroneLabel->SetTextRenderColor(DroneState == EDroneRelayState::Disabled ? FColor::Red : FColor::Green);
		DroneLabel->SetVisibility(bScenarioAEnabled);
	}
}

void ARelayDroneActor::LogDroneState(const TCHAR* Reason) const
{
	UE_LOG(LogIrisDemo, Log, TEXT("Relay drone %s: DroneId=%d DebugName=%s Zone=%d Sequence=%d Enabled=%s Battery=%d State=%s Actor=%s"),
		Reason,
		DroneId,
		*DebugName.ToString(),
		ZoneId,
		LastUpdateSequence,
		bScenarioAEnabled ? TEXT("true") : TEXT("false"),
		BatteryPercent,
		LexToString(DroneState),
		*GetName());
}

const TCHAR* ARelayDroneActor::LexToString(EDroneRelayState State)
{
	switch (State)
	{
	case EDroneRelayState::Patrol:
		return TEXT("Patrol");
	case EDroneRelayState::Investigating:
		return TEXT("Investigating");
	case EDroneRelayState::Returning:
		return TEXT("Returning");
	case EDroneRelayState::Disabled:
		return TEXT("Disabled");
	default:
		return TEXT("Unknown");
	}
}
