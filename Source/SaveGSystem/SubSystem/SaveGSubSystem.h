// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaveGSystem/Data/SaveGSystemDataTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGSubSystem.generated.h"

class UUpdateSaveDataAsyncTask;

/**
 * @class Subsystem for managing save and load operations in the game.
 */
UCLASS()
class SAVEGSYSTEM_API USaveGSubSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

#pragma region Subsystems

public:
    /** @public Get the SaveGSubSystem instance from the World **/
    static USaveGSubSystem* Get(const UWorld* World);

    /** @public Get the SaveGSubSystem instance from the GameInstance **/
    static USaveGSubSystem* Get(const UGameInstance* GameInstance);

#pragma endregion

#pragma region Actions

public:
    /** @public Creates or updates data for all properties of an object **/
    UFUNCTION(BlueprintCallable)
    void UpdateSaveData(FString Tag, UObject* SavedObject);

    /** @public Checking for a tag with saved data **/
    UFUNCTION(BlueprintCallable)
    bool IsHaveTag(FString Tag) const;

    /** @public Loads all the saved data into an object **/
    UFUNCTION(BlueprintCallable)
    void LoadSaveData(FString Tag, UObject* SavedObject);

    /** @public Save all data to a file **/
    UFUNCTION(BlueprintCallable)
    void SaveDataInFile(FString FileName = TEXT(""));

    /** @public Load data from a file **/
    UFUNCTION(BlueprintCallable)
    void LoadDataFromFile(const FString& FileName);

    /** @public Get all save files in the save directory **/
    UFUNCTION(BlueprintCallable)
    TArray<FString> GetAllSaveFiles();

    /** @public Check if any save/load action is in process **/
    UFUNCTION(BlueprintCallable)
    bool IsActionDataProcess() { return ActionDataAsyncTask.Get() != nullptr || RequestActionData.Num() != 0; }

protected:
    /** @protected Generate a save file name based on the current date and time **/
    virtual FString GenerateSaveFileName();

private:
    /** @private Process the next request in the action data queue **/
    void NextRequestActionData();

    /** @private Handle the completion of an async task **/
    UFUNCTION()
    void RegisterCompleteActionDataAsyncTask(const FString& Tag, UObject* SavedObject);

#pragma endregion

#pragma region Signatures

public:
    /** @public Bind a function to the OnActionLoadComplete delegate **/
    template <typename FuncClass>
    void BindOnActionLoadComplete(FuncClass* Object, void (FuncClass::*Func)(const FString&, UObject*))
    {
        BindDelegateActionSaveGSystemSignature(OnActionLoadComplete, Object, Func);
    }

    /** @public Bind a function to the OnActionSaveComplete delegate **/
    template <typename FuncClass>
    void BindOnActionSaveComplete(FuncClass* Object, void (FuncClass::*Func)(const FString&, UObject*))
    {
        BindDelegateActionSaveGSystemSignature(OnActionSaveComplete, Object, Func);
    }

protected:
    /** @protected Template function to bind a delegate with a specific signature FActionSaveGSystemSignature **/
    template <typename FuncClass>
    void BindDelegateActionSaveGSystemSignature(FActionSaveGSystemSignature& Delegate, FuncClass* Object, void (FuncClass::*Func)(const FString&, UObject*))
    {
        if (CLOG_SAVE_G_SYSTEM(Object == nullptr, "Object is nullptr")) return;
        if (CLOG_SAVE_G_SYSTEM(Func == nullptr, "Func is nullptr")) return;
        Delegate.AddUniqueDynamic(Object, Func);
    }

private:
    /** @private Delegate for load completion **/
    FActionSaveGSystemSignature OnActionLoadComplete;

    /** @private Delegate for save completion **/
    FActionSaveGSystemSignature OnActionSaveComplete;

#pragma endregion

#pragma region Data

private:
    /** @private Async task for processing save/load actions **/
    TWeakObjectPtr<UUpdateSaveDataAsyncTask> ActionDataAsyncTask;

    /** @private Request Queue for save/load actions **/
    TArray<FInitDataAsyncTask_SaveGSystem> RequestActionData;

    /** @private Map to store save data with tags as keys **/
    TMap<FString, FString> SaveGData;

#pragma endregion
};