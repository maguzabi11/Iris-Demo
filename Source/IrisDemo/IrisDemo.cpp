// Copyright Epic Games, Inc. All Rights Reserved.

#include "IrisDemo.h"

#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Iris/IrisConfig.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, IrisDemo, "IrisDemo" );

DEFINE_LOG_CATEGORY(LogIrisDemo)

namespace
{
FString GetReplicationSystemLabel(EReplicationSystem ReplicationSystem)
{
	switch (ReplicationSystem)
	{
	case EReplicationSystem::Iris:
		return TEXT("Iris");
	case EReplicationSystem::Generic:
		return TEXT("Generic");
	case EReplicationSystem::Default:
	default:
		return TEXT("Default");
	}
}

EReplicationSystem GetFallbackReplicationSystem()
{
	const EReplicationSystem CommandLineOverride = UE::Net::GetUseIrisReplicationCmdlineValue();
	if (CommandLineOverride != EReplicationSystem::Default)
	{
		return CommandLineOverride;
	}

	return UE::Net::ShouldUseIrisReplication() ? EReplicationSystem::Iris : EReplicationSystem::Generic;
}
}

IRISDEMO_API void ApplyIrisReplicationCommandLineOverride()
{
	const EReplicationSystem CommandLineOverride = UE::Net::GetUseIrisReplicationCmdlineValue();
	if (CommandLineOverride == EReplicationSystem::Iris)
	{
		UE::Net::SetUseIrisReplication(true);
	}
	else if (CommandLineOverride == EReplicationSystem::Generic)
	{
		UE::Net::SetUseIrisReplication(false);
	}
}

IRISDEMO_API bool IsUsingIrisReplication(const UWorld* World)
{
	if (World)
	{
		if (const UNetDriver* NetDriver = World->GetNetDriver())
		{
			return NetDriver->IsUsingIrisReplication();
		}
	}

	return GetFallbackReplicationSystem() == EReplicationSystem::Iris;
}

IRISDEMO_API FString GetIrisReplicationCommandLineOverrideLabel()
{
	return GetReplicationSystemLabel(UE::Net::GetUseIrisReplicationCmdlineValue());
}

IRISDEMO_API FString GetIrisReplicationModeLabel()
{
	return GetReplicationSystemLabel(GetFallbackReplicationSystem());
}

IRISDEMO_API FString GetIrisReplicationModeLabel(const UWorld* World)
{
	if (World)
	{
		if (const UNetDriver* NetDriver = World->GetNetDriver())
		{
			return NetDriver->IsUsingIrisReplication() ? TEXT("Iris") : TEXT("Generic");
		}
	}

	return GetIrisReplicationModeLabel();
}
