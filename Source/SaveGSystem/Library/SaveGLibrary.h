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
 * 4. Serialize Property String/Numeric/Object/Bool/Struct/Array
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
    static FString ConvertJsonObjectToString(const TSharedPtr<FJsonObject>& JsonObject);

    /** @public  **/
    static TSharedPtr<FJsonObject> ConvertStringToJsonObject(const FString& JsonString);

    /** @public  **/
    static TArray<uint8> ConvertStringToByte(const FString& JsonString);

    /** @public  **/
    static FString ConvertByteToString(const TArray<uint8>& ByteArray);

    /** @public  **/
    static TArray<FProperty*> GetAllPropertyHasMetaSaveGame(const UObject* ObjectData);

    /** @public  **/
    static TArray<FProperty*> GetAllPropertyHasCustomMeta(const UObject* ObjectData, const FName& MetaName);

    /** @public  **/
    static bool SerializeBoolProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool DeserializeBoolProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);
    
    /** @public  **/
    static bool SerializeByteProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool DeserializeByteProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool SerializeStringProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool DeserializeStringProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);
    
    /** @public  **/
    static bool SerializeNumericProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool DeserializeNumericProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool SerializeObjectProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool DeserializeObjectProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool SerializeStructProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool DeserializeStructProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool SerializeArrayProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static bool DeserializeArrayProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static void SerializeSubProperty(FProperty* SubProperty, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public  **/
    static void DeserializeSubProperty(FProperty* SubProperty, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);
};
