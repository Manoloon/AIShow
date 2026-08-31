// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AIShowEditorTarget : TargetRules
{
	public AIShowEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		bLegacyParentIncludePaths = false;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppStandard = CppStandardVersion.Default;
		bValidateFormatStrings = true;
		ExtraModuleNames.Add("AIShow");
	}
}
