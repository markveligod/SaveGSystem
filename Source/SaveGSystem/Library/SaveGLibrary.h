// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "SaveGLibrary.generated.h"

/**
 * Library for working with SaveGSystem.
 * Provides functionality for:
 * 1. Data compression and decompression.
 * 2. Reading and writing data.
 * 3. Finding properties with specific metadata.
 * 4. Serializing and deserializing properties (String, Byte, Enum, Numeric, Object, Bool, Struct, Array, Map).
 */
UCLASS()
class SAVEGSYSTEM_API USaveGLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** @public Compresses a byte array using a compression algorithm. **/
    UFUNCTION(BlueprintCallable, Category = "SaveGLibrary | Compression")
    static bool CompressData(TArray<uint8>& SomeData, TArray<uint8>& OutData);

    /** @public Decompresses a byte array that was previously compressed. **/
    UFUNCTION(BlueprintCallable, Category = "SaveGLibrary | Compression")
    static bool DecompressData(const TArray<uint8>& CompressedData, TArray<uint8>& OutData);

    /** @public Converts a JSON object to a string representation. **/
    static FString ConvertJsonObjectToString(const TSharedPtr<FJsonObject>& JsonObject);

    /** @public Converts a JSON string to a JSON object. **/
    static TSharedPtr<FJsonObject> ConvertStringToJsonObject(const FString& JsonString);

    /** @public Converts a string to a byte array. **/
    static TArray<uint8> ConvertStringToByte(const FString& JsonString);

    /** @public Converts a byte array to a string. **/
    static FString ConvertByteToString(const TArray<uint8>& ByteArray);

    /** @public Retrieves all properties of an object that have the "SaveGame" metadata. **/
    static TArray<FProperty*> GetAllPropertyHasMetaSaveGame(const UObject* ObjectData);

    /** @public Retrieves all properties of an object that have a specific custom metadata. **/
    static TArray<FProperty*> GetAllPropertyHasCustomMeta(const UObject* ObjectData, const FName& MetaName);

    /** @public Serializes a boolean property to a JSON object. **/
    static bool SerializeBoolProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Deserializes a boolean property from a JSON object. **/
    static bool DeserializeBoolProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Serializes a boolean property to a FString. **/
    static bool SerializeBoolProperty(FProperty* Property, const void* ObjectData, FString& Str);

    /** @public Deserializes a boolean property from a FString. **/
    static bool DeserializeBoolProperty(FProperty* Property, void* ObjectData, const FString& Str);

    /** @public Serializes a byte property (or enum) to a JSON object. **/
    static bool SerializeByteProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Deserializes a byte property (or enum) from a JSON object. **/
    static bool DeserializeByteProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Serializes a byte property (or enum) to a FString. **/
    static bool SerializeByteProperty(FProperty* Property, const void* ObjectData, FString& Str);

    /** @public Deserializes a byte property (or enum) from a FString. **/
    static bool DeserializeByteProperty(FProperty* Property, void* ObjectData, const FString& Str);

    /** @public Serializes a string, name, or text property to a JSON object. **/
    static bool SerializeStringProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Deserializes a string, name, or text property from a JSON object. **/
    static bool DeserializeStringProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Serializes a string, name, or text property to a FString. **/
    static bool SerializeStringProperty(FProperty* Property, const void* ObjectData, FString& Str);

    /** @public Deserializes a string, name, or text property from a FString. **/
    static bool DeserializeStringProperty(FProperty* Property, void* ObjectData, const FString& Str);

    /** @public Serializes a numeric property (int, float, double, etc.) to a JSON object. **/
    static bool SerializeNumericProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Deserializes a numeric property (int, float, double, etc.) from a JSON object. **/
    static bool DeserializeNumericProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Serializes a numeric property (int, float, double, etc.) to a FString. **/
    static bool SerializeNumericProperty(FProperty* Property, const void* ObjectData, FString& Str);

    /** @public Deserializes a numeric property (int, float, double, etc.) from a FString. **/
    static bool DeserializeNumericProperty(FProperty* Property, void* ObjectData, const FString& Str);

    /** @public Serializes an object property (UObject, soft object, etc.) to a JSON object. **/
    static bool SerializeObjectProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Deserializes an object property (UObject, soft object, etc.) from a JSON object. **/
    static bool DeserializeObjectProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Serializes a struct property to a JSON object. **/
    static bool SerializeStructProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Deserializes a struct property from a JSON object. **/
    static bool DeserializeStructProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Serializes an array property to a JSON object. **/
    static bool SerializeArrayProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Deserializes an array property from a JSON object. **/
    static bool DeserializeArrayProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Serializes a map key or value based on its property type. **/
    static FString SerializeMapKeyValue(FProperty* Property, const void* Ptr);

    /** @public Deserializes a map key or value based on its property type. **/
    static bool DeserializeMapKeyValue(FProperty* Property, void* Ptr, const FString& DataString);

    /** @public Serializes a map property to a JSON object. **/
    static bool SerializeMapProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Deserializes a map property from a JSON object. **/
    static bool DeserializeMapProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Serializes a sub-property (recursively handles nested properties). **/
    static void SerializeSubProperty(FProperty* SubProperty, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Deserializes a sub-property (recursively handles nested properties). **/
    static void DeserializeSubProperty(FProperty* SubProperty, void* ObjectData, TSharedPtr<FJsonObject> JsonObject);

    /** @public Validates a file name by removing invalid characters. **/
    static FString ValidateFileName(const FString& FileName);
};