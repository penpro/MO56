#include "Skills/InspectableComponent.h"

#include "Skills/SkillSystemComponent.h"

void UInspectableComponent::BuildInspectionParams(USkillSystemComponent* SkillSystem, TArray<FSkillInspectionParams>& OutParams) const
{
        OutParams.Reset();

        if (!SkillSystem)
        {
                return;
        }

        for (const FInspectionDiscovery& Discovery : Discoveries)
        {
                if (Discovery.KnowledgeTag.IsNone())
                {
                        continue;
                }

                if (Discovery.bOncePerActor && SkillSystem->HasCompletedInspectionForSource(this, Discovery.KnowledgeTag))
                {
                        continue;
                }

                FSkillInspectionParams Params;
                Params.KnowledgeId = Discovery.KnowledgeTag;
                Params.KnowledgeGain = Discovery.KnowledgeGain;
                Params.Duration = Discovery.Duration;
                Params.bOncePerSource = Discovery.bOncePerActor;
                Params.Description = Discovery.Description;

                const ESkillDomain Domain = SkillDefinitions::ResolveSkillDomainFromTag(Discovery.SkillTag);
                if (Domain != ESkillDomain::None && Discovery.SkillXP > 0.f)
                {
                        Params.SkillXpRewards.Add(Domain, Discovery.SkillXP);
                }

                if (Params.Duration <= 0.f)
                {
                        Params.Duration = 20.f;
                }

                if (!Params.Description.IsEmpty())
                {
                        Params.Description = Discovery.Description;
                }

                OutParams.Add(MoveTemp(Params));
        }
}
