// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
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

#pragma endregion
    
#pragma region Data

private:
    
    /** @private Key is tag. Value storage save data **/
    TMap<FString, FString> SaveGData;

#pragma endregion
};
