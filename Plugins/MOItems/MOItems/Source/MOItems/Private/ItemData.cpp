#include "ItemData.h"

#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "PhysicsEngine/BodySetup.h"
#include "PhysicsEngine/PhysicsAsset.h"

namespace
{
        constexpr float KgToLbs = 2.20462262f;
        constexpr float Cm3ToM3 = 1.0e-6f;
}

float UItemData::GetWeightKg() const
{
        if (WeightKgOverride > 0.f)
        {
                return WeightKgOverride;
        }

        if (WeightLbsOverride > 0.f)
        {
                return WeightLbsOverride / KgToLbs;
        }

        return CalculateDefaultWeightKg();
}

float UItemData::GetWeightLbs() const
{
        if (WeightLbsOverride > 0.f)
        {
                return WeightLbsOverride;
        }

        return GetWeightKg() * KgToLbs;
}

float UItemData::GetVolumeCubicMeters() const
{
        if (VolumeOverride > 0.f)
        {
                return VolumeOverride;
        }

        return CalculateDefaultVolumeCubicMeters();
}

float UItemData::GetDensity() const
{
        if (DensityOverride > 0.f)
        {
                return DensityOverride;
        }

        const float Volume = GetVolumeCubicMeters();
        if (Volume <= KINDA_SMALL_NUMBER)
        {
                return 0.f;
        }

        return GetWeightKg() / Volume;
}

float UItemData::CalculateDefaultWeightKg() const
{
        if (UStaticMesh* StaticMesh = WorldStaticMesh.LoadSynchronous())
        {
                if (UBodySetup* BodySetup = StaticMesh->GetBodySetup())
                {
                        float Mass = 0.f;
#if WITH_EDITOR
                        Mass = BodySetup->CalculateMass(WorldScale3D);
#endif
                        if (Mass <= KINDA_SMALL_NUMBER)
                        {
                                Mass = BodySetup->MassInKg;
                                const float ScaleFactor = FMath::Abs(WorldScale3D.X * WorldScale3D.Y * WorldScale3D.Z);
                                if (ScaleFactor > KINDA_SMALL_NUMBER)
                                {
                                        Mass *= ScaleFactor;
                                }
                        }

                        if (Mass > KINDA_SMALL_NUMBER)
                        {
                                return Mass;
                        }
                }
        }

        if (USkeletalMesh* SkeletalMesh = WorldSkeletalMesh.LoadSynchronous())
        {
                if (UPhysicsAsset* PhysicsAsset = SkeletalMesh->GetPhysicsAsset())
                {
                        const float Mass = PhysicsAsset->CalculateMassInKg();
                        if (Mass > KINDA_SMALL_NUMBER)
                        {
                                return Mass;
                        }
                }
        }

        return 0.f;
}

float UItemData::CalculateDefaultVolumeCubicMeters() const
{
        float VolumeCm3 = 0.f;

        if (UStaticMesh* StaticMesh = WorldStaticMesh.LoadSynchronous())
        {
                if (UBodySetup* BodySetup = StaticMesh->GetBodySetup())
                {
                        VolumeCm3 = BodySetup->AggGeom.GetVolume(WorldScale3D);
                }

                if (VolumeCm3 <= 0.f)
                {
                        const FBoxSphereBounds Bounds = StaticMesh->GetBounds();
                        const FVector Extents = Bounds.BoxExtent * WorldScale3D * 2.f;
                        VolumeCm3 = Extents.X * Extents.Y * Extents.Z;
                }
        }

        if (VolumeCm3 <= 0.f)
        {
                if (USkeletalMesh* SkeletalMesh = WorldSkeletalMesh.LoadSynchronous())
                {
                        const FBoxSphereBounds Bounds = SkeletalMesh->GetBounds();
                        const FVector Extents = Bounds.BoxExtent * WorldScale3D * 2.f;
                        VolumeCm3 = Extents.X * Extents.Y * Extents.Z;
                }
        }

        return FMath::Max(0.f, VolumeCm3) * Cm3ToM3;
}
