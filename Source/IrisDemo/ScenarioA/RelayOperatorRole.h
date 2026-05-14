// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RelayOperatorRole.generated.h"

UENUM(BlueprintType)
enum class ERelayOperatorRole : uint8
{
	Unassigned UMETA(DisplayName = "Unassigned"),
	Commander UMETA(DisplayName = "Commander"),
	FieldAgent UMETA(DisplayName = "Field Agent"),
	Spectator UMETA(DisplayName = "Spectator")
};
