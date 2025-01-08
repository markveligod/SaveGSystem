// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "SaveGSettings.generated.h"

/**
 *
 */
UCLASS(Config = "Game", defaultconfig, meta = (DisplayName = "SaveG System Settings"))
class SAVEGSYSTEM_API USaveGSettings : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    /** @public Getting status data saving to a JSON file **/
    UFUNCTION(BlueprintCallable, Category = "General Settings")
    static bool IsEnableDataJSONFileStatic();

    /** @public Getting status data saving to a JSON file **/
    bool IsEnableDataJSONFile() const { return bEnableSaveDataJSONFile; }

private:
    /** @private Enable data saving to a JSON file **/
    UPROPERTY(Config, EditAnywhere, Category = "General Settings")
    bool bEnableSaveDataJSONFile{false};
};
