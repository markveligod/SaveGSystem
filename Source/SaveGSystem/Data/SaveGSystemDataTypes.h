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

/** --- | ENUM | --- **/

/** @enum Type of the stored variable **/
UENUM(BlueprintType)
enum class ETypeProperty_SaveG : uint8
{
    None,
    Number,
    Boolean,
    String,
};

/** --- | STRUCT | --- **/

/** @struct Storing context data about a variable **/
USTRUCT(BlueprintType)
struct FPropertyContext_SaveG
{
    GENERATED_BODY()

    /****/
    UPROPERTY(BlueprintReadWrite)
    ETypeProperty_SaveG TypeProperty_SaveG{ETypeProperty_SaveG::None};

    /****/
    UPROPERTY(BlueprintReadWrite)
    FString PropertyName;

    /****/
    UPROPERTY(BlueprintReadWrite)
    FString PropertyValue;
};

/** @struct Storing data to bind a key to data by property **/
USTRUCT(BlueprintType)
struct FObjectContext_SaveG
{
    GENERATED_BODY()

    /** The key is a tag indicating the registration of an object. **/
    UPROPERTY(BlueprintReadWrite)
    FString Key;

    /****/
    UPROPERTY(BlueprintReadWrite)
    TArray<FPropertyContext_SaveG> Values;
};

/** @struct Storing all data for serialization in JSON format. **/
USTRUCT(BlueprintType)
struct FJsonSerializer_SaveG
{
    GENERATED_BODY()

    /** Data with manual registration in the save system **/
    UPROPERTY(BlueprintReadWrite)
    TArray<FObjectContext_SaveG> CustomObjects;
};
