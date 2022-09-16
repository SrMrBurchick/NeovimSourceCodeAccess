namespace UnrealBuildTool.Rules
{
	public class NeovimSourceCodeAccess : ModuleRules
	{

        public NeovimSourceCodeAccess(ReadOnlyTargetRules Target) : base(Target)
		{
            PrivateDependencyModuleNames.AddRange(
				new string[]
				{
					"Core",
					"CoreUObject",
					"Engine",
					"SourceCodeAccess",
					"DesktopPlatform",
					"DeveloperSettings",
				}
			);
		}
	}
}
