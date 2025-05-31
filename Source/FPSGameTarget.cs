using UnrealBuildTool;
using System.Collections.Generic;

public class FPSGameTarget : TargetRules
{
	public FPSGameTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V2;
		ExtraModuleNames.AddRange( new string[] { "FPSGame" } );
	}
}
