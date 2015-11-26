namespace UnrealBuildTool.Rules
{
	public class VRPNInput : ModuleRules
	{
        public VRPNInput(TargetInfo Target)
		{
			PublicIncludePaths.AddRange(
				new string[] {
					// ... add public include paths required here ...
				}
				);

			PrivateIncludePaths.AddRange(
				new string[] {
					"VRPNInput/Private",
					// ... add other private include paths required here ...
				}
				);
			PublicDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
                    "CoreUObject",
                    "Engine",
                    "InputCore",
                    "InputDevice",
                    "Slate",
                    "SlateCore",
                    "VRPN"
					// ... add other public dependencies that you statically link with here ...
				}
				);

            PrivateIncludePathModuleNames.AddRange(
                new string[]
                {
                    "HeadMountedDisplay"	// For IMotionController.h
				}
                );

            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
					// ... add private dependencies that you statically link with here ...
				}
                );

			DynamicallyLoadedModuleNames.AddRange(
				new string[]
				{
					// ... add any modules that your module loads dynamically here ...
				}
				);

            // Add the standard configuration as a dependancy
            if (Target.Platform == UnrealTargetPlatform.Win64 || Target.Platform == UnrealTargetPlatform.Win32)
            {
                string EngineConfigFile = "$(EngineDir)/Plugins/HPCV/VRPNInput/Config/VRPNConfig.ini";
                RuntimeDependencies.Add(new RuntimeDependency(EngineConfigFile));
            }
        }
	}
}