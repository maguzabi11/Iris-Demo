// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogIrisDemo, Log, All);

/** Returns a human-readable label for the current Iris replication mode (Iris / Generic / Unknown). */
IRISDEMO_API FString GetIrisReplicationModeLabel();