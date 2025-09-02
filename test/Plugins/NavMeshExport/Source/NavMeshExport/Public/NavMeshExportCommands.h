// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Framework/Commands/Commands.h"
#include "NavMeshExportStyle.h"

class FNavMeshExportCommands : public TCommands<FNavMeshExportCommands>
{
public:

	FNavMeshExportCommands()
		: TCommands<FNavMeshExportCommands>(TEXT("NavMeshExport"), NSLOCTEXT("Contexts", "NavMeshExport", "NavMeshExport Plugin"), NAME_None, FNavMeshExportStyle::GetStyleSetName())
	{
	}

	// TCommands<> interface
	virtual void RegisterCommands() override;

public:
	TSharedPtr< FUICommandInfo > PluginAction;
};
