using UnrealBuildTool;

public class NullAction : ModuleRules
{
	public NullAction(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets code include "Character/NACharacterBase.h" etc. from any subfolder.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"NetCore",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AnimGraphRuntime"
		});
	}
}
