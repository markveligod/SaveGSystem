#pragma once

#include "CoreMinimal.h"
#include "SaveGSystemDataTypes.generated.h"

/** --- | LOGGING | --- **/

DEFINE_LOG_CATEGORY_STATIC(LogSaveGSystem, All, All);

namespace SaveGSystemSpace
{
inline bool IsLogPrint()
{
    if (const auto SaveGSystemShowLog = IConsoleManager::Get().FindConsoleVariable(TEXT("SaveGSystem.ShowLog")))
    {
#if WITH_EDITOR
        return true;
#else
        return SaveGSystemShowLog->GetBool();
#endif
    }
    return false;
}

inline bool ClogPrint(bool Cond, TCHAR* NameFunction, const FString& Text)
{
    if (Cond)
    {
        UE_LOG(LogSaveGSystem, Error, TEXT("[%s] | TEXT:[%s]"), NameFunction, *Text);
    }
    return Cond;
}
}  // namespace SaveGSystemSpace

#define LOG_SAVE_G_SYSTEM(Verbosity, Format, ...)                                                           \
    {                                                                                                       \
        if (SaveGSystemSpace::IsLogPrint())                                                                 \
        {                                                                                                   \
            const FString Msg = FString::Printf(TEXT(Format), ##__VA_ARGS__);                               \
            UE_LOG(LogSaveGSystem, Verbosity, TEXT("[%s] | TEXT:[%s]"), ANSI_TO_TCHAR(__FUNCTION__), *Msg); \
        }                                                                                                   \
    }

#define CLOG_SAVE_G_SYSTEM(Cond, Format, ...) \
    SaveGSystemSpace::ClogPrint(Cond, ANSI_TO_TCHAR(__FUNCTION__), FString::Printf(TEXT(Format), ##__VA_ARGS__))

/** --- | UENUM | --- **/
UENUM()
enum class ETaskAction_SaveGSystem : uint8
{
    None = 0,
    Save,
    Load,
};

/** --- | STRUCT | --- **/

/** @struct Data for the operation of asynktask **/
struct FInitDataAsyncTask_SaveGSystem
{
    ETaskAction_SaveGSystem Action{ETaskAction_SaveGSystem::None};
    float Delay{5.0f};
    FString Tag{};
    TWeakObjectPtr<> Object{nullptr};
    FString JsonSaveData{};

    UObject* GetObject() const { return Object.Get(); }

    UClass* GetObjectClass() const { return Object.IsValid() ? Object->GetClass() : nullptr; }

    bool IsValid() const
    {
        return Object.IsValid() && Object->GetClass() && !Tag.IsEmpty() && Action != ETaskAction_SaveGSystem::None && Delay > 0.0f;
    }
};

/** --- | Signatures | --- **/

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FActionSaveGSystemSignature, const FString&, Tag, UObject*, SavedObject);
