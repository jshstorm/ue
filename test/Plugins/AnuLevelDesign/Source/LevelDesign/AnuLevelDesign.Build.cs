// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class AnuLevelDesign : ModuleRules
{
    public AnuLevelDesign(ReadOnlyTargetRules Target) : base(Target)
    {
        CppStandard = CppStandardVersion.Cpp17;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.AddRange(
			new string[] {	
				Path.Combine(ModuleDirectory, "Public"),
			}
		);

		PrivateIncludePaths.AddRange(
			new string[] {
				Path.Combine(ModuleDirectory, "Private"),
				"../../../../server/source/include",
			}
		);

		PublicDependencyModuleNames.AddRange(
            new string[] {
                "Core",
                "CoreUObject",
                "Engine",
                "InputCore",
                "UMG",
				"Json",
				"JsonUtilities",
				"ClientNet"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "Slate",
                "SlateCore",
                "Projects",
                "APPFRAMEWORK"
            }
        );
	}
}