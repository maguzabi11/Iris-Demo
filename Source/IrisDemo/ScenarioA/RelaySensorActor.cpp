// Copyright Epic Games, Inc. All Rights Reserved.

#include "ScenarioA/RelaySensorActor.h"

#include "IrisDemo.h"
#include "Net/UnrealNetwork.h"

ARelaySensorActor::ARelaySensorActor()
{
	bReplicates = true;
	bAlwaysRelevant = true;
	SetNetUpdateFrequency(2.0f);
}

void ARelaySensorActor::ConfigureSensor(int32 NewZoneId, FName NewDebugName, bool bNewScenarioAEnabled)
{
	if (!HasAuthority())
	{
		return;
	}

	ZoneId = NewZoneId;
	DebugName = NewDebugName;
	bScenarioAEnabled = bNewScenarioAEnabled;
	++LastUpdateSequence;

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

	LogSensorState(TEXT("updated"));
}

void ARelaySensorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ARelaySensorActor, ZoneId);
	DOREPLIFETIME(ARelaySensorActor, DebugName);
	DOREPLIFETIME(ARelaySensorActor, LastUpdateSequence);
	DOREPLIFETIME(ARelaySensorActor, bScenarioAEnabled);
	DOREPLIFETIME(ARelaySensorActor, AlertLevel);
	DOREPLIFETIME(ARelaySensorActor, bTriggered);
}

void ARelaySensorActor::OnRep_LastUpdateSequence()
{
	LogSensorState(TEXT("replicated"));
}

void ARelaySensorActor::LogSensorState(const TCHAR* Reason) const
{
	UE_LOG(LogIrisDemo, Log, TEXT("Relay sensor %s: Actor=%s DebugName=%s Zone=%d Sequence=%d Enabled=%s AlertLevel=%d Triggered=%s"),
		Reason,
		*GetName(),
		*DebugName.ToString(),
		ZoneId,
		LastUpdateSequence,
		bScenarioAEnabled ? TEXT("true") : TEXT("false"),
		AlertLevel,
		bTriggered ? TEXT("true") : TEXT("false"));
}
