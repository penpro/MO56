#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MO56DebugLogSubsystem.generated.h"

class UGameInstance;

USTRUCT(BlueprintType)
struct FMO56DebugEvent
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FDateTime Timestamp = FDateTime::UtcNow();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FName System = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FName Action = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FString Detail;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FGuid PlayerId;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
    FGuid PawnId;
};

DECLARE_LOG_CATEGORY_EXTERN(LogMO56Debug, Log, All);

UCLASS()
class MO56_API UMO56DebugLogSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    UMO56DebugLogSubsystem();

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    void LogEvent(FName System, FName Action, const FString& Detail, const FGuid& PlayerId = FGuid(), const FGuid& PawnId = FGuid());
    void FlushToDisk();

    void SetEnabled(bool bInEnabled);
    void SetJsonEnabled(bool bInJsonEnabled);

    bool IsEnabled() const { return bEnabled; }
    bool IsJsonEnabled() const { return bJsonEnabled; }

    static UMO56DebugLogSubsystem* Get(const UObject* WorldContextObject);

private:
    void HandleFlushCommand();
    void RefreshCVarState();
    void EnsureDirectoryCreated();
    FString BuildLogLine(const FMO56DebugEvent& Event) const;
    FString BuildJsonLine(const FMO56DebugEvent& Event) const;
    FString BuildTextLine(const FMO56DebugEvent& Event) const;

    void AppendEventsToFile(const TArray<FMO56DebugEvent>& EventsToWrite);
    FString ResolveSessionFilePath() const;

private:
    mutable FCriticalSection BufferMutex;
    TArray<FMO56DebugEvent> BufferedEvents;
    int32 MaxBufferedEvents = 128;

    bool bEnabled = false;
    bool bJsonEnabled = false;
    bool bCommandLineJson = false;

    FString SessionFileName;
    FString SessionDirectory;
    FDateTime SessionStartUtc;

    static void RegisterConsoleDelegates();
    static void UnregisterConsoleDelegates();
    static void ForEachActiveSubsystem(TFunctionRef<void(UMO56DebugLogSubsystem&)> Callback);
    static void HandleEnableChanged(IConsoleVariable* Var);
    static void HandleJsonChanged(IConsoleVariable* Var);
    static void HandleFlushCommandStatic();

    static TSet<TWeakObjectPtr<UMO56DebugLogSubsystem>> ActiveSubsystems;
    static bool bConsoleDelegatesRegistered;
};

FGuid MO56ResolvePawnId(const APawn* Pawn);
FGuid MO56ResolveCharacterId(const ACharacter* Character);

#define MO56_DEBUG(System, Action, Detail, PlayerId, PawnId) \
    if (UMO56DebugLogSubsystem* LogSubsystem = UMO56DebugLogSubsystem::Get(this)) \
    { \
        if (LogSubsystem->IsEnabled()) \
        { \
            LogSubsystem->LogEvent(System, Action, Detail, PlayerId, PawnId); \
        } \
    }

