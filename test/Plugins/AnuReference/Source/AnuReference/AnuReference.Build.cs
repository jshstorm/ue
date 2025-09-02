// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

namespace UnrealBuildTool.Rules
{
    public class AnuReference : ModuleRules
    {
        public AnuReference(ReadOnlyTargetRules Target) : base(Target)
        {
            CppStandard = CppStandardVersion.Cpp17;
            PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
            bEnableUndefinedIdentifierWarnings = false;

            string workspaceDirectory = Path.Combine(ModuleDirectory, "../../../../../");
            PrivateIncludePaths.Add(Path.Combine(workspaceDirectory, "server/source/include"));
            PublicIncludePaths.Add(Path.Combine(workspaceDirectory, "server/source/include"));

            PublicDependencyModuleNames.AddRange(
                new string[] {
                "Core",
                "XmlParser",
                "Json",
				"AnuLevelDesign",
				"AIModule",
                }
            );

            if (Target.bBuildEditor == true)
            {
                //reference the module "MyModule"
                PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                "Projects",
                "InputCore",
                "UnrealEd",
                "LevelEditor",
                }
                );
            }

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "Paper2D",
                    // ... add private dependencies that you statically link with here ...
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

