using UnrealBuildTool;
using System.Collections.Generic;

public class FPSGameEditorTarget : TargetRules
{
	public FPSGameEditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "FPSGame" } );
	}
}
