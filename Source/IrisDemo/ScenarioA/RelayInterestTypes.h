// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RelayInterestTypes.generated.h"

UENUM(BlueprintType)
enum class ERelayInterestCategory : uint8
{
	SensorDetail,
	DroneDetail,
	SupplyDetail,
	OperationalSummary
};

UENUM(BlueprintType)
enum class ERelayInterestDetailLevel : uint8
{
	Summary,
	Detail
};
