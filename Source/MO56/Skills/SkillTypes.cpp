#include "Skills/SkillTypes.h"

#include "Containers/Array.h"
#include "Internationalization/Text.h"
#include "Templates/UnrealTemplate.h"

namespace
{
        const TArray<FSkillDomainInfo>& BuildDomainInfo()
        {
                static const TArray<FSkillDomainInfo> DomainInfos = {
                        FSkillDomainInfo(ESkillDomain::Naturalist, FName(TEXT("Skill.Naturalist")), NSLOCTEXT("Skills", "Naturalist", "Naturalist"),
                                NSLOCTEXT("Skills", "NaturalistHistory", "Interpreting the landscape through plants, animals, and seasonal patterns as taught by ancestral guides."),
                                NSLOCTEXT("Skills", "NaturalistTips", "Survey new biomes, catalog specimens, and share observations to deepen this discipline."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Foraging, FName(TEXT("Skill.Foraging")), NSLOCTEXT("Skills", "Foraging", "Foraging"),
                                NSLOCTEXT("Skills", "ForagingHistory", "Gathering edible and medicinal resources with the care used by woodland caretakers."),
                                NSLOCTEXT("Skills", "ForagingTips", "Harvest responsibly, study plant cycles, and experiment with recipes to progress."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Cordage, FName(TEXT("Skill.Cordage")), NSLOCTEXT("Skills", "Cordage", "Cordage"),
                                NSLOCTEXT("Skills", "CordageHistory", "Twisting fibers into rope just as river nomads prepared lines and nets."),
                                NSLOCTEXT("Skills", "CordageTips", "Process fresh fibers, practice new braids, and refine tension to gain ranks."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Knapping, FName(TEXT("Skill.Knapping")), NSLOCTEXT("Skills", "Knapping", "Knapping"),
                                NSLOCTEXT("Skills", "KnappingHistory", "Shaping stone edges following techniques passed down along campfires."),
                                NSLOCTEXT("Skills", "KnappingTips", "Collect varied cores, strike deliberately, and study failed flakes to improve."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Firecraft, FName(TEXT("Skill.Firecraft")), NSLOCTEXT("Skills", "Firecraft", "Firecraft"),
                                NSLOCTEXT("Skills", "FirecraftHistory", "Maintaining embers and kindling traditions refined by countless hearth keepers."),
                                NSLOCTEXT("Skills", "FirecraftTips", "Experiment with tinder, practice drills, and safeguard coals to progress."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Woodworking, FName(TEXT("Skill.Woodworking")), NSLOCTEXT("Skills", "Woodworking", "Woodworking"),
                                NSLOCTEXT("Skills", "WoodworkingHistory", "Carving and joining timber as artisans shaped shelters and tools from the forest."),
                                NSLOCTEXT("Skills", "WoodworkingTips", "Shape beams, plane surfaces, and assemble frames to raise proficiency."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Toolbinding, FName(TEXT("Skill.Toolbinding")), NSLOCTEXT("Skills", "Toolbinding", "Toolbinding"),
                                NSLOCTEXT("Skills", "ToolbindingHistory", "Securing blades to handles using gums, fibers, and pitch known to primitive engineers."),
                                NSLOCTEXT("Skills", "ToolbindingTips", "Experiment with adhesives, try new lashings, and repair worn gear to advance."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Watercrafting, FName(TEXT("Skill.Watercrafting")), NSLOCTEXT("Skills", "Watercrafting", "Watercrafting"),
                                NSLOCTEXT("Skills", "WatercraftingHistory", "Constructing vessels and devices that let travelers master lakes and rivers."),
                                NSLOCTEXT("Skills", "WatercraftingTips", "Patch hulls, lash rafts, and sail in varied waters to strengthen this art."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Sheltercraft, FName(TEXT("Skill.Sheltercraft")), NSLOCTEXT("Skills", "Sheltercraft", "Sheltercraft"),
                                NSLOCTEXT("Skills", "SheltercraftHistory", "Raising frames, thatching roofs, and shaping camps like traveling bands once did."),
                                NSLOCTEXT("Skills", "SheltercraftTips", "Erect structures in harsher climates, reinforce joints, and insulate walls to grow."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Weaving, FName(TEXT("Skill.Weaving")), NSLOCTEXT("Skills", "Weaving", "Weaving"),
                                NSLOCTEXT("Skills", "WeavingHistory", "Plaiting fibers into mats, baskets, and garments inspired by ancient looms."),
                                NSLOCTEXT("Skills", "WeavingTips", "Collect diverse strands, attempt complex patterns, and dye fibers to advance."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Tanning, FName(TEXT("Skill.Tanning")), NSLOCTEXT("Skills", "Tanning", "Tanning/Softening"),
                                NSLOCTEXT("Skills", "TanningHistory", "Curing hides with smoke, oil, and patience like skilled hideworkers."),
                                NSLOCTEXT("Skills", "TanningTips", "Stretch skins, manage curing racks, and experiment with new solutions to improve."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::TrappingFishing, FName(TEXT("Skill.TrappingFishing")), NSLOCTEXT("Skills", "TrappingFishing", "Trapping & Fishing"),
                                NSLOCTEXT("Skills", "TrappingFishingHistory", "Setting lines, weirs, and snares echoing coastal and woodland survival tactics."),
                                NSLOCTEXT("Skills", "TrappingFishingTips", "Deploy traps in fresh habitats, monitor bait, and refine trigger designs."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Cooking, FName(TEXT("Skill.Cooking")), NSLOCTEXT("Skills", "Cooking", "Cooking & Preservation"),
                                NSLOCTEXT("Skills", "CookingHistory", "Preparing meals and preserving harvests the way hearth keepers fed their clans."),
                                NSLOCTEXT("Skills", "CookingTips", "Combine ingredients, experiment with smoking, and manage drying racks to advance."),
                                TSoftObjectPtr<UTexture2D>()),
                        FSkillDomainInfo(ESkillDomain::Clayworking, FName(TEXT("Skill.Clayworking")), NSLOCTEXT("Skills", "Clayworking", "Clayworking"),
                                NSLOCTEXT("Skills", "ClayworkingHistory", "Forming vessels and kilns as river settlements perfected centuries ago."),
                                NSLOCTEXT("Skills", "ClayworkingTips", "Gather new clays, refine forms, and fire pieces carefully to improve."),
                                TSoftObjectPtr<UTexture2D>())
                };

                return DomainInfos;
        }

        const TArray<FKnowledgeInfo>& BuildKnowledgeInfo()
        {
                static const TArray<FKnowledgeInfo> KnowledgeInfos = {
                        FKnowledgeInfo(FName(TEXT("Knowledge.PlantFibers")), NSLOCTEXT("Skills", "Knowledge_PlantFibers", "Plant Fibers"), FName(TEXT("Skill.Naturalist")),
                                NSLOCTEXT("Skills", "Knowledge_PlantFibers_History", "Harvesting fibrous stems for twine as forest dwellers once did."),
                                NSLOCTEXT("Skills", "Knowledge_PlantFibers_Tips", "Collect seasonal plants and strip fibers to expand this knowledge."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.BarkStrips")), NSLOCTEXT("Skills", "Knowledge_BarkStrips", "Bark Strips"), FName(TEXT("Skill.Naturalist")),
                                NSLOCTEXT("Skills", "Knowledge_BarkStrips_History", "Peeling inner bark for cordage following riverbank traditions."),
                                NSLOCTEXT("Skills", "Knowledge_BarkStrips_Tips", "Score trunks carefully and soak bark to gain proficiency."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.FlexibleBindings")), NSLOCTEXT("Skills", "Knowledge_FlexibleBindings", "Flexible Bindings"), FName(TEXT("Skill.Cordage")),
                                NSLOCTEXT("Skills", "Knowledge_FlexibleBindings_History", "Looping lashings that kept shelters standing through storms."),
                                NSLOCTEXT("Skills", "Knowledge_FlexibleBindings_Tips", "Practice knots on new materials to advance."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.PlantCordage")), NSLOCTEXT("Skills", "Knowledge_PlantCordage", "Cordage Recipes"), FName(TEXT("Skill.Cordage")),
                                NSLOCTEXT("Skills", "Knowledge_PlantCordage_History", "Combining roots and grasses into braided line."),
                                NSLOCTEXT("Skills", "Knowledge_PlantCordage_Tips", "Test different plies and tension to improve."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.TinderTypes")), NSLOCTEXT("Skills", "Knowledge_TinderTypes", "Tinder Types"), FName(TEXT("Skill.Firecraft")),
                                NSLOCTEXT("Skills", "Knowledge_TinderTypes_History", "Cataloging fluff, shavings, and fungus prized by fire keepers."),
                                NSLOCTEXT("Skills", "Knowledge_TinderTypes_Tips", "Collect diverse tinder and test spark catching to grow."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Fireboard")), NSLOCTEXT("Skills", "Knowledge_Fireboard", "Fireboard & Spindle"), FName(TEXT("Skill.Firecraft")),
                                NSLOCTEXT("Skills", "Knowledge_Fireboard_History", "Carving fire sets refined by countless camps."),
                                NSLOCTEXT("Skills", "Knowledge_Fireboard_Tips", "Craft new boards, tune notches, and practice drills."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.EmberCare")), NSLOCTEXT("Skills", "Knowledge_EmberCare", "Ember Care"), FName(TEXT("Skill.Firecraft")),
                                NSLOCTEXT("Skills", "Knowledge_EmberCare_History", "Banking coals to transport warmth between settlements."),
                                NSLOCTEXT("Skills", "Knowledge_EmberCare_Tips", "Maintain ember beds and experiment with carriers to advance."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.ClayHandling")), NSLOCTEXT("Skills", "Knowledge_ClayHandling", "Clay Handling"), FName(TEXT("Skill.Clayworking")),
                                NSLOCTEXT("Skills", "Knowledge_ClayHandling_History", "Wedging and tempering clays taught in river villages."),
                                NSLOCTEXT("Skills", "Knowledge_ClayHandling_Tips", "Source varied clays and remove impurities to progress."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.CoilPot")), NSLOCTEXT("Skills", "Knowledge_CoilPot", "Coil Pot Construction"), FName(TEXT("Skill.Clayworking")),
                                NSLOCTEXT("Skills", "Knowledge_CoilPot_History", "Stacking coils for durable vessels in communal kilns."),
                                NSLOCTEXT("Skills", "Knowledge_CoilPot_Tips", "Shape taller pots and refine finishing tools to advance."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.FlintKnapping")), NSLOCTEXT("Skills", "Knowledge_FlintKnapping", "Knapping Basics"), FName(TEXT("Skill.Knapping")),
                                NSLOCTEXT("Skills", "Knowledge_FlintKnapping_History", "Reading stone grain like master knappers."),
                                NSLOCTEXT("Skills", "Knowledge_FlintKnapping_Tips", "Practice percussion and pressure flaking with varied stones."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Basketry")), NSLOCTEXT("Skills", "Knowledge_Basketry", "Basketry"), FName(TEXT("Skill.Weaving")),
                                NSLOCTEXT("Skills", "Knowledge_Basketry_History", "Weaving containers to gather and store staples."),
                                NSLOCTEXT("Skills", "Knowledge_Basketry_Tips", "Alternate patterns and rim finishes to improve."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Thatch")), NSLOCTEXT("Skills", "Knowledge_Thatch", "Thatching"), FName(TEXT("Skill.Weaving")),
                                NSLOCTEXT("Skills", "Knowledge_Thatch_History", "Layering grasses for shelter roofs that shed storms."),
                                NSLOCTEXT("Skills", "Knowledge_Thatch_Tips", "Harvest long stalks and practice dense overlaps."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.WaterFilters")), NSLOCTEXT("Skills", "Knowledge_WaterFilters", "Water Filters"), FName(TEXT("Skill.Watercrafting")),
                                NSLOCTEXT("Skills", "Knowledge_WaterFilters_History", "Filtering river water through sand, charcoal, and fiber."),
                                NSLOCTEXT("Skills", "Knowledge_WaterFilters_Tips", "Assemble multi-stage filters and test flow to progress."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.BoilStones")), NSLOCTEXT("Skills", "Knowledge_BoilStones", "Boil Stones"), FName(TEXT("Skill.Firecraft")),
                                NSLOCTEXT("Skills", "Knowledge_BoilStones_History", "Heating rocks for cooking when vessels could not meet flame."),
                                NSLOCTEXT("Skills", "Knowledge_BoilStones_Tips", "Cycle stones through fires and water safely to grow."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.PitchGlue")), NSLOCTEXT("Skills", "Knowledge_PitchGlue", "Pitch Glue"), FName(TEXT("Skill.Toolbinding")),
                                NSLOCTEXT("Skills", "Knowledge_PitchGlue_History", "Rendering resins into binding glue."),
                                NSLOCTEXT("Skills", "Knowledge_PitchGlue_Tips", "Collect sap and mix additives to strengthen adhesive."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Hafting")), NSLOCTEXT("Skills", "Knowledge_Hafting", "Hafting"), FName(TEXT("Skill.Toolbinding")),
                                NSLOCTEXT("Skills", "Knowledge_Hafting_History", "Setting blades securely on handles for daily labor."),
                                NSLOCTEXT("Skills", "Knowledge_Hafting_Tips", "Shape sockets, wedge pegs, and test strikes to advance."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Snares")), NSLOCTEXT("Skills", "Knowledge_Snares", "Simple Snares"), FName(TEXT("Skill.TrappingFishing")),
                                NSLOCTEXT("Skills", "Knowledge_Snares_History", "Setting ground loops inspired by woodland hunters."),
                                NSLOCTEXT("Skills", "Knowledge_Snares_Tips", "Place snares on new trails and refine triggers to progress."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.FishTraps")), NSLOCTEXT("Skills", "Knowledge_FishTraps", "Fish Traps"), FName(TEXT("Skill.TrappingFishing")),
                                NSLOCTEXT("Skills", "Knowledge_FishTraps_History", "Braiding traps used along tidal flats and creeks."),
                                NSLOCTEXT("Skills", "Knowledge_FishTraps_Tips", "Experiment with funnel shapes and bait placements to advance."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.FireCooking")), NSLOCTEXT("Skills", "Knowledge_FireCooking", "Fire Cooking"), FName(TEXT("Skill.Cooking")),
                                NSLOCTEXT("Skills", "Knowledge_FireCooking_History", "Rotating roasts and tending coals for communal meals."),
                                NSLOCTEXT("Skills", "Knowledge_FireCooking_Tips", "Try new seasoning, manage heat, and cook varied meals."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.Drying")), NSLOCTEXT("Skills", "Knowledge_Drying", "Drying & Preservation"), FName(TEXT("Skill.Cooking")),
                                NSLOCTEXT("Skills", "Knowledge_Drying_History", "Preserving harvests using racks, smoke, and sun."),
                                NSLOCTEXT("Skills", "Knowledge_Drying_Tips", "Build drying frames and monitor weather to progress."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.HideSoftening")), NSLOCTEXT("Skills", "Knowledge_HideSoftening", "Hide Softening"), FName(TEXT("Skill.Tanning")),
                                NSLOCTEXT("Skills", "Knowledge_HideSoftening_History", "Working hides over stakes with oils and smoke."),
                                NSLOCTEXT("Skills", "Knowledge_HideSoftening_Tips", "Scrape evenly, stretch continuously, and finish hides to grow."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.ShelterFrames")), NSLOCTEXT("Skills", "Knowledge_ShelterFrames", "Shelter Frames"), FName(TEXT("Skill.Sheltercraft")),
                                NSLOCTEXT("Skills", "Knowledge_ShelterFrames_History", "Lashing sturdy ribs for seasonal dwellings."),
                                NSLOCTEXT("Skills", "Knowledge_ShelterFrames_Tips", "Raise new frame styles and reinforce joints to progress."),
                                TSoftObjectPtr<UTexture2D>()),
                        FKnowledgeInfo(FName(TEXT("Knowledge.WovenMats")), NSLOCTEXT("Skills", "Knowledge_WovenMats", "Woven Mats"), FName(TEXT("Skill.Weaving")),
                                NSLOCTEXT("Skills", "Knowledge_WovenMats_History", "Creating mats for bedding, walls, and drying racks."),
                                NSLOCTEXT("Skills", "Knowledge_WovenMats_Tips", "Combine reeds and pattern designs to advance."),
                                TSoftObjectPtr<UTexture2D>())
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

        if (const FKnowledgeInfo* Info = FindKnowledgeInfo(KnowledgeId))
        {
                return Info->DisplayName;
        }

        return FText::FromName(KnowledgeId);
}

const FSkillDomainInfo* SkillDefinitions::FindDomainInfo(ESkillDomain Domain)
{
        const TArray<FSkillDomainInfo>& Infos = BuildDomainInfo();
        for (const FSkillDomainInfo& Info : Infos)
        {
                if (Info.Domain == Domain)
                {
                        return &Info;
                }
        }

        return nullptr;
}

const FKnowledgeInfo* SkillDefinitions::FindKnowledgeInfo(const FName& KnowledgeId)
{
        if (KnowledgeId.IsNone())
        {
                return nullptr;
        }

        const TArray<FKnowledgeInfo>& Infos = BuildKnowledgeInfo();
        for (const FKnowledgeInfo& Info : Infos)
        {
                if (Info.KnowledgeId == KnowledgeId)
                {
                        return &Info;
                }
        }

        return nullptr;
}

FText SkillDefinitions::BuildRankText(float Value)
{
        const float ClampedValue = FMath::Clamp(Value, 0.f, 100.f);

        if (ClampedValue >= 100.f)
        {
                return NSLOCTEXT("Skills", "Rank_Master", "Master");
        }
        if (ClampedValue >= 75.f)
        {
                return NSLOCTEXT("Skills", "Rank_Expert", "Expert");
        }
        if (ClampedValue >= 50.f)
        {
                return NSLOCTEXT("Skills", "Rank_Journeyman", "Journeyman");
        }
        if (ClampedValue >= 25.f)
        {
                return NSLOCTEXT("Skills", "Rank_Apprentice", "Apprentice");
        }

        return NSLOCTEXT("Skills", "Rank_Novice", "Novice");
}
