using UnrealBuildTool;

public class FPSGame : ModuleRules
{
	public FPSGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore",
			"PhysicsCore",
			"UMG",
			"Slate",
			"SlateCore",
			"NavigationSystem",
			"AIModule",
			"GameplayTasks",
			"NetworkReplayStreaming",
			"ReplicationGraph"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { 
			// Networking modules
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"Sockets",
			"NetCore",
			"PacketHandler",
			"ReliableUDP",
			
			// Audio modules
			"AudioMixer",
			"AudioExtensions",
			"SignalProcessing",
			"SoundUtilities",
			"AudioPlatformConfiguration",
			"AudioMixerCore",
			
			// Rendering and materials
			"RenderCore",
			"RHI",
			"MaterialShaderQualitySettings",
			"Renderer",
			
			// Utility modules
			"Json",
			"JsonUtilities",
			"HTTP",
			"ImageWrapper",
			"AssetRegistry",
			"DeveloperSettings",
			
			// Physics and destruction
			"Chaos",
			"ChaosSolverEngine",
			"FieldSystemEngine",
			"GeometryCollectionEngine",
			
			// Async and threading
			"Tasks",
			
			// Testing and automation
			"AutomationController",
			"UnrealEd",
			"ToolMenus",
			"EditorStyle",
			"EditorWidgets",
			"PropertyEditor",
			"Slate",
			"SlateCore",
			"EditorSubsystem",
			
			// Advanced object pooling requirements
			"CoreUObject",
			"ApplicationCore"
		});

		// Enable networking features
		PublicDefinitions.Add("WITH_NETWORKING=1");
		
		// Enable advanced audio features
		PublicDefinitions.Add("WITH_ADVANCED_AUDIO=1");
		
		// Enable destruction system
		PublicDefinitions.Add("WITH_DESTRUCTION_SYSTEM=1");
		
		// Enable advanced object pooling
		PublicDefinitions.Add("WITH_ADVANCED_OBJECT_POOLING=1");
		
		// Enable comprehensive testing
		PublicDefinitions.Add("WITH_ADVANCED_TESTING=1");
		
		// Optimization flags for performance
		if (Target.Configuration == UnrealTargetConfiguration.Shipping)
		{
			PublicDefinitions.Add("UE_BUILD_SHIPPING=1");
			PublicDefinitions.Add("WITH_POOL_DEBUGGING=0");
		}
		else
		{
			PublicDefinitions.Add("WITH_POOL_DEBUGGING=1");
		}

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
