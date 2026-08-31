// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AIShowTarget : TargetRules
{
	public AIShowTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		bLegacyParentIncludePaths = false;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		CppStandard = CppStandardVersion.Default;
		bValidateFormatStrings = true;
		ExtraModuleNames.Add("AIShow");
	}
}
