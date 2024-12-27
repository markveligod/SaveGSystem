// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveGLibrary.generated.h"

/**
 * Library for working with SaveGSystem
 * 1. Data compression
 * 2. Data read/write
 * 3. Find Property
 * 4. Serialize Property String/Numeric
 */
UCLASS()
class SAVEGSYSTEM_API USaveGLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** @public  **/
    UFUNCTION(BlueprintCallable, Category = "SaveGLibrary | Compression")
    static void CompressData(const TArray<uint8>& SomeData, TArray<uint8>& OutData);

    /** @public  **/
    UFUNCTION(BlueprintCallable, Category = "SaveGLibrary | Compression")
    static bool DecompressData(const TArray<uint8>& CompressedData, TArray<uint8>& OutData);

    /** @public  **/
    static TArray<FProperty*> GetAllPropertyHasMetaSaveGame(const UObject* ObjectData);

    /** @public  **/
    static bool SerializeStringProperty(FProperty* Property, const UObject* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool SerializeNumericProperty(FProperty* Property, const UObject* ObjectData, TSharedPtr<FJsonObject> JsonObject);
};
