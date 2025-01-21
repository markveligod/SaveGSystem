#pragma once
#include "Chaos/Deformable/MuscleActivationConstraints.h"

#if WITH_AUTOMATION_TESTS

#include "CoreMinimal.h"
#include "Misc/AutomationTest.h"
#include "SaveGSystem/Interface/SaveGInterface.h"
#include "SaveGSystem/SubSystem/SaveGSubSystem.h"

inline UWorld* GetTestGameWorld()
{
    const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
    auto FindElem = Algo::FindByPredicate(
        WorldContexts, [](const FWorldContext& Context) { return (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World(); });

    if (!FindElem)
    {
        FindElem = Algo::FindByPredicate(WorldContexts, [](const FWorldContext& Context) { return Context.WorldType == EWorldType::Editor && Context.World(); });
    }

    return FindElem ? FindElem->World() : nullptr;
}

inline UWorld* CreateWorld()
{
    FName WorldName = MakeUniqueObjectName(nullptr, UWorld::StaticClass(), NAME_None, EUniqueObjectNameOptions::GloballyUnique);
    FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
    UWorld* Result = UWorld::CreateWorld(EWorldType::Game, true, WorldName, GetTransientPackage());
    check(Result != nullptr);
    Result->AddToRoot();
    WorldContext.SetCurrentWorld(Result);

    Result->InitializeActorsForPlay(FURL());
    check(Result->GetPhysicsScene() != nullptr);

    return Result;
}

inline UGameInstance* CreateGameInstance(UWorld* World)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>(World);
    if (GameInstance)
    {
        GameInstance->AddToRoot();
        GameInstance->Init();
        World->SetGameInstance(GameInstance);
    }
    return GameInstance;
}

class FWorldSimulationTicker
{
public:
    virtual ~FWorldSimulationTicker() = default;

    FWorldSimulationTicker(const TFunction<bool()>& InCondition, float InTime, UWorld* World) : Condition(InCondition), MaxSimulatedTime(InTime), WeakWorld(World) {}

    virtual void Run()
    {
        const float DeltaTime = 0.016f;  // Simulate 60 FPS
        float ElapsedTime = 0.0f;
        bool bBreak = false;
        while (ElapsedTime < MaxSimulatedTime)
        {
            ++GFrameCounter;
            WeakWorld->Tick(LEVELTICK_All, DeltaTime);
            ElapsedTime += DeltaTime;
            if (bBreak)
            {
                break;
            }
            if (Condition())
            {
                bBreak = true;
            }
            // Simulate a small delay to mimic frame time
            FPlatformProcess::Sleep(DeltaTime);
        }
    }

private:
    TFunction<bool()> Condition;
    float MaxSimulatedTime{6.0f};
    TWeakObjectPtr<UWorld> WeakWorld;
};

// Define a custom latent command
class FSaveGWaitForResponseLatentCommand : public IAutomationLatentCommand
{
public:
    TFunction<bool()> Condition;
    TFunction<void()> OnComplete;

    FSaveGWaitForResponseLatentCommand(TFunction<bool()> InCondition, TFunction<void()> InOnComplete) : Condition(InCondition), OnComplete(InOnComplete) {}

    virtual bool Update() override
    {
        if (Condition())
        {
            OnComplete();
            return true;  // Done
        }

        return false;  // Keep waiting
    }
};

struct FInitTestWorld
{
    FInitTestWorld()
    {
        UWorld* L_World = CreateWorld();
        if (!L_World) return;
        L_World->InitializeActorsForPlay(FURL(), true);

        UGameInstance* L_GameInstance = CreateGameInstance(L_World);
        if (!L_GameInstance) return;

        USaveGSubSystem* L_SaveGSubSystem = USaveGSubSystem::Get(L_GameInstance);
        if (!L_SaveGSubSystem) return;
        WeakWorld = L_World;
        WeakGameInstance = L_GameInstance;
        WeakSaveGSubSystem = L_SaveGSubSystem;
    }

    ~FInitTestWorld()
    {
        if (WeakWorld.Get())
        {
            WeakWorld->DestroyWorld(false);
            WeakWorld.Reset();
        }
        if (WeakGameInstance.Get())
        {
            WeakGameInstance->Shutdown();
            WeakGameInstance.Reset();
        }
        WeakSaveGSubSystem.Reset();
    }

