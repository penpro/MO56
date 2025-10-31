#include "Skills/SkillTypes.h"

#include "Containers/Array.h"
#include "Internationalization/Text.h"
#include "Templates/UnrealTemplate.h"

namespace
{
        const TArray<FSkillDomainInfo>& BuildDomainInfo()
        {
                static const TArray<FSkillDomainInfo> DomainInfos = {
                        FSkillDomainInfo(ESkillDomain::Naturalist, FName(TEXT("Skill.Naturalist")), NSLOCTEXT("Skills", "Naturalist", "Naturalist")),
                        FSkillDomainInfo(ESkillDomain::Foraging, FName(TEXT("Skill.Foraging")), NSLOCTEXT("Skills", "Foraging", "Foraging")),
                        FSkillDomainInfo(ESkillDomain::Cordage, FName(TEXT("Skill.Cordage")), NSLOCTEXT("Skills", "Cordage", "Cordage")),
                        FSkillDomainInfo(ESkillDomain::Knapping, FName(TEXT("Skill.Knapping")), NSLOCTEXT("Skills", "Knapping", "Knapping")),
                        FSkillDomainInfo(ESkillDomain::Firecraft, FName(TEXT("Skill.Firecraft")), NSLOCTEXT("Skills", "Firecraft", "Firecraft")),
                        FSkillDomainInfo(ESkillDomain::Woodworking, FName(TEXT("Skill.Woodworking")), NSLOCTEXT("Skills", "Woodworking", "Woodworking")),
                        FSkillDomainInfo(ESkillDomain::Toolbinding, FName(TEXT("Skill.Toolbinding")), NSLOCTEXT("Skills", "Toolbinding", "Toolbinding")),
                        FSkillDomainInfo(ESkillDomain::Watercrafting, FName(TEXT("Skill.Watercrafting")), NSLOCTEXT("Skills", "Watercrafting", "Watercrafting")),
                        FSkillDomainInfo(ESkillDomain::Sheltercraft, FName(TEXT("Skill.Sheltercraft")), NSLOCTEXT("Skills", "Sheltercraft", "Sheltercraft")),
                        FSkillDomainInfo(ESkillDomain::Weaving, FName(TEXT("Skill.Weaving")), NSLOCTEXT("Skills", "Weaving", "Weaving")),
                        FSkillDomainInfo(ESkillDomain::Tanning, FName(TEXT("Skill.Tanning")), NSLOCTEXT("Skills", "Tanning", "Tanning/Softening")),
                        FSkillDomainInfo(ESkillDomain::TrappingFishing, FName(TEXT("Skill.TrappingFishing")), NSLOCTEXT("Skills", "TrappingFishing", "Trapping & Fishing")),
                        FSkillDomainInfo(ESkillDomain::Cooking, FName(TEXT("Skill.Cooking")), NSLOCTEXT("Skills", "Cooking", "Cooking & Preservation")),
                        FSkillDomainInfo(ESkillDomain::Clayworking, FName(TEXT("Skill.Clayworking")), NSLOCTEXT("Skills", "Clayworking", "Clayworking"))
                };

                return DomainInfos;
        }

