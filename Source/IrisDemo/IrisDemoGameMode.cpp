// Copyright Epic Games, Inc. All Rights Reserved.

#include "IrisDemoGameMode.h"

#include "GameFramework/PlayerController.h"
#include "IrisDemo.h"
#include "ScenarioA/RelayDroneActor.h"
#include "ScenarioA/RelayPlayerState.h"
#include "ScenarioA/RelaySensorActor.h"
#include "TimerManager.h"

AIrisDemoGameMode::AIrisDemoGameMode()
{
	EnsureScenarioAPlayerStateClass();
	ScenarioASensorClass = ARelaySensorActor::StaticClass();
	ScenarioADroneClass = ARelayDroneActor::StaticClass();
}

void AIrisDemoGameMode::BeginPlay()
{
	Super::BeginPlay();

	SpawnScenarioASensors();
	SpawnScenarioADrones();

	if ((ScenarioASensors.Num() > 0 || ScenarioADrones.Num() > 0) && ScenarioASensorUpdateInterval > 0.0f)
	{
		GetWorldTimerManager().SetTimer(
			ScenarioASensorUpdateTimerHandle,
			this,
			&AIrisDemoGameMode::UpdateScenarioASensors,
			ScenarioASensorUpdateInterval,
			true);
	}
}

void AIrisDemoGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(ScenarioASensorUpdateTimerHandle);

	Super::EndPlay(EndPlayReason);
}

void AIrisDemoGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	EnsureScenarioAPlayerStateClass();

	Super::InitGame(MapName, Options, ErrorMessage);
}

void AIrisDemoGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	if (!NewPlayer)
	{
		return;
	}

	if (ARelayPlayerState* RelayPlayerState = NewPlayer->GetPlayerState<ARelayPlayerState>())
	{
		RelayPlayerState->SetOperatorRole(GetNextScenarioARole());
		return;
	}

	UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A role assignment skipped: PlayerStateClass=%s Player=%s"),
		PlayerStateClass ? *PlayerStateClass->GetName() : TEXT("None"),
		*NewPlayer->GetName());
}

void AIrisDemoGameMode::EnsureScenarioAPlayerStateClass()
{
	if (!PlayerStateClass || !PlayerStateClass->IsChildOf(ARelayPlayerState::StaticClass()))
	{
		PlayerStateClass = ARelayPlayerState::StaticClass();
	}
}

ERelayOperatorRole AIrisDemoGameMode::GetNextScenarioARole()
{
	static constexpr ERelayOperatorRole RoleOrder[] = {
		ERelayOperatorRole::Commander,
		ERelayOperatorRole::FieldAgent,
		ERelayOperatorRole::Spectator
	};

	const ERelayOperatorRole AssignedRole = RoleOrder[NextScenarioARoleIndex % UE_ARRAY_COUNT(RoleOrder)];
	++NextScenarioARoleIndex;

	return AssignedRole;
}

void AIrisDemoGameMode::SpawnScenarioASensors()
{
	if (!HasAuthority() || !ScenarioASensorClass || ScenarioASensorCount <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ScenarioASensors.Reset();

	for (int32 SensorIndex = 0; SensorIndex < ScenarioASensorCount; ++SensorIndex)
	{
		const int32 ZoneId = SensorIndex % 3;
		const FVector SpawnLocation(
			static_cast<double>(ZoneId) * 450.0,
			static_cast<double>(SensorIndex / 3) * 350.0,
			120.0);
		const FRotator SpawnRotation = FRotator::ZeroRotator;

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ARelaySensorActor* Sensor = World->SpawnActor<ARelaySensorActor>(
			ScenarioASensorClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParameters);

		if (!Sensor)
		{
			UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A sensor spawn failed: Index=%d Zone=%d"), SensorIndex, ZoneId);
			continue;
		}

		Sensor->ConfigureSensor(SensorIndex, ZoneId, FName(*FString::Printf(TEXT("Sensor_%02d"), SensorIndex)), true);
		Sensor->SetSensorState(ZoneId + 1, SensorIndex == 0);
		ScenarioASensors.Add(Sensor);
	}

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A sensors spawned: Count=%d"), ScenarioASensors.Num());
}

void AIrisDemoGameMode::UpdateScenarioASensors()
{
	if (!HasAuthority())
	{
		return;
	}

	for (int32 SensorIndex = 0; SensorIndex < ScenarioASensors.Num(); ++SensorIndex)
	{
		ARelaySensorActor* Sensor = ScenarioASensors[SensorIndex];
		if (!IsValid(Sensor) || !Sensor->IsScenarioAEnabled())
		{
			continue;
		}

		const int32 NextAlertLevel = (Sensor->GetAlertLevel() + 1 + SensorIndex) % 5;
		const bool bNextTriggered = NextAlertLevel >= 3;
		Sensor->SetSensorState(NextAlertLevel, bNextTriggered);
	}

	UpdateScenarioADrones();
}

void AIrisDemoGameMode::SpawnScenarioADrones()
{
	if (!HasAuthority() || !ScenarioADroneClass || ScenarioADroneCount <= 0)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	ScenarioADrones.Reset();

	for (int32 DroneIndex = 0; DroneIndex < ScenarioADroneCount; ++DroneIndex)
	{
		const int32 ZoneId = DroneIndex % 3;
		const FVector SpawnLocation(
			static_cast<double>(ZoneId) * 450.0,
			-300.0,
			260.0 + static_cast<double>(DroneIndex) * 30.0);
		const FRotator SpawnRotation(0.0, 45.0 * static_cast<double>(DroneIndex), 0.0);

		FActorSpawnParameters SpawnParameters;
		SpawnParameters.Owner = this;
		SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		ARelayDroneActor* Drone = World->SpawnActor<ARelayDroneActor>(
			ScenarioADroneClass,
			SpawnLocation,
			SpawnRotation,
			SpawnParameters);

		if (!Drone)
		{
			UE_LOG(LogIrisDemo, Warning, TEXT("Scenario A drone spawn failed: Index=%d Zone=%d"), DroneIndex, ZoneId);
			continue;
		}

		Drone->ConfigureDrone(DroneIndex, ZoneId, FName(*FString::Printf(TEXT("Drone_%02d"), DroneIndex)), true);
		Drone->SetDroneState(100 - DroneIndex * 15, EDroneRelayState::Patrol);
		ScenarioADrones.Add(Drone);
	}

	UE_LOG(LogIrisDemo, Log, TEXT("Scenario A drones spawned: Count=%d"), ScenarioADrones.Num());
}

void AIrisDemoGameMode::UpdateScenarioADrones()
{
	if (!HasAuthority())
	{
		return;
	}

	for (int32 DroneIndex = 0; DroneIndex < ScenarioADrones.Num(); ++DroneIndex)
	{
		ARelayDroneActor* Drone = ScenarioADrones[DroneIndex];
		if (!IsValid(Drone) || !Drone->IsScenarioAEnabled())
		{
			continue;
		}

		const int32 NextBattery = (Drone->GetBatteryPercent() <= 10) ? 100 : Drone->GetBatteryPercent() - (5 + DroneIndex);
		EDroneRelayState NextState = EDroneRelayState::Patrol;
		if (NextBattery <= 15)
		{
			NextState = EDroneRelayState::Disabled;
		}
		else if (NextBattery <= 35)
		{
			NextState = EDroneRelayState::Returning;
		}
		else if ((DroneIndex + NextBattery) % 3 == 0)
		{
			NextState = EDroneRelayState::Investigating;
		}

		Drone->SetDroneState(NextBattery, NextState);
	}
}
