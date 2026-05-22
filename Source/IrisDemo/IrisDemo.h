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

/** Returns a human-readable label for the active world net mode (Standalone / DedicatedServer / ListenServer / Client / Unknown). */
IRISDEMO_API FString GetScenarioANetModeLabel(const UWorld* World);

/** Returns a filesystem-safe Scenario A run id. Empty input generates one from local time and replication mode. */
IRISDEMO_API FString MakeScenarioARunId(const FString& RawRunId);

/** Returns the Scenario A run id from -ScenarioARunId, or generates a local fallback. */
IRISDEMO_API FString GetScenarioARunId();

/** Returns Saved/ScenarioA/Runs/<RunId>. */
IRISDEMO_API FString GetScenarioARunDirectory(const FString& RunId);
