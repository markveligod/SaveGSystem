// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SaveGSystem/Data/SaveGSystemDataTypes.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "SaveGSubSystem.generated.h"

/**
 * 
 */
UCLASS()
class SAVEGSYSTEM_API USaveGSubSystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

#pragma region Actions

public:

    /** @public Creates or updates data for all properties of an object **/
    UFUNCTION(BlueprintCallable)
    void UpdateSaveData(FString Tag, UObject* SavedObject);

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

protected:

    /** @protected  **/
    virtual FString GenerateSaveFileName();

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
    
    /** @private Key is tag. Value storage save data **/
    TMap<FString, FString> SaveGData;

#pragma endregion
};
