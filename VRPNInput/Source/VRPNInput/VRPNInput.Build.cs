using System.IO;

namespace UnrealBuildTool.Rules
{
	public class VRPNInput : ModuleRules
	{
		private string ThirdPartyPath
		{
			get { return Path.GetFullPath(Path.Combine(ModuleDirectory, "../ThirdParty/" )); }
		}

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
					"SlateCore"
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

			string vrpnPath = Path.Combine(ThirdPartyPath, "VRPN");
			PublicIncludePaths.Add(Path.Combine(vrpnPath, "Include"));

			if (Target.Platform == UnrealTargetPlatform.Win64)
			{
				PublicLibraryPaths.Add(Path.Combine(vrpnPath, "Lib", "Win64"));
				PublicAdditionalLibraries.Add("vrpn.lib");
				PublicAdditionalLibraries.Add("quat.lib");
			}
			else if (Target.Platform == UnrealTargetPlatform.Linux)
			{
				PublicLibraryPaths.Add(Path.Combine(vrpnPath, "Lib", "Linux"));
				PublicAdditionalLibraries.Add(Path.Combine(vrpnPath, "Lib", "Linux", "vrpn.a"));
				PublicAdditionalLibraries.Add(Path.Combine(vrpnPath, "Lib", "Linux", "quat.a"));
			}
			else if (Target.Platform == UnrealTargetPlatform.Mac)
			{
				PublicLibraryPaths.Add(Path.Combine(vrpnPath, "Lib", "Mac"));
				PublicAdditionalLibraries.Add(Path.Combine(vrpnPath, "Lib", "Mac", "libvrpn.a"));
				PublicAdditionalLibraries.Add(Path.Combine(vrpnPath, "Lib", "Mac", "quat.a"));
			}
		}
	}
}
