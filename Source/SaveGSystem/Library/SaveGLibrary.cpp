// Fill out your copyright notice in the Description page of Project Settings.

#include "SaveGLibrary.h"
#include "Compression/CompressedBuffer.h"
#include "SaveGSystem/Data/SaveGSystemDataTypes.h"

void USaveGLibrary::CompressData(const TArray<uint8>& SomeData, TArray<uint8>& OutData)
{
    // Create an FSharedBuffer from the uncompressed data
    const FSharedBuffer UncompressedBuffer = FSharedBuffer::MakeView(SomeData.GetData(), SomeData.Num());

    // Compress the data with the desired compression level
    const FCompressedBuffer CompressedBuffer =
        FCompressedBuffer::Compress(UncompressedBuffer, ECompressedBufferCompressor::Kraken, ECompressedBufferCompressionLevel::Normal);

    // Retrieve the compressed data
    const FSharedBuffer SharedCompressedData = CompressedBuffer.GetCompressed().ToShared();
    const void* DataPtr = SharedCompressedData.GetData();
    const int64 DataSize = SharedCompressedData.GetSize();

    // Copy the compressed data to the output array
    OutData.SetNumUninitialized(DataSize);
    FMemory::Memcpy(OutData.GetData(), DataPtr, DataSize);
}

bool USaveGLibrary::DecompressData(const TArray<uint8>& CompressedData, TArray<uint8>& OutData)
{
    // Create an FSharedBuffer from the compressed data
    const FSharedBuffer SharedCompressedData = FSharedBuffer::MakeView(CompressedData.GetData(), CompressedData.Num());

    // Create a compressed buffer from the shared buffer
    const FCompressedBuffer CompressedBuffer = FCompressedBuffer::FromCompressed(SharedCompressedData);

    // Decompress the data
    const FSharedBuffer DecompressedBuffer = CompressedBuffer.Decompress();

    // Retrieve the decompressed data
    const void* DataPtr = DecompressedBuffer.GetData();
    const int64 DataSize = DecompressedBuffer.GetSize();

    // Copy the decompressed data to the output array
    OutData.SetNumUninitialized(DataSize);
    FMemory::Memcpy(OutData.GetData(), DataPtr, DataSize);

    return true;
}

TArray<FProperty*> USaveGLibrary::GetAllPropertyHasMetaSaveGame(const UObject* ObjectData)
{
    TArray<FProperty*> Properties;
    for (TFieldIterator<FProperty> PropIt(ObjectData->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        // Check if the property is marked with SaveGame metadata
        if (Property->HasMetaData(TEXT("SaveGame")))
        {
            Properties.Add(Property);
        }
    }
    return Properties;
}

bool USaveGLibrary::SerializeStringProperty(FProperty* Property, const UObject* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
    {
        const FString Value = StrProperty->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetStringField(Property->GetName(), Value);
        return true;
    }
    if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        const FName Value = NameProperty->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetStringField(Property->GetName(), Value.ToString());
        return true;
    }
    if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
    {
        const FText Value = TextProperty->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetStringField(Property->GetName(), Value.ToString());  // Convert FText to FString
        return true;
    }
    return false;
}

bool USaveGLibrary::SerializeNumericProperty(FProperty* Property, const UObject* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (const FIntProperty* IntProperty = CastField<FIntProperty>(Property))
    {
        const int32 Value = IntProperty->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), Value);
        return true;
    }
    if (const FUInt32Property* UInt32Property = CastField<FUInt32Property>(Property))
    {
        const uint32 Value = UInt32Property->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), Value);
        return true;
    }
    if (const FInt64Property* Int64Property = CastField<FInt64Property>(Property))
    {
        const int64 Value = Int64Property->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), static_cast<double>(Value));  // JSON uses double for numbers
        return true;
    }
    if (const FUInt64Property* UInt64Property = CastField<FUInt64Property>(Property))
    {
        const uint64 Value = UInt64Property->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), static_cast<double>(Value));  // Convert to double
        return true;
    }
    if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        const float Value = FloatProperty->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), Value);
        return true;
    }
    if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
    {
        const double Value = DoubleProperty->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), Value);
        return true;
    }
    return false;
}
