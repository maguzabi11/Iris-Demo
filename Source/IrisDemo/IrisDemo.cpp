// Copyright Epic Games, Inc. All Rights Reserved.

#include "IrisDemo.h"
#include "HAL/IConsoleManager.h"
#include "Modules/ModuleManager.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, IrisDemo, "IrisDemo" );

DEFINE_LOG_CATEGORY(LogIrisDemo)

IRISDEMO_API FString GetIrisReplicationModeLabel()
{
	const IConsoleVariable* UseIrisCVar = IConsoleManager::Get().FindConsoleVariable(TEXT("net.Iris.UseIrisReplication"));
	if (!UseIrisCVar)
	{
		return TEXT("Unknown");
	}

	return UseIrisCVar->GetInt() != 0 ? TEXT("Iris") : TEXT("Generic");
}