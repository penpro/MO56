#include "MO56DebugLogSubsystem.h"

#include "Components/MOPersistentPawnComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Paths.h"
#include "Misc/ScopeLock.h"
#include "MO56Character.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Subsystems/SubsystemCollection.h"

DEFINE_LOG_CATEGORY(LogMO56Debug);

namespace
{
    static TAutoConsoleVariable<int32> CVarMO56DebugEnabled(
        TEXT("MO56.Debug.Enable"),
        0,
        TEXT("Enable the MO56 structured debug logging subsystem."),
        ECVF_Default);

    static TAutoConsoleVariable<int32> CVarMO56DebugJson(
        TEXT("MO56.Debug.Json"),
        0,
        TEXT("Emit MO56 debug logs as JSON lines."),
        ECVF_Default);

    static FAutoConsoleCommand CCmdMO56DebugFlush(
        TEXT("MO56.Debug.Flush"),
        TEXT("Flush the MO56 structured debug log buffer to disk."),
        FConsoleCommandDelegate::CreateStatic(&UMO56DebugLogSubsystem::HandleFlushCommandStatic));
}

TSet<TWeakObjectPtr<UMO56DebugLogSubsystem>> UMO56DebugLogSubsystem::ActiveSubsystems;
bool UMO56DebugLogSubsystem::bConsoleDelegatesRegistered = false;

UMO56DebugLogSubsystem::UMO56DebugLogSubsystem()
{
    SessionStartUtc = FDateTime::UtcNow();
}

void UMO56DebugLogSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    SessionDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MO56Logs"));
    SessionFileName = FString::Printf(TEXT("Session-%s.log"), *SessionStartUtc.ToString(TEXT("%Y%m%d-%H%M%S")));
    bCommandLineJson = FParse::Param(FCommandLine::Get(), TEXT("LogJson"));

    {
        FScopeLock ScopeLock(&BufferMutex);
        BufferedEvents.Reserve(MaxBufferedEvents);
    }

    ActiveSubsystems.Add(this);
    RegisterConsoleDelegates();
    RefreshCVarState();

    UE_LOG(LogMO56Debug, Verbose, TEXT("MO56DebugLogSubsystem initialized. Enabled=%s Json=%s File=%s"),
        bEnabled ? TEXT("true") : TEXT("false"),
        (bJsonEnabled || bCommandLineJson) ? TEXT("true") : TEXT("false"),
        *ResolveSessionFilePath());
}

void UMO56DebugLogSubsystem::Deinitialize()
{
    FlushToDisk();

    ActiveSubsystems.Remove(this);
    if (ActiveSubsystems.Num() == 0)
    {
        UnregisterConsoleDelegates();
    }

    Super::Deinitialize();
}

void UMO56DebugLogSubsystem::LogEvent(FName System, FName Action, const FString& Detail, const FGuid& PlayerId, const FGuid& PawnId)
{
    if (!bEnabled)
    {
        return;
    }

    FMO56DebugEvent Event;
    Event.Timestamp = FDateTime::UtcNow();
    Event.System = System;
    Event.Action = Action;
    Event.Detail = Detail;
    Event.PlayerId = PlayerId;
    Event.PawnId = PawnId;

    {
        FScopeLock ScopeLock(&BufferMutex);
        BufferedEvents.Add(Event);

        if (BufferedEvents.Num() >= MaxBufferedEvents)
        {
            FlushToDisk();
        }
    }

    UE_LOG(LogMO56Debug, VeryVerbose, TEXT("%s"), *BuildLogLine(Event));
}

void UMO56DebugLogSubsystem::FlushToDisk()
{
    TArray<FMO56DebugEvent> EventsToWrite;
    {
        FScopeLock ScopeLock(&BufferMutex);
        if (BufferedEvents.Num() == 0)
        {
            return;
        }

        EventsToWrite = BufferedEvents;
        BufferedEvents.Reset();
    }

    AppendEventsToFile(EventsToWrite);
}