    bool IsValid() const { return WeakWorld.IsValid() && WeakGameInstance.IsValid() && WeakSaveGSubSystem.IsValid(); }

    TWeakObjectPtr<UWorld> WeakWorld;
    TWeakObjectPtr<UGameInstance> WeakGameInstance;
    TWeakObjectPtr<USaveGSubSystem> WeakSaveGSubSystem;
};

#endif

#include "SaveGSystemTests.generated.h"

UCLASS()
class USaveGBaseTestObject : public UObject, public ISaveGInterface
{
    GENERATED_BODY()

public:
    bool bActionPreSave{false};
    bool bActionPostSave{false};
    bool bActionPreLoad{false};
    bool bActionPostLoad{false};

    virtual void PreSave_Implementation() override { bActionPreSave = true; }
    virtual void PostSave_Implementation() override { bActionPostSave = true; }
    virtual void PreLoad_Implementation() override { bActionPreLoad = true; }
    virtual void PostLoad_Implementation() override { bActionPostLoad = true; }

    bool IsSaved() const { return bActionPostSave; }
    bool IsLoaded() const { return bActionPostLoad; }

    virtual void Generate() {}
    virtual bool IsValidValue() { return true; }
    virtual void Reset() {}
};

UCLASS()
class SAVEGSYSTEM_API USaveGTestBoolObject : public USaveGBaseTestObject
{
    GENERATED_BODY()

private:
    UPROPERTY(SaveGame)
    bool bTestBool{false};

public:
    virtual void Generate() override { bTestBool = true; }
    virtual bool IsValidValue() override { return bTestBool; }
    virtual void Reset() override { bTestBool = false; }
};

UENUM()
enum class ETestEnumObject : uint8
{
    None = 0,
    TestValue1,
    TestValue2,
    TestValue3,
};

UCLASS()
class SAVEGSYSTEM_API USaveGTestByteObject : public USaveGBaseTestObject
{
    GENERATED_BODY()

private:
    UPROPERTY(SaveGame)
    uint8 Uint8{0};

    UPROPERTY(SaveGame)
    ETestEnumObject Enum{ETestEnumObject::None};

public:
    virtual void Generate() override
    {
        Uint8 = FMath::RandRange(1, UINT8_MAX);
        Enum = ETestEnumObject::TestValue2;
    }
    virtual bool IsValidValue() override { return Uint8 != 0 && Enum != ETestEnumObject::None; }
    virtual void Reset() override
    {
        Uint8 = 0;
        Enum = ETestEnumObject::None;
    }
};

UCLASS()
class SAVEGSYSTEM_API USaveGTestNumericObject : public USaveGBaseTestObject
{
    GENERATED_BODY()

private:
    UPROPERTY(SaveGame)
    int8 Int8{0};

    UPROPERTY(SaveGame)
    int16 Int16{0};

    UPROPERTY(SaveGame)
    uint16 UInt16{0};

    UPROPERTY(SaveGame)
    int32 Int32{0};

    UPROPERTY(SaveGame)
    uint32 UInt32{0};

    UPROPERTY(SaveGame)
    int64 Int64{0};

    UPROPERTY(SaveGame)
    uint64 UInt64{0};

    UPROPERTY(SaveGame)
    float Float{0.0f};

    UPROPERTY(SaveGame)
    double Double{0.0};

public:
    virtual void Generate() override
    {
        Int8 = FMath::RandRange(1, INT8_MAX);
        Int16 = FMath::RandRange(1, INT16_MAX);
        UInt16 = FMath::RandRange(1, UINT16_MAX);
        Int32 = FMath::RandRange(1, INT32_MAX);
        UInt32 = FMath::RandRange(1, UINT32_MAX);
        Int64 = FMath::RandRange(1, INT32_MAX);
        UInt64 = FMath::RandRange(1, UINT32_MAX);
        Float = FMath::RandRange(1.0f, MAX_FLT);
        Double = FMath::RandRange(1.0, MAX_dbl);
    }

    virtual bool IsValidValue() override
    {
        return Int8 != 0 && Int16 != 0 && UInt16 != 0 && Int32 != 0 && UInt32 != 0 && Int64 != 0 && UInt64 != 0 && Float != 0.0f && Double != 0.0;
    }

