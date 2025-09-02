// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class AnuDataTableEditor : ModuleRules
{
	public AnuDataTableEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		CppStandard = CppStandardVersion.Cpp17;
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
		);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"AnuDataTableEditor/Private",
				"AnuDataTableEditor/Private/AssetTools",
				"AnuDataTableEditor/Private/Factories",
				// ... add other private include paths required here ...
			}
		);
		
		string workspaceDirectory = Path.Combine(ModuleDirectory, "../../../../../");
        PrivateIncludePaths.Add(Path.Combine(workspaceDirectory, "server/source/include"));
		
		PublicIncludePathModuleNames.Add("LevelEditor");
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"UnrealEd",
				"PropertyEditor",
				"ToolMenus",
				"AssetTools",
				"DataTableEditor",
				"AnuDataTable",
				"AnuReference",
				"Anu"
				// ... add other public dependencies that you statically link with here ...
			}
		);
		
		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"ApplicationCore",
				"Engine",
                "InputCore",
				"Slate",
				"SlateCore",
                "EditorStyle",
                "PropertyEditor",
				"UnrealEd",
				"Json",
				"AnuDataTable",
				"DataTableEditor"
				// ... add private dependencies that you statically link with here ...	
			}
		);
		
		PrivateIncludePathModuleNames.AddRange(
            new string[] 
			{
                "Media",
                "UnrealEd",
                "WorkspaceMenuStructure",
				"DesktopPlatform",
				"ToolMenus",
				"DataTableEditor"
            }
        );
		
		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				"DesktopPlatform",
				"WorkspaceMenuStructure",
				//"DataTableEditor"
				// ... add any modules that your module loads dynamically here ...
			}
		);
	}
}