        const TArray<FKnowledgeInfo>& BuildKnowledgeInfo()
        {
                static const TArray<FKnowledgeInfo> KnowledgeInfos = {
                        FKnowledgeInfo(FName(TEXT("Knowledge.PlantFibers")), NSLOCTEXT("Skills", "Knowledge_PlantFibers", "Plant Fibers"), FName(TEXT("Skill.Naturalist"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.BarkStrips")), NSLOCTEXT("Skills", "Knowledge_BarkStrips", "Bark Strips"), FName(TEXT("Skill.Naturalist"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.FlexibleBindings")), NSLOCTEXT("Skills", "Knowledge_FlexibleBindings", "Flexible Bindings"), FName(TEXT("Skill.Cordage"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.PlantCordage")), NSLOCTEXT("Skills", "Knowledge_PlantCordage", "Cordage Recipes"), FName(TEXT("Skill.Cordage"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.TinderTypes")), NSLOCTEXT("Skills", "Knowledge_TinderTypes", "Tinder Types"), FName(TEXT("Skill.Firecraft"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Fireboard")), NSLOCTEXT("Skills", "Knowledge_Fireboard", "Fireboard & Spindle"), FName(TEXT("Skill.Firecraft"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.EmberCare")), NSLOCTEXT("Skills", "Knowledge_EmberCare", "Ember Care"), FName(TEXT("Skill.Firecraft"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.ClayHandling")), NSLOCTEXT("Skills", "Knowledge_ClayHandling", "Clay Handling"), FName(TEXT("Skill.Clayworking"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.CoilPot")), NSLOCTEXT("Skills", "Knowledge_CoilPot", "Coil Pot Construction"), FName(TEXT("Skill.Clayworking"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.FlintKnapping")), NSLOCTEXT("Skills", "Knowledge_FlintKnapping", "Knapping Basics"), FName(TEXT("Skill.Knapping"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Basketry")), NSLOCTEXT("Skills", "Knowledge_Basketry", "Basketry"), FName(TEXT("Skill.Weaving"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Thatch")), NSLOCTEXT("Skills", "Knowledge_Thatch", "Thatching"), FName(TEXT("Skill.Weaving"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.WaterFilters")), NSLOCTEXT("Skills", "Knowledge_WaterFilters", "Water Filters"), FName(TEXT("Skill.Watercrafting"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.BoilStones")), NSLOCTEXT("Skills", "Knowledge_BoilStones", "Boil Stones"), FName(TEXT("Skill.Firecraft"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.PitchGlue")), NSLOCTEXT("Skills", "Knowledge_PitchGlue", "Pitch Glue"), FName(TEXT("Skill.Toolbinding"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Hafting")), NSLOCTEXT("Skills", "Knowledge_Hafting", "Hafting"), FName(TEXT("Skill.Toolbinding"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Snares")), NSLOCTEXT("Skills", "Knowledge_Snares", "Simple Snares"), FName(TEXT("Skill.TrappingFishing"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.FishTraps")), NSLOCTEXT("Skills", "Knowledge_FishTraps", "Fish Traps"), FName(TEXT("Skill.TrappingFishing"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.FireCooking")), NSLOCTEXT("Skills", "Knowledge_FireCooking", "Fire Cooking"), FName(TEXT("Skill.Cooking"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Drying")), NSLOCTEXT("Skills", "Knowledge_Drying", "Drying & Preservation"), FName(TEXT("Skill.Cooking"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.HideSoftening")), NSLOCTEXT("Skills", "Knowledge_HideSoftening", "Hide Softening"), FName(TEXT("Skill.Tanning"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.ShelterFrames")), NSLOCTEXT("Skills", "Knowledge_ShelterFrames", "Shelter Frames"), FName(TEXT("Skill.Sheltercraft"))),
                        FKnowledgeInfo(FName(TEXT("Knowledge.WovenMats")), NSLOCTEXT("Skills", "Knowledge_WovenMats", "Woven Mats"), FName(TEXT("Skill.Weaving")))
                };

                return KnowledgeInfos;
        }
}

const TArray<FSkillDomainInfo>& SkillDefinitions::GetSkillDomains()
{
        return BuildDomainInfo();
}

const TArray<FKnowledgeInfo>& SkillDefinitions::GetKnowledgeDefinitions()
{
        return BuildKnowledgeInfo();
}

FName SkillDefinitions::GetSkillDomainTag(ESkillDomain Domain)
{
        if (Domain == ESkillDomain::None)
        {
                return NAME_None;
        }

        const TArray<FSkillDomainInfo>& Infos = BuildDomainInfo();
        for (const FSkillDomainInfo& Info : Infos)
        {
                if (Info.Domain == Domain)
                {
                        return Info.Tag;
                }
        }

        return NAME_None;
}

ESkillDomain SkillDefinitions::ResolveSkillDomainFromTag(const FName& Tag)
{
        if (Tag.IsNone())
        {
                return ESkillDomain::None;
        }

        const TArray<FSkillDomainInfo>& Infos = BuildDomainInfo();
        for (const FSkillDomainInfo& Info : Infos)
        {
                if (Info.Tag == Tag || Info.Tag.GetPlainNameString().Equals(Tag.ToString(), ESearchCase::IgnoreCase))
                {
                        return Info.Domain;
                }
        }

        return ESkillDomain::None;
}

FText SkillDefinitions::GetKnowledgeDisplayName(const FName& KnowledgeId)
{
        if (KnowledgeId.IsNone())
        {
                return FText::GetEmpty();
        }

        const TArray<FKnowledgeInfo>& Infos = BuildKnowledgeInfo();
        for (const FKnowledgeInfo& Info : Infos)
        {
                if (Info.KnowledgeId == KnowledgeId)
                {
                        return Info.DisplayName;
                }
        }

        return FText::FromName(KnowledgeId);
}
