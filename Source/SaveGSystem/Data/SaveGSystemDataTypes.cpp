
#include "SaveGSystemDataTypes.h"
#include "EngineUtils.h"
#include "SaveGSystem/Interface/SaveGInterface.h"
#include "SaveGSystem/SubSystem/SaveGSubSystem.h"

#if !UE_BUILD_SHIPPING && !UE_BUILD_TEST

static TAutoConsoleVariable<bool> EnableD_SaveGSystemShowLog(TEXT("SaveGSystem.ShowLog"), false, TEXT("SaveGSystem.ShowLog [true/false]"), ECVF_Cheat);

static FAutoConsoleCommandWithWorldAndArgs EnableD_SaveGSystemSaveAllWorldActor(TEXT("SaveGSystem.SaveAllWorldActor"),
    TEXT("SaveGSystem.SaveAllWorldActor <nothing> | Let's go save all the actors that have the USaveGInterface"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
        [](const TArray<FString>& Args, UWorld* World)
        {
            USaveGSubSystem* SaveGSubSystem = USaveGSubSystem::Get(World);
            if (!SaveGSubSystem) return;
            for (FActorIterator It(World); It; ++It)
            {
                AActor* Actor = *It;
                if (!Actor) continue;
                if (!Actor->GetClass()) continue;
                if (!Actor->GetClass()->ImplementsInterface(USaveGInterface::StaticClass())) continue;
                SaveGSubSystem->UpdateSaveData(Actor->GetName(), Actor);
            }
        }),
    ECVF_Cheat);

static FAutoConsoleCommandWithWorldAndArgs EnableD_SaveGSystemLoadAllWorldActor(TEXT("SaveGSystem.LoadAllWorldActor"),
    TEXT("SaveGSystem.LoadAllWorldActor <nothing> | Let's go load all the actors that have the USaveGInterface"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
        [](const TArray<FString>& Args, UWorld* World)
        {
            USaveGSubSystem* SaveGSubSystem = USaveGSubSystem::Get(World);
            if (!SaveGSubSystem) return;
            for (FActorIterator It(World); It; ++It)
            {
                AActor* Actor = *It;
                if (!Actor) continue;
                if (!Actor->GetClass()) continue;
                if (!Actor->GetClass()->ImplementsInterface(USaveGInterface::StaticClass())) continue;
                SaveGSubSystem->LoadSaveData(Actor->GetName(), Actor);
            }
        }),
    ECVF_Cheat);

static FAutoConsoleCommandWithWorldAndArgs EnableD_SaveGSystemSaveFile(TEXT("SaveGSystem.SaveFile"), TEXT("SaveGSystem.SaveFile [Arg1 = FileName]"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!Args.IsValidIndex(0)) return;
            USaveGSubSystem* SaveGSubSystem = USaveGSubSystem::Get(World);
            if (!SaveGSubSystem) return;
            SaveGSubSystem->SaveDataInFile(Args[0]);
        }),
    ECVF_Cheat);

static FAutoConsoleCommandWithWorldAndArgs EnableD_SaveGSystemLoadFile(TEXT("SaveGSystem.LoadFile"), TEXT("SaveGSystem.LoadFile [Arg1 = FileName]"),
    FConsoleCommandWithWorldAndArgsDelegate::CreateLambda(
        [](const TArray<FString>& Args, UWorld* World)
        {
            if (!Args.IsValidIndex(0)) return;
            USaveGSubSystem* SaveGSubSystem = USaveGSubSystem::Get(World);
            if (!SaveGSubSystem) return;
            SaveGSubSystem->LoadDataFromFile(Args[0]);
        }),
    ECVF_Cheat);

#endif
