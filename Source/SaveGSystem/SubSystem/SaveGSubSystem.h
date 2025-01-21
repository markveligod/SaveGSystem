// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaveGSystem/Data/SaveGSystemDataTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGSubSystem.generated.h"

class UUpdateSaveDataAsyncTask;
/**
 *
 */
UCLASS()
class SAVEGSYSTEM_API USaveGSubSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

#pragma region Subsystems

public:
    /** @public **/
    static USaveGSubSystem* Get(const UWorld* World);

    /** @public **/
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

    /** @public  **/
    UFUNCTION(BlueprintCallable)
    void SaveDataInFile(FString FileName = TEXT(""));

    /** @public  **/
    UFUNCTION(BlueprintCallable)
    void LoadDataFromFile(const FString& FileName);

    /** @public  **/
    UFUNCTION(BlueprintCallable)
    TArray<FString> GetAllSaveFiles();

    /** @public  **/
    UFUNCTION(BlueprintCallable)
    bool IsActionDataProcess() { return ActionDataAsyncTask.Get() != nullptr || RequestActionData.Num() != 0; }

protected:
    /** @protected  **/
    virtual FString GenerateSaveFileName();

private:
    /** @private **/
    void NextRequestActionData();

    /** @private **/
    UFUNCTION()
    void RegisterCompleteActionDataAsyncTask(const FString& Tag, UObject* SavedObject);

#pragma endregion

#pragma region Signatures

public:
    /** @public Signature on delegate OnActionLoadComplete
     * Use macro UFUNCTION()
     * Example: BindOnActionLoadComplete(this, &ThisClass::SomeFuncName)
     */
    template <typename FuncClass>
    void BindOnActionLoadComplete(FuncClass* Object, void (FuncClass::*Func)(const FString&, UObject*))
    {
        BindDelegateActionSaveGSystemSignature(OnActionLoadComplete, Object, Func);
    }

    /** @public Signature on delegate OnActionSaveComplete
     * Use macro UFUNCTION()
     * Example: BindOnActionSaveComplete(this, &ThisClass::SomeFuncName)
     */
    template <typename FuncClass>
    void BindOnActionSaveComplete(FuncClass* Object, void (FuncClass::*Func)(const FString&, UObject*))
    {
        BindDelegateActionSaveGSystemSignature(OnActionSaveComplete, Object, Func);
    }

protected:
    /** @protected Template function to bind a delegate with a specific signature FActionSaveGSystemSignature with Arg: const FString&,
     * UObject* */
    template <typename FuncClass>
    void BindDelegateActionSaveGSystemSignature(FActionSaveGSystemSignature& Delegate, FuncClass* Object, void (FuncClass::*Func)(const FString&, UObject*))
    {
        if (CLOG_SAVE_G_SYSTEM(Object == nullptr, "Object is nullptr")) return;
        if (CLOG_SAVE_G_SYSTEM(Func == nullptr, "Func is nullptr")) return;
        Delegate.AddUniqueDynamic(Object, Func);
    }

private:
    /** @private **/
    FActionSaveGSystemSignature OnActionLoadComplete;

    /** @private **/
    FActionSaveGSystemSignature OnActionSaveComplete;

#pragma endregion

#pragma region Data

private:
    /** @private **/
    TWeakObjectPtr<UUpdateSaveDataAsyncTask> ActionDataAsyncTask;

    /** @private Request Queue action save/load data **/
    TArray<FInitDataAsyncTask_SaveGSystem> RequestActionData;

    /** @private Key is tag. Value storage save data **/
    TMap<FString, FString> SaveGData;

#pragma endregion
};