void UMO56DebugLogSubsystem::SetEnabled(bool bInEnabled)
{
    if (IConsoleVariable* Var = CVarMO56DebugEnabled.AsVariable())
    {
        Var->Set(bInEnabled ? 1 : 0, ECVF_SetByCode);
    }

    RefreshCVarState();

    if (!bEnabled)
    {
        FlushToDisk();
    }
}

void UMO56DebugLogSubsystem::SetJsonEnabled(bool bInJsonEnabled)
{
    if (IConsoleVariable* Var = CVarMO56DebugJson.AsVariable())
    {
        Var->Set(bInJsonEnabled ? 1 : 0, ECVF_SetByCode);
    }

    RefreshCVarState();
}

UMO56DebugLogSubsystem* UMO56DebugLogSubsystem::Get(const UObject* WorldContextObject)
{
    if (!WorldContextObject)
    {
        return nullptr;
    }

    if (const UWorld* World = WorldContextObject->GetWorld())
    {
        if (UGameInstance* GameInstance = World->GetGameInstance())
        {
            return GameInstance->GetSubsystem<UMO56DebugLogSubsystem>();
        }
    }

    if (const UGameInstance* GameInstance = Cast<UGameInstance>(WorldContextObject))
    {
        return GameInstance->GetSubsystem<UMO56DebugLogSubsystem>();
    }

    return nullptr;
}

void UMO56DebugLogSubsystem::HandleFlushCommand()
{
    FlushToDisk();
}

void UMO56DebugLogSubsystem::RefreshCVarState()
{
    const bool bCVarEnabled = CVarMO56DebugEnabled.GetValueOnAnyThread() != 0;
    const bool bCVarJson = CVarMO56DebugJson.GetValueOnAnyThread() != 0;

    bEnabled = bCVarEnabled;
    bJsonEnabled = bCVarJson || bCommandLineJson;
}

void UMO56DebugLogSubsystem::EnsureDirectoryCreated()
{
    if (SessionDirectory.IsEmpty())
    {
        SessionDirectory = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("MO56Logs"));
    }

    IFileManager::Get().MakeDirectory(*SessionDirectory, true);
}

FString UMO56DebugLogSubsystem::BuildLogLine(const FMO56DebugEvent& Event) const
{
    return (bJsonEnabled || bCommandLineJson) ? BuildJsonLine(Event) : BuildTextLine(Event);
}

FString UMO56DebugLogSubsystem::BuildJsonLine(const FMO56DebugEvent& Event) const
{
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    Writer->WriteObjectStart();
    Writer->WriteValue(TEXT("timestamp"), Event.Timestamp.ToIso8601());
    Writer->WriteValue(TEXT("system"), Event.System.ToString());
    Writer->WriteValue(TEXT("action"), Event.Action.ToString());
    Writer->WriteValue(TEXT("detail"), Event.Detail);
    if (Event.PlayerId.IsValid())
    {
        Writer->WriteValue(TEXT("playerId"), Event.PlayerId.ToString(EGuidFormats::DigitsWithHyphens));
    }
    if (Event.PawnId.IsValid())
    {
        Writer->WriteValue(TEXT("pawnId"), Event.PawnId.ToString(EGuidFormats::DigitsWithHyphens));
    }
    Writer->WriteObjectEnd();
    Writer->Close();

    return OutputString;
}

FString UMO56DebugLogSubsystem::BuildTextLine(const FMO56DebugEvent& Event) const
{
    FString PlayerString = Event.PlayerId.IsValid() ? Event.PlayerId.ToString(EGuidFormats::DigitsWithHyphens) : TEXT("None");
    FString PawnString = Event.PawnId.IsValid() ? Event.PawnId.ToString(EGuidFormats::DigitsWithHyphens) : TEXT("None");
    return FString::Printf(TEXT("[%s][%s][%s] %s | Player=%s Pawn=%s"),
        *Event.Timestamp.ToString(),
        *Event.System.ToString(),
        *Event.Action.ToString(),
        *Event.Detail,
        *PlayerString,
        *PawnString);
}

