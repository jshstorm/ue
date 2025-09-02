// Copyright 2018 CLOVERGAMES Co., Ltd. All Rights Reserved.

#include "ClientNetPlugin.h"
#include "Runtime/Online/Websockets/Public/WebSocketsModule.h"

#define LOCTEXT_NAMESPACE "FClientNetworkModule"

void FClientNetModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FWebSocketsModule& module = FModuleManager::LoadModuleChecked<FWebSocketsModule>(TEXT("WebSockets"));
}

void FClientNetModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FClientNetModule, ClientNet)
