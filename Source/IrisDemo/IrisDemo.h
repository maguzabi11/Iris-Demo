// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWorld;

/** Main log category used across the project */
DECLARE_LOG_CATEGORY_EXTERN(LogIrisDemo, Log, All);

/** Re-applies the Iris replication command line override after config CVars have loaded. */
IRISDEMO_API void ApplyIrisReplicationCommandLineOverride();

/** Returns whether the active world NetDriver, or fallback Iris preference, is using Iris replication. */
IRISDEMO_API bool IsUsingIrisReplication(const UWorld* World);

/** Returns the -UseIrisReplication command line override label (Iris / Generic / Default). */
IRISDEMO_API FString GetIrisReplicationCommandLineOverrideLabel();

/** Returns a human-readable label for the current Iris replication mode (Iris / Generic / Unknown). */
IRISDEMO_API FString GetIrisReplicationModeLabel();

/** Returns a human-readable label for the active world NetDriver replication mode (Iris / Generic / Unknown). */
IRISDEMO_API FString GetIrisReplicationModeLabel(const UWorld* World);
