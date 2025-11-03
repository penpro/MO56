#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "MO56SaveTypes.generated.h"

USTRUCT(BlueprintType)
struct FMO56PlayerStats
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Player")
        FVector Location = FVector::ZeroVector;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Player")
        FRotator Rotation = FRotator::ZeroRotator;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Player")
        TMap<FName, float> Scalars;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Player")
        TArray<FName> LearnedSkills;
};

USTRUCT(BlueprintType)
struct FMO56WorldActorSaveData
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|World")
        FGuid ActorId;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|World")
        FString ActorPath;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|World")
        FTransform Transform = FTransform::Identity;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|World")
        TArray<uint8> SerializedData;
};

USTRUCT(BlueprintType)
struct FSaveIndexEntry
{
        GENERATED_BODY()

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Index")
        FGuid SaveId;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Index")
        FString SlotName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Index")
        FString LevelName;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Index")
        FDateTime UpdatedUtc;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Index")
        float TotalPlaySeconds = 0.f;

        UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Save|Index")
        FString ScreenshotPath;
};

inline constexpr int32 MO56_SAVE_VERSION = 1;

USTRUCT()
struct FPawnSaveData
{
        GENERATED_BODY()

        UPROPERTY()
        FGuid PawnId;

        UPROPERTY()
        TSoftClassPtr<APawn> ClassPath;

        UPROPERTY()
        FTransform Transform = FTransform::Identity;

        UPROPERTY()
        bool bPlayerCandidate = true;

        UPROPERTY()
        FGuid InventoryId;
};

USTRUCT()
struct FPlayerAssignment
{
        GENERATED_BODY()

        UPROPERTY()
        FGuid PlayerSaveId;

        UPROPERTY()
        FGuid PawnId;
};

