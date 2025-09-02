// Copyright 2017 CLOVERGAMES Co., Ltd. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class AnuLevelDesignEditor : ModuleRules
{
    public AnuLevelDesignEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        CppStandard = CppStandardVersion.Cpp17;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Private"));

        PublicDependencyModuleNames.AddRange(
		    new string[]
		    {
  			    "Core",
			    "CoreUObject",
                "Engine",
				"UnrealEd",
                "Slate",
				"EditorWidgets",
                "UnrealEd",
			    "KismetWidgets",
                "GraphEditor",
				"Json",
				"JsonUtilities",
				"Kismet"
            }
		);
			
		PrivateDependencyModuleNames.AddRange(
		    new string[]
		    {
                "InputCore",
                "SlateCore",
                "PropertyEditor",
			    "EditorStyle",
			    "ContentBrowser",
			    "Projects",
                "ApplicationCore",
                "ToolMenus",
                "AnuLevelDesign"
            }
        );
	}
}
