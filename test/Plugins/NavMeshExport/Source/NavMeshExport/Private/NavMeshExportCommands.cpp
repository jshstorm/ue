// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "NavMeshExportCommands.h"

#define LOCTEXT_NAMESPACE "FNavMeshExportModule"

void FNavMeshExportCommands::RegisterCommands()
{
	UI_COMMAND(PluginAction, "NavMeshExport", "Execute NavMeshExport action", EUserInterfaceActionType::Button, FInputGesture());
}

#undef LOCTEXT_NAMESPACE
