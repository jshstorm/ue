// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FClientNetModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
