// Build rules — PREFLIGHT-VERIFY against the UE 5.8 install on primo.
using UnrealBuildTool;

public class ByHeart : ModuleRules
{
    public ByHeart(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
    }
}