    virtual void Reset() override
    {
        Int8 = 0;
        Int16 = 0;
        UInt16 = 0;
        Int32 = 0;
        UInt32 = 0;
        Int64 = 0;
        UInt64 = 0;
        Float = 0.0f;
        Double = 0.0;
    }
};

UCLASS()
class SAVEGSYSTEM_API USaveGTestStringObject : public USaveGBaseTestObject
{
    GENERATED_BODY()

private:
    UPROPERTY(SaveGame)
    FString Str{};

    UPROPERTY(SaveGame)
    FName Name{NAME_None};

    UPROPERTY(SaveGame)
    FText Text{};

public:
    virtual void Generate() override
    {
        Str = TEXT("Hello World !!!");
        Name = FName(Str);
        Text = FText::FromString(Str);
    }

    virtual bool IsValidValue() override { return !Str.IsEmpty() && !Name.IsNone() && !Text.IsEmpty(); }

    virtual void Reset() override
    {
        Str.Empty();
        Name = NAME_None;
        Text = FText::FromString(Str);
    }
};

UCLASS()
class SAVEGSYSTEM_API USaveGTestObjectHandle : public USaveGBaseTestObject
{
    GENERATED_BODY()

private:
    UPROPERTY(SaveGame)
    TSoftClassPtr<UClass> SoftClass{nullptr};

    UPROPERTY(SaveGame)
    TSoftObjectPtr<UObject> SoftObject{nullptr};

    UPROPERTY(SaveGame)
    FSoftClassPath PathClass{};

    UPROPERTY(SaveGame)
    FSoftObjectPath PathObject{};

public:
    virtual void Generate() override
    {
        SoftClass = AActor::StaticClass();
        SoftObject = AActor::StaticClass();
        PathClass = AActor::StaticClass();
        PathObject = AActor::StaticClass();
    }

    virtual bool IsValidValue() override
    {
        return !SoftClass.GetAssetName().IsEmpty() && !SoftObject.GetAssetName().IsEmpty() && !PathClass.GetAssetName().IsEmpty() && !PathObject.GetAssetName().IsEmpty();
    }

    virtual void Reset() override
    {
        SoftClass.Reset();
        SoftObject.Reset();
        PathClass.Reset();
        PathObject.Reset();
    }
};

USTRUCT()
struct FTestStructObject
{
    GENERATED_BODY()

    uint8 DefaultUint8{0};
    ETestEnumObject DefaultEnum{ETestEnumObject::None};
    bool DefaultbTestBool{false};
    int8 DefaultInt8{0};
    int16 DefaultInt16{0};
    uint16 DefaultUInt16{0};
    int32 DefaultInt32{0};
    uint32 DefaultUInt32{0};
    int64 DefaultInt64{0};
    uint64 DefaultUInt64{0};
    float DefaultFloat{0.0f};
    double DefaultDouble{0.0};
    TSoftClassPtr<UClass> DefaultSoftClass{nullptr};
    TSoftObjectPtr<UObject> DefaultSoftObject{nullptr};
    FSoftClassPath DefaultPathClass{};
    FSoftObjectPath DefaultPathObject{};
    FString DefaultStr{};
    FName DefaultName{NAME_None};
    FText DefaultText{};

    // ---
    UPROPERTY()
    uint8 Uint8{0};

    UPROPERTY()
    ETestEnumObject Enum{ETestEnumObject::None};

    UPROPERTY()
    bool bTestBool{false};

    UPROPERTY()
    int8 Int8{0};

    UPROPERTY()
    int16 Int16{0};

    UPROPERTY()
    uint16 UInt16{0};

    UPROPERTY()
    int32 Int32{0};

    UPROPERTY()
    uint32 UInt32{0};

    UPROPERTY()
    int64 Int64{0};

    UPROPERTY()
    uint64 UInt64{0};

    UPROPERTY()
    float Float{0.0f};

    UPROPERTY()
    double Double{0.0};

    UPROPERTY()
    TSoftClassPtr<UClass> SoftClass{nullptr};

    UPROPERTY()
    TSoftObjectPtr<UObject> SoftObject{nullptr};

