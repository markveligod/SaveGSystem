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

    /** @public **/
    const FActionSaveGSystemSignature& GetActionLoadCompleteSignature() const { return OnActionLoadComplete; }

    /** @public **/
    const FActionSaveGSystemSignature& GetActionSaveCompleteSignature() const { return OnActionSaveComplete; }

protected:

    /** @protected **/
    UPROPERTY(BlueprintAssignable)
    FActionSaveGSystemSignature OnActionLoadComplete;

    /** @protected **/
    UPROPERTY(BlueprintAssignable)
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
