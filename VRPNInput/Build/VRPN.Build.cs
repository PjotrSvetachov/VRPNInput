
using UnrealBuildTool;
using System;
using System.IO;

public class VRPN : ModuleRules
{
    public VRPN(TargetInfo Target)
	{
		Type = ModuleType.External;

        string basePath = Path.GetDirectoryName(RulesCompiler.GetModuleFilename(this.GetType().Name));
        
        PublicSystemIncludePaths.Add(Path.Combine(basePath, "include"));

        if (Target.Platform == UnrealTargetPlatform.Win64)
		{
            PublicLibraryPaths.Add(Path.Combine(basePath, "lib", "win64"));
            PublicAdditionalLibraries.Add("vrpn.lib");
		}
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            PublicLibraryPaths.Add(Path.Combine(basePath, "lib", "linux"));
            PublicAdditionalLibraries.Add(Path.Combine(basePath, "lib", "linux", "vrpn.a"));
            PublicAdditionalLibraries.Add(Path.Combine(basePath, "lib", "linux", "quat.a"));
        }
	}
}