    UPROPERTY()
    FSoftClassPath PathClass{};

    UPROPERTY()
    FSoftObjectPath PathObject{};

    UPROPERTY()
    FString Str{};

    UPROPERTY()
    FName Name{NAME_None};

    UPROPERTY()
    FText Text{};

    void Generate()
    {
        DefaultSoftClass = AActor::StaticClass();
        DefaultSoftObject = AActor::StaticClass();
        DefaultPathClass = AActor::StaticClass();
        DefaultPathObject = AActor::StaticClass();
        DefaultStr = TEXT("Hello World !!!");
        DefaultName = FName(Str);
        DefaultText = FText::FromString(Str);
        DefaultInt8 = FMath::RandRange(1, INT8_MAX);
        DefaultInt16 = FMath::RandRange(1, INT16_MAX);
        DefaultUInt16 = FMath::RandRange(1, UINT16_MAX);
        DefaultInt32 = FMath::RandRange(1, INT32_MAX);
        DefaultUInt32 = FMath::RandRange(1, UINT32_MAX);
        DefaultInt64 = FMath::RandRange(1, INT32_MAX);
        DefaultUInt64 = FMath::RandRange(1, UINT32_MAX);
        DefaultFloat = FMath::RandRange(1.0f, MAX_FLT);
        DefaultDouble = FMath::RandRange(1.0, MAX_dbl);
        DefaultUint8 = FMath::RandRange(1, UINT8_MAX);
        DefaultEnum = ETestEnumObject::TestValue2;
        DefaultbTestBool = true;

        SoftClass = DefaultSoftClass;
        SoftObject = DefaultSoftObject;
        PathClass = DefaultPathClass;
        PathObject = DefaultPathObject;
        Str = DefaultStr;
        Name = DefaultName;
        Text = DefaultText;
        Int8 = DefaultInt8;
        Int16 = DefaultInt16;
        UInt16 = DefaultUInt16;
        Int32 = DefaultInt32;
        UInt32 = DefaultUInt32;
        Int64 = DefaultInt64;
        UInt64 = DefaultUInt64;
        Float = DefaultFloat;
        Double = DefaultDouble;
        Uint8 = DefaultUint8;
        Enum = DefaultEnum;
        bTestBool = DefaultbTestBool;
    }

    bool IsValidValue() const
    {
        if (SoftClass.GetAssetName() != DefaultSoftClass.GetAssetName()) return false;
        if (SoftObject.GetAssetName() != DefaultSoftObject.GetAssetName()) return false;
        if (PathClass.GetAssetName() != DefaultPathClass.GetAssetName()) return false;
        if (PathObject.GetAssetName() != DefaultPathObject.GetAssetName()) return false;
        if (Str != DefaultStr) return false;
        if (Name != DefaultName) return false;
        if (Text.ToString() != DefaultText.ToString()) return false;
        if (bTestBool != DefaultbTestBool) return false;
        if (Int8 != DefaultInt8) return false;
        if (Uint8 != DefaultUint8) return false;
        if (Int16 != DefaultInt16) return false;
        if (UInt16 != DefaultUInt16) return false;
        if (Int32 != DefaultInt32) return false;
        if (UInt32 != DefaultUInt32) return false;
        if (Int64 != DefaultInt64) return false;
        if (UInt64 != DefaultUInt64) return false;

        return true;
    }

    bool IsValidValue(const FTestStructObject& Other) const
    {
        if (SoftClass.GetAssetName() != Other.DefaultSoftClass.GetAssetName()) return false;
        if (SoftObject.GetAssetName() != Other.DefaultSoftObject.GetAssetName()) return false;
        if (PathClass.GetAssetName() != Other.DefaultPathClass.GetAssetName()) return false;
        if (PathObject.GetAssetName() != Other.DefaultPathObject.GetAssetName()) return false;
        if (Str != Other.DefaultStr) return false;
        if (Name != Other.DefaultName) return false;
        if (Text.ToString() != Other.DefaultText.ToString()) return false;
        if (bTestBool != Other.DefaultbTestBool) return false;
        if (Int8 != Other.DefaultInt8) return false;
        if (Uint8 != Other.DefaultUint8) return false;
        if (Int16 != Other.DefaultInt16) return false;
        if (UInt16 != Other.DefaultUInt16) return false;
        if (Int32 != Other.DefaultInt32) return false;
        if (UInt32 != Other.DefaultUInt32) return false;
        if (Int64 != Other.DefaultInt64) return false;
        if (UInt64 != Other.DefaultUInt64) return false;

        return true;
    }

