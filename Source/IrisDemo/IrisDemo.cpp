// Copyright Epic Games, Inc. All Rights Reserved.

#include "IrisDemo.h"

#include "Engine/NetDriver.h"
#include "Engine/World.h"
#include "Iris/IrisConfig.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
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

FString SanitizeScenarioARunId(FString RunId)
{
	RunId.TrimStartAndEndInline();

	for (int32 Index = 0; Index < RunId.Len(); ++Index)
	{
		const TCHAR Character = RunId[Index];
		const bool bAllowed = FChar::IsAlnum(Character) || Character == TCHAR('_') || Character == TCHAR('-') || Character == TCHAR('.');
		if (!bAllowed)
		{
			RunId[Index] = TCHAR('_');
		}
	}

	return RunId;
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

IRISDEMO_API FString GetScenarioANetModeLabel(const UWorld* World)
{
	if (!World)
	{
		return TEXT("Unknown");
	}

	switch (World->GetNetMode())
	{
	case NM_Standalone:
		return TEXT("Standalone");
	case NM_DedicatedServer:
		return TEXT("DedicatedServer");
	case NM_ListenServer:
		return TEXT("ListenServer");
	case NM_Client:
		return TEXT("Client");
	default:
		return TEXT("Unknown");
	}
}

IRISDEMO_API FString MakeScenarioARunId(const FString& RawRunId)
{
	FString RunId = SanitizeScenarioARunId(RawRunId);
	if (!RunId.IsEmpty())
	{
		return RunId;
	}

	const FDateTime Now = FDateTime::Now();
	return FString::Printf(TEXT("ScenarioA_%s_%s"),
		*FString::Printf(TEXT("%s_%03d"), *Now.ToString(TEXT("%Y%m%d_%H%M%S")), Now.GetMillisecond()),
		*GetIrisReplicationModeLabel());
}

IRISDEMO_API FString GetScenarioARunId()
{
	FString RawRunId;
	FParse::Value(FCommandLine::Get(), TEXT("-ScenarioARunId="), RawRunId);
	return MakeScenarioARunId(RawRunId);
}

IRISDEMO_API FString GetScenarioARunDirectory(const FString& RunId)
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ScenarioA"), TEXT("Runs"), MakeScenarioARunId(RunId));
}
