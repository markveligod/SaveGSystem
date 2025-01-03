// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "SaveGInterface.generated.h"

// This class does not need to be modified.
UINTERFACE()
class USaveGInterface : public UInterface
{
    GENERATED_BODY()
};

/**
 * 
 */
class SAVEGSYSTEM_API ISaveGInterface
{
    GENERATED_BODY()

    // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveGInterface")
    void PreSave();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveGInterface")
    void PostSave();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveGInterface")
    void PreLoad();

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "SaveGInterface")
    void PostLoad();
};