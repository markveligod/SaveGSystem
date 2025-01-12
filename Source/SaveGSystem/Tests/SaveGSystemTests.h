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
    auto FindElem = Algo::FindByPredicate(WorldContexts, [](const FWorldContext& Context)
        { return (Context.WorldType == EWorldType::PIE || Context.WorldType == EWorldType::Game) && Context.World(); });

    if (!FindElem)
    {
        FindElem = Algo::FindByPredicate(
            WorldContexts, [](const FWorldContext& Context) { return Context.WorldType == EWorldType::Editor && Context.World(); });
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

    FWorldSimulationTicker(const TFunction<bool()>& InCondition, float InTime, UWorld* World)
        : Condition(InCondition), MaxSimulatedTime(InTime), WeakWorld(World)
    {
    }

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

    FSaveGWaitForResponseLatentCommand(TFunction<bool()> InCondition, TFunction<void()> InOnComplete)
        : Condition(InCondition), OnComplete(InOnComplete)
    {
    }

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
        return Int8 != 0 && Int16 != 0 && UInt16 != 0 && Int32 != 0 && UInt32 != 0 && Int64 != 0 && UInt64 != 0 && Float != 0.0f &&
               Double != 0.0;
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
        return !SoftClass.GetAssetName().IsEmpty() && !SoftObject.GetAssetName().IsEmpty() && !PathClass.GetAssetName().IsEmpty() &&
               !PathObject.GetAssetName().IsEmpty();
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
        SoftClass = AActor::StaticClass();
        SoftObject = AActor::StaticClass();
        PathClass = AActor::StaticClass();
        PathObject = AActor::StaticClass();
        Str = TEXT("Hello World !!!");
        Name = FName(Str);
        Text = FText::FromString(Str);
        Int8 = FMath::RandRange(1, INT8_MAX);
        Int16 = FMath::RandRange(1, INT16_MAX);
        UInt16 = FMath::RandRange(1, UINT16_MAX);
        Int32 = FMath::RandRange(1, INT32_MAX);
        UInt32 = FMath::RandRange(1, UINT32_MAX);
        Int64 = FMath::RandRange(1, INT32_MAX);
        UInt64 = FMath::RandRange(1, UINT32_MAX);
        Float = FMath::RandRange(1.0f, MAX_FLT);
        Double = FMath::RandRange(1.0, MAX_dbl);
        Uint8 = FMath::RandRange(1, UINT8_MAX);
        Enum = ETestEnumObject::TestValue2;
        bTestBool = true;
    }

    bool IsValidValue()
    {
        return bTestBool && Uint8 != 0 && Enum != ETestEnumObject::None && Int8 != 0 && Int16 != 0 && UInt16 != 0 && Int32 != 0 &&
               UInt32 != 0 && Int64 != 0 && UInt64 != 0 && Float != 0.0f && Double != 0.0 && !Str.IsEmpty() && !Name.IsNone() &&
               !Text.IsEmpty() && !SoftClass.GetAssetName().IsEmpty() && !SoftObject.GetAssetName().IsEmpty() &&
               !PathClass.GetAssetName().IsEmpty() && !PathObject.GetAssetName().IsEmpty();
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
    virtual void Reset() override { ArrayStruct.Empty(); }
};

UCLASS()
class SAVEGSYSTEM_API USaveGTestMapStructObject : public USaveGBaseTestObject
{
    GENERATED_BODY()

private:
    UPROPERTY(SaveGame)
    TMap<int32, FTestStructObject> MapStruct{};

public:
    virtual void Generate() override
    {
        for (int32 i = 0; i < 3; i++)
        {
            MapStruct.Add(i, FTestStructObject());
        }
        for (auto& Data : MapStruct)
        {
            Data.Value.Generate();
        }
    }
    virtual bool IsValidValue() override
    {
        if (MapStruct.Num() == 0) return false;
        for (int32 i = 0; i < 3; i++)
        {
            if (!MapStruct.Contains(i)) return false;
            auto& Data = MapStruct[i];
            if (!Data.IsValidValue()) return false;
        }
        return true;
    }
    virtual void Reset() override { MapStruct.Empty(); }
};