    void Reset()
    {
        SoftClass.Reset();
        SoftObject.Reset();
        PathClass.Reset();
        PathObject.Reset();
        Str.Empty();
        Name = NAME_None;
        Text = FText::FromString(Str);
        Int8 = 0;
        Int16 = 0;
        UInt16 = 0;
        Int32 = 0;
        UInt32 = 0;
        Int64 = 0;
        UInt64 = 0;
        Float = 0.0f;
        Double = 0.0;
        Uint8 = 0;
        Enum = ETestEnumObject::None;
        bTestBool = false;
    }
};

UCLASS()
class SAVEGSYSTEM_API USaveGTestStructObject : public USaveGBaseTestObject
{
    GENERATED_BODY()

private:
    UPROPERTY(SaveGame)
    FTestStructObject Struct{};

public:
    virtual void Generate() override { Struct.Generate(); }
    virtual bool IsValidValue() override { return Struct.IsValidValue(); }
    virtual void Reset() override { Struct.Reset(); }
};

UCLASS()
class SAVEGSYSTEM_API USaveGTestArrayStructObject : public USaveGBaseTestObject
{
    GENERATED_BODY()

private:
    UPROPERTY(SaveGame)
    TArray<FTestStructObject> ArrayStruct{};

public:
    virtual void Generate() override
    {
        for (int32 i = 0; i < 3; i++)
        {
            ArrayStruct.Add(FTestStructObject());
        }
        for (auto& Data : ArrayStruct)
        {
            Data.Generate();
        }
    }
    virtual bool IsValidValue() override
    {
        if (ArrayStruct.Num() == 0) return false;
        for (auto& Data : ArrayStruct)
        {
            if (!Data.IsValidValue()) return false;
        }
        return true;
    }
    virtual void Reset() override
    {
        for (auto& Data : ArrayStruct)
        {
            Data.Reset();
        }
    }
};

UCLASS()
class SAVEGSYSTEM_API USaveGTestMapStructObject : public USaveGBaseTestObject
{
    GENERATED_BODY()

private:
    FTestStructObject TestStruct;

    // Default Maps
    TMap<ETestEnumObject, FTestStructObject> DefaultMapETestEnumObject{};
    TMap<uint8, FTestStructObject> DefaultMapUInt8{};
    TMap<int8, FTestStructObject> DefaultMapInt8{};
    TMap<uint16, FTestStructObject> DefaultMapUInt16{};
    TMap<int16, FTestStructObject> DefaultMapInt16{};
    TMap<uint32, FTestStructObject> DefaultMapUInt32{};
    TMap<int32, FTestStructObject> DefaultMapInt32{};
    TMap<uint64, FTestStructObject> DefaultMapUInt64{};
    TMap<int64, FTestStructObject> DefaultMapInt64{};
    TMap<FString, FTestStructObject> DefaultMapFString{};
    TMap<FName, FTestStructObject> DefaultMapFName{};

    // Saved Maps
    UPROPERTY(SaveGame)
    TMap<ETestEnumObject, FTestStructObject> MapETestEnumObject{};

    UPROPERTY(SaveGame)
    TMap<uint8, FTestStructObject> MapUInt8{};

    UPROPERTY(SaveGame)
    TMap<int8, FTestStructObject> MapInt8{};

    UPROPERTY(SaveGame)
    TMap<uint16, FTestStructObject> MapUInt16{};

    UPROPERTY(SaveGame)
    TMap<int16, FTestStructObject> MapInt16{};

    UPROPERTY(SaveGame)
    TMap<uint32, FTestStructObject> MapUInt32{};

    UPROPERTY(SaveGame)
    TMap<int32, FTestStructObject> MapInt32{};

    UPROPERTY(SaveGame)
    TMap<uint64, FTestStructObject> MapUInt64{};

    UPROPERTY(SaveGame)
    TMap<int64, FTestStructObject> MapInt64{};