void UMO56DebugLogSubsystem::AppendEventsToFile(const TArray<FMO56DebugEvent>& EventsToWrite)
{
    if (EventsToWrite.Num() == 0)
    {
        return;
    }

    EnsureDirectoryCreated();

    const FString FilePath = ResolveSessionFilePath();
    if (FilePath.IsEmpty())
    {
        return;
    }

    TUniquePtr<FArchive> FileWriter(IFileManager::Get().CreateFileWriter(*FilePath, FILEWRITE_Append | FILEWRITE_AllowRead));
    if (!FileWriter)
    {
        UE_LOG(LogMO56Debug, Warning, TEXT("Failed to open debug log file %s for append."), *FilePath);
        return;
    }

    for (const FMO56DebugEvent& Event : EventsToWrite)
    {
        const FString Line = BuildLogLine(Event) + LINE_TERMINATOR;
        FTCHARToUTF8 Converter(*Line);
        FileWriter->Serialize(const_cast<UTF8CHAR*>(Converter.Get()), Converter.Length());
    }

    FileWriter->Close();
}

FString UMO56DebugLogSubsystem::ResolveSessionFilePath() const
{
    return FPaths::Combine(SessionDirectory, SessionFileName);
}

void UMO56DebugLogSubsystem::RegisterConsoleDelegates()
{
    if (bConsoleDelegatesRegistered)
    {
        return;
    }

    CVarMO56DebugEnabled.AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateStatic(&UMO56DebugLogSubsystem::HandleEnableChanged));
    CVarMO56DebugJson.AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate::CreateStatic(&UMO56DebugLogSubsystem::HandleJsonChanged));

    bConsoleDelegatesRegistered = true;
}

void UMO56DebugLogSubsystem::UnregisterConsoleDelegates()
{
    if (!bConsoleDelegatesRegistered)
    {
        return;
    }

    CVarMO56DebugEnabled.AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate());
    CVarMO56DebugJson.AsVariable()->SetOnChangedCallback(FConsoleVariableDelegate());

    bConsoleDelegatesRegistered = false;
}

void UMO56DebugLogSubsystem::ForEachActiveSubsystem(TFunctionRef<void(UMO56DebugLogSubsystem&)> Callback)
{
    for (TWeakObjectPtr<UMO56DebugLogSubsystem> WeakSubsystem : ActiveSubsystems)
    {
        if (UMO56DebugLogSubsystem* Subsystem = WeakSubsystem.Get())
        {
            Callback(*Subsystem);
        }
    }
}

void UMO56DebugLogSubsystem::HandleEnableChanged(IConsoleVariable* Var)
{
    ForEachActiveSubsystem([](UMO56DebugLogSubsystem& Subsystem)
    {
        Subsystem.RefreshCVarState();
    });
}

void UMO56DebugLogSubsystem::HandleJsonChanged(IConsoleVariable* Var)
{
    ForEachActiveSubsystem([](UMO56DebugLogSubsystem& Subsystem)
    {
        Subsystem.RefreshCVarState();
    });
}

void UMO56DebugLogSubsystem::HandleFlushCommandStatic()
{
    ForEachActiveSubsystem([](UMO56DebugLogSubsystem& Subsystem)
    {
        Subsystem.HandleFlushCommand();
    });
}

FGuid MO56ResolvePawnId(const APawn* Pawn)
{
    if (!Pawn)
    {
        return FGuid();
    }

    if (const UMOPersistentPawnComponent* PersistentComp = Pawn->FindComponentByClass<UMOPersistentPawnComponent>())
    {
        return PersistentComp->PawnId;
    }

    if (const AMO56Character* Character = Cast<AMO56Character>(Pawn))
    {
        return Character->GetCharacterId();
    }

    return FGuid();
}

FGuid MO56ResolveCharacterId(const ACharacter* Character)
{
    if (!Character)
    {
        return FGuid();
    }

    if (const AMO56Character* MO56Character = Cast<AMO56Character>(Character))
    {
        return MO56Character->GetCharacterId();
    }

    if (const UMOPersistentPawnComponent* PersistentComp = Character->FindComponentByClass<UMOPersistentPawnComponent>())
    {
        return PersistentComp->PawnId;
    }

    return FGuid();
}

