// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
    public class ClientNet : ModuleRules
    {
        private string ThirdPartyPath
        {
            get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../ThirdParty/")); }
        }

        public ClientNet(ReadOnlyTargetRules Target) : base(Target)
        {
			DefaultBuildSettings = BuildSettingsVersion.V5;
			CppStandard = CppStandardVersion.Cpp20;
			PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

            PublicIncludePaths.AddRange(
                new string[] {
                }
            );

            PrivateIncludePaths.AddRange(
                new string[] {
                    "../../../../server/source/include"
                }
            );

            PublicDependencyModuleNames.AddRange(
                new string[]
                {
					"WebSockets"
				}
            );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                "Core",
                "CoreUObject",
                "Engine",
                "Networking",
                "Sockets",
				"HTTP",
                "Json",
                "JsonUtilities",
                "OpenSSL",
                }
            );

            DynamicallyLoadedModuleNames.AddRange(
                new string[]
                {
					// ... add any modules that your module loads dynamically here ...
				}
            );

        }
    }
}