    UPROPERTY(SaveGame)
    TMap<FString, FTestStructObject> MapFString{};

    UPROPERTY(SaveGame)
    TMap<FName, FTestStructObject> MapFName{};

    UPROPERTY(SaveGame)
    TMap<int32, FTestStructObject> MapFText{};

public:
    virtual void Generate() override
    {
        TestStruct.Generate();

        // Helper function to populate a map with test data
        auto PopulateMap = [&](auto& Map, auto KeyGenerator)
        {
            Map.Empty();
            for (int32 i = 0; i < 3; ++i)
            {
                Map.Add(KeyGenerator(i), TestStruct);
            }
        };

        // Populate default maps
        PopulateMap(DefaultMapFString, [](int32 i) { return TEXT("Hello World") + FString::FromInt(i); });
        PopulateMap(DefaultMapFName, [](int32 i) { return FName(TEXT("Hello World") + FString::FromInt(i)); });
        PopulateMap(DefaultMapETestEnumObject, [](int32 i) { return static_cast<ETestEnumObject>(i); });
        PopulateMap(DefaultMapUInt8, [](int32 i) { return static_cast<uint8>(i); });
        PopulateMap(DefaultMapInt8, [](int32 i) { return static_cast<int8>(i); });
        PopulateMap(DefaultMapUInt16, [](int32 i) { return static_cast<uint16>(i); });
        PopulateMap(DefaultMapInt16, [](int32 i) { return static_cast<int16>(i); });
        PopulateMap(DefaultMapUInt32, [](int32 i) { return static_cast<uint32>(i); });
        PopulateMap(DefaultMapInt32, [](int32 i) { return static_cast<int32>(i); });
        PopulateMap(DefaultMapUInt64, [](int32 i) { return static_cast<uint64>(i); });
        PopulateMap(DefaultMapInt64, [](int32 i) { return static_cast<int64>(i); });

        // Copy default maps to saved maps
        MapFString = DefaultMapFString;
        MapFName = DefaultMapFName;
        MapETestEnumObject = DefaultMapETestEnumObject;
        MapUInt8 = DefaultMapUInt8;
        MapUInt16 = DefaultMapUInt16;
        MapUInt32 = DefaultMapUInt32;
        MapUInt64 = DefaultMapUInt64;
        MapInt8 = DefaultMapInt8;
        MapInt16 = DefaultMapInt16;
        MapInt32 = DefaultMapInt32;
        MapInt64 = DefaultMapInt64;
    }

    virtual bool IsValidValue() override
    {
        // Helper function to validate a map
        auto ValidateMap = [&](const auto& DefaultMap, const auto& Map)
        {
            for (const auto& Pair : DefaultMap)
            {
                if (!Map.Contains(Pair.Key) || !Map[Pair.Key].IsValidValue(TestStruct))
                {
                    return false;
                }
            }
            return true;
        };

        return ValidateMap(DefaultMapFString, MapFString) && ValidateMap(DefaultMapFName, MapFName) && ValidateMap(DefaultMapETestEnumObject, MapETestEnumObject) &&
               ValidateMap(DefaultMapUInt8, MapUInt8) && ValidateMap(DefaultMapUInt16, MapUInt16) && ValidateMap(DefaultMapUInt32, MapUInt32) &&
               ValidateMap(DefaultMapUInt64, MapUInt64) && ValidateMap(DefaultMapInt8, MapInt8) && ValidateMap(DefaultMapInt16, MapInt16) &&
               ValidateMap(DefaultMapInt32, MapInt32) && ValidateMap(DefaultMapInt64, MapInt64);
    }

    virtual void Reset() override
    {
        // Helper function to reset a map
        auto ResetMap = [](auto& Map) { Map.Empty(); };

        ResetMap(MapFString);
        ResetMap(MapFName);
        ResetMap(MapFText);
        ResetMap(MapETestEnumObject);
        ResetMap(MapUInt8);
        ResetMap(MapInt8);
        ResetMap(MapUInt16);
        ResetMap(MapInt16);
        ResetMap(MapUInt32);
        ResetMap(MapInt32);
        ResetMap(MapUInt64);
        ResetMap(MapInt64);
    }
};
