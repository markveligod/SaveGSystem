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

bool USaveGLibrary::SerializeBoolProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        const bool Value = BoolProperty->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetBoolField(Property->GetName(), Value);
        return true;
    }
    return false;
}

TSharedPtr<FJsonValue> USaveGLibrary::SerializeBoolProperty(FProperty* Property, const void* ObjectData)
{
    if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        const bool Value = BoolProperty->GetPropertyValue_InContainer(ObjectData);
        return MakeShared<FJsonValueBoolean>(Value);
    }
    return {};
}

bool USaveGLibrary::SerializeByteProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
    {
        // Get the UEnum object associated with the property
        UEnum* Enum = EnumProperty->GetEnum();
        if (Enum)
        {
            // Get the enum value as an integer
            int64 EnumValue = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue_InContainer(ObjectData);

            // Option 1: Store as the numeric value
            JsonObject->SetNumberField(Property->GetName(), EnumValue);

            // Option 2: Store as the name (human-readable)
            FString EnumName = Enum->GetNameStringByValue(EnumValue);
            JsonObject->SetStringField(FString::Printf(TEXT("%s_Name"), *Property->GetName()), EnumName);
            return true;
        }
    }
    if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
    {
        // If there's no associated UEnum, treat it as a plain byte
        uint8 Value = ByteProperty->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), Value);
        return true;
    }
    return false;
}

TSharedPtr<FJsonValue> USaveGLibrary::SerializeByteProperty(FProperty* Property, const void* ObjectData)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
    {
        // Get the UEnum object associated with the property
        UEnum* Enum = EnumProperty->GetEnum();
        if (Enum)
        {
            // Get the enum value as an integer
            int64 EnumValue = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue_InContainer(ObjectData);

            // Option 1: Store as the numeric value
            JsonObject->SetNumberField(Property->GetName(), EnumValue);

            // Option 2: Store as the name (human-readable)
            FString EnumName = Enum->GetNameStringByValue(EnumValue);
            JsonObject->SetStringField(FString::Printf(TEXT("%s_Name"), *Property->GetName()), EnumName);
            return MakeShared<FJsonValueObject>(JsonObject);
        }
    }
    if (FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
    {
        // If there's no associated UEnum, treat it as a plain byte
        uint8 Value = ByteProperty->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), Value);
        return MakeShared<FJsonValueObject>(JsonObject);
    }
    return {};
}

bool USaveGLibrary::SerializeStringProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
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
        JsonObject->SetStringField(Property->GetName(), Value.ToString());
        return true;
    }
    return false;
}

TSharedPtr<FJsonValue> USaveGLibrary::SerializeStringProperty(FProperty* Property, const void* ObjectData)
{
    if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
    {
        const FString Value = StrProperty->GetPropertyValue_InContainer(ObjectData);
        return MakeShared<FJsonValueString>(Value);
    }
    if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        const FName Value = NameProperty->GetPropertyValue_InContainer(ObjectData);
        return MakeShared<FJsonValueString>(Value.ToString());
    }
    if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
    {
        const FText Value = TextProperty->GetPropertyValue_InContainer(ObjectData);
        return MakeShared<FJsonValueString>(Value.ToString());
    }
    return {};
}

bool USaveGLibrary::SerializeNumericProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
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

TSharedPtr<FJsonValue> USaveGLibrary::SerializeNumericProperty(FProperty* Property, const void* ObjectData)
{
    if (const FIntProperty* IntProperty = CastField<FIntProperty>(Property))
    {
        const int32 Value = IntProperty->GetPropertyValue_InContainer(ObjectData);
        return MakeShared<FJsonValueNumber>(Value);
    }
    if (const FUInt32Property* UInt32Property = CastField<FUInt32Property>(Property))
    {
        const uint32 Value = UInt32Property->GetPropertyValue_InContainer(ObjectData);
        return MakeShared<FJsonValueNumber>(Value);
    }
    if (const FInt64Property* Int64Property = CastField<FInt64Property>(Property))
    {
        const int64 Value = Int64Property->GetPropertyValue_InContainer(ObjectData);
        return MakeShared<FJsonValueNumber>(Value);
    }
    if (const FUInt64Property* UInt64Property = CastField<FUInt64Property>(Property))
    {
        const uint64 Value = UInt64Property->GetPropertyValue_InContainer(ObjectData);
        return MakeShared<FJsonValueNumber>(Value);
    }
    if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        const float Value = FloatProperty->GetPropertyValue_InContainer(ObjectData);
        return MakeShared<FJsonValueNumber>(Value);
    }
    if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
    {
        const double Value = DoubleProperty->GetPropertyValue_InContainer(ObjectData);
        return MakeShared<FJsonValueNumber>(Value);
    }
    return {};
}

bool USaveGLibrary::SerializeObjectProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
    {
        if (const UObject* ObjectValue = ObjectProperty->GetObjectPropertyValue_InContainer(ObjectData))
        {
            const FString ObjectName = ObjectValue->GetName();
            JsonObject->SetStringField(Property->GetName(), ObjectName);

            // Serialize the object's properties recursively
            TSharedPtr<FJsonObject> ObjectJson = MakeShared<FJsonObject>();
            TArray<FProperty*> AllSaveProperty = GetAllPropertyHasMetaSaveGame(ObjectValue);
            for (FProperty* SubProperty : AllSaveProperty)
            {
                if (!SubProperty) continue;
                SerializeSubProperty(SubProperty, ObjectValue, ObjectJson);
            }

            JsonObject->SetObjectField(Property->GetName(), ObjectJson);
            return true;
        }
    }
    else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Property))
    {
        FWeakObjectPtr WeakObject = WeakObjectProperty->GetPropertyValue_InContainer(ObjectData);

        if (WeakObject.IsValid())
        {
            // Serialize basic information about the referenced object
            JsonObject->SetStringField(Property->GetName(), WeakObject.Get()->GetName());
            return true;
        }
    }
    else if (FLazyObjectProperty* LazyObjectProperty = CastField<FLazyObjectProperty>(Property))
    {
        FLazyObjectPtr LazyObject = LazyObjectProperty->GetPropertyValue_InContainer(ObjectData);

        if (UObject* ResolvedObject = LazyObject.Get())
        {
            // Serialize basic information about the resolved object
            JsonObject->SetStringField(Property->GetName(), ResolvedObject->GetName());
            return true;
        }
    }
    else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
    {
        FClassProperty::TCppType ClassValue = ClassProperty->GetPropertyValue_InContainer(ObjectData);
        if (ClassValue.Get())
        {
            // Serialize the class name
            JsonObject->SetStringField(Property->GetName(), ClassValue->GetName());
            return true;
        }
    }
    else if (FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
    {
        FSoftObjectPtr SoftClass = SoftClassProperty->GetPropertyValue_InContainer(ObjectData);

        if (SoftClass.IsValid())
        {
            // Serialize the class's asset path
            FString ClassPath = SoftClass.ToString();
            JsonObject->SetStringField(Property->GetName(), ClassPath);
            return true;
        }
    }
    else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
    {
        FSoftObjectPtr SoftObject = SoftObjectProperty->GetPropertyValue_InContainer(ObjectData);

        if (SoftObject.IsValid())
        {
            // Serialize the object's asset path
            const FString AssetPath = SoftObject.ToString();
            JsonObject->SetStringField(Property->GetName(), AssetPath);
            return true;
        }
    }

    // If the class reference is null
    JsonObject->SetField(Property->GetName(), MakeShared<FJsonValueNull>());
    return false;
}

TSharedPtr<FJsonValue> USaveGLibrary::SerializeObjectProperty(FProperty* Property, const void* ObjectData)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
    {
        if (const UObject* ObjectValue = ObjectProperty->GetObjectPropertyValue_InContainer(ObjectData))
        {
            const FString ObjectName = ObjectValue->GetName();
            JsonObject->SetStringField(Property->GetName(), ObjectName);

            // Serialize the object's properties recursively
            TSharedPtr<FJsonObject> ObjectJson = MakeShared<FJsonObject>();
            TArray<FProperty*> AllSaveProperty = GetAllPropertyHasMetaSaveGame(ObjectValue);
            for (FProperty* SubProperty : AllSaveProperty)
            {
                if (!SubProperty) continue;
                SerializeSubProperty(SubProperty, ObjectValue, JsonObject);
            }

            JsonObject->SetObjectField(Property->GetName(), ObjectJson);
            return MakeShared<FJsonValueObject>(JsonObject);
        }
    }
    else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Property))
    {
        FWeakObjectPtr WeakObject = WeakObjectProperty->GetPropertyValue_InContainer(ObjectData);

        if (WeakObject.IsValid())
        {
            // Serialize basic information about the referenced object
            JsonObject->SetStringField(Property->GetName(), WeakObject.Get()->GetName());
            return MakeShared<FJsonValueObject>(JsonObject);
        }
    }
    else if (FLazyObjectProperty* LazyObjectProperty = CastField<FLazyObjectProperty>(Property))
    {
        FLazyObjectPtr LazyObject = LazyObjectProperty->GetPropertyValue_InContainer(ObjectData);

        if (UObject* ResolvedObject = LazyObject.Get())
        {
            // Serialize basic information about the resolved object
            JsonObject->SetStringField(Property->GetName(), ResolvedObject->GetName());
            return MakeShared<FJsonValueObject>(JsonObject);
        }
    }
    else if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
    {
        FClassProperty::TCppType ClassValue = ClassProperty->GetPropertyValue_InContainer(ObjectData);
        if (ClassValue.Get())
        {
            // Serialize the class name
            JsonObject->SetStringField(Property->GetName(), ClassValue->GetName());
            return MakeShared<FJsonValueObject>(JsonObject);
        }
    }
    else if (FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
    {
        FSoftObjectPtr SoftClass = SoftClassProperty->GetPropertyValue_InContainer(ObjectData);

        if (SoftClass.IsValid())
        {
            // Serialize the class's asset path
            FString ClassPath = SoftClass.ToString();
            JsonObject->SetStringField(Property->GetName(), ClassPath);
            return MakeShared<FJsonValueObject>(JsonObject);
        }
    }
    else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
    {
        FSoftObjectPtr SoftObject = SoftObjectProperty->GetPropertyValue_InContainer(ObjectData);

        if (SoftObject.IsValid())
        {
            // Serialize the object's asset path
            const FString AssetPath = SoftObject.ToString();
            JsonObject->SetStringField(Property->GetName(), AssetPath);
            return MakeShared<FJsonValueObject>(JsonObject);
        }
    }
    return {};
}

bool USaveGLibrary::SerializeStructProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
    {
        // Create a JSON object for the struct
        TSharedPtr<FJsonObject> StructJsonObject = MakeShared<FJsonObject>();

        // Get the UStruct definition
        UStruct* Struct = StructProperty->Struct;

        // Get the pointer to the struct instance
        void const* StructData = StructProperty->ContainerPtrToValuePtr<void>(ObjectData);

        // Iterate over the struct fields
        for (TFieldIterator<FProperty> It(Struct); It; ++It)
        {
            FProperty* StructField = *It;
            if (!StructField) continue;
            if (!StructField->HasMetaData("SaveGame")) continue;

            FString FieldName = StructField->GetName();

            SerializeSubProperty(StructField, StructData, StructJsonObject);
        }

        JsonObject->SetObjectField(Property->GetName(), StructJsonObject);
        return true;
    }
    return false;
}

TSharedPtr<FJsonValue> USaveGLibrary::SerializeStructProperty(FProperty* Property, const void* ObjectData)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
    {
        // Create a JSON object for the struct
        TSharedPtr<FJsonObject> StructJsonObject = MakeShared<FJsonObject>();

        // Get the UStruct definition
        UStruct* Struct = StructProperty->Struct;

        // Get the pointer to the struct instance
        void const* StructData = StructProperty->ContainerPtrToValuePtr<void>(ObjectData);

        // Iterate over the struct fields
        for (TFieldIterator<FProperty> It(Struct); It; ++It)
        {
            FProperty* StructField = *It;
            if (!StructField) continue;
            if (!StructField->HasMetaData("SaveGame")) continue;

            FString FieldName = StructField->GetName();

            SerializeSubProperty(StructField, StructData, StructJsonObject);
        }

        return MakeShared<FJsonValueObject>(JsonObject);
    }
    return {};
}

bool USaveGLibrary::SerializeArrayProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
    {
        // Get the inner property (type of the array elements)
        FProperty* InnerProperty = ArrayProperty->Inner;

        // Get the array pointer
        FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(ObjectData));

        TArray<TSharedPtr<FJsonValue>> JsonArray;
        for (int32 Index = 0; Index < ArrayHelper.Num(); Index++)
        {
            const void* ElementPtr = ArrayHelper.GetRawPtr(Index);
            if (!ElementPtr) continue;

            TSharedPtr<FJsonValue> JsonValue = SerializeElementProperty(InnerProperty, ElementPtr);
            if (JsonValue.IsValid())
            {
                JsonArray.Add(JsonValue);
            }
        }

        JsonObject->SetArrayField(Property->GetName(), JsonArray);
        return true;
    }
    return false;
}

TSharedPtr<FJsonValue> USaveGLibrary::SerializeArrayProperty(FProperty* Property, const void* ObjectData)
{
    if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
    {
        // Get the inner property (type of the array elements)
        FProperty* InnerProperty = ArrayProperty->Inner;

        // Get the array pointer
        FScriptArrayHelper ArrayHelper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(ObjectData));

        TArray<TSharedPtr<FJsonValue>> JsonArray;
        for (int32 Index = 0; Index < ArrayHelper.Num(); Index++)
        {
            const void* ElementPtr = ArrayHelper.GetRawPtr(Index);
            if (!ElementPtr) continue;

            TSharedPtr<FJsonValue> JsonValue = SerializeElementProperty(InnerProperty, ElementPtr);
            if (JsonValue.IsValid())
            {
                JsonArray.Add(JsonValue);
            }
        }

        return MakeShared<FJsonValueArray>(JsonArray);
    }
    return {};
}

bool USaveGLibrary::SerializeMapProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
    {
        // Get the key and value properties
        FProperty* KeyProperty = MapProperty->KeyProp;
        FProperty* ValueProperty = MapProperty->ValueProp;

        // Access the map using a helper
        FScriptMapHelper MapHelper(MapProperty, MapProperty->ContainerPtrToValuePtr<void>(ObjectData));

        TSharedPtr<FJsonObject> JsonMapObject = MakeShared<FJsonObject>();

        for (int32 Index = 0; Index < MapHelper.Num(); Index++)
        {
            if (MapHelper.IsValidIndex(Index))
            {
                const void* KeyPtr = MapHelper.GetKeyPtr(Index);
                const void* ValuePtr = MapHelper.GetValuePtr(Index);
                SerializeSubProperty(KeyProperty, KeyPtr, JsonMapObject);
                SerializeSubProperty(ValueProperty, ValuePtr, JsonMapObject);
            }
        }

        JsonObject->SetObjectField(Property->GetName(), JsonMapObject);
        return true;
    }
    return false;
}

TSharedPtr<FJsonValue> USaveGLibrary::SerializeMapProperty(FProperty* Property, const void* ObjectData)
{
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
    {
        // Get the key and value properties
        FProperty* KeyProperty = MapProperty->KeyProp;
        FProperty* ValueProperty = MapProperty->ValueProp;

        // Access the map using a helper
        FScriptMapHelper MapHelper(MapProperty, MapProperty->ContainerPtrToValuePtr<void>(ObjectData));

        TSharedPtr<FJsonObject> JsonMapObject = MakeShared<FJsonObject>();

        for (int32 Index = 0; Index < MapHelper.Num(); Index++)
        {
            if (MapHelper.IsValidIndex(Index))
            {
                const void* KeyPtr = MapHelper.GetKeyPtr(Index);
                const void* ValuePtr = MapHelper.GetValuePtr(Index);
                SerializeSubProperty(KeyProperty, KeyPtr, JsonMapObject);
                SerializeSubProperty(ValueProperty, ValuePtr, JsonMapObject);
            }
        }

        JsonObject->SetObjectField(Property->GetName(), JsonMapObject);
        return MakeShared<FJsonValueObject>(JsonObject);
    }
    return {};
}

void USaveGLibrary::SerializeSubProperty(FProperty* SubProperty, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (SerializeBoolProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeStringProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeNumericProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeStructProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeArrayProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeMapProperty(SubProperty, ObjectData, JsonObject)) return;
}

TSharedPtr<FJsonValue> USaveGLibrary::SerializeElementProperty(FProperty* InnerProperty, const void* ElementPtr)
{
    TSharedPtr<FJsonValue> JsonValue;
    JsonValue = SerializeBoolProperty(InnerProperty, ElementPtr);
    if (JsonValue.IsValid())
    {
        return JsonValue;
    }
    JsonValue = SerializeStringProperty(InnerProperty, ElementPtr);
    if (JsonValue.IsValid())
    {
        return JsonValue;
    }
    JsonValue = SerializeNumericProperty(InnerProperty, ElementPtr);
    if (JsonValue.IsValid())
    {
        return JsonValue;
    }
    JsonValue = SerializeStructProperty(InnerProperty, ElementPtr);
    if (JsonValue.IsValid())
    {
        return JsonValue;
    }
    JsonValue = SerializeObjectProperty(InnerProperty, ElementPtr);
    if (JsonValue.IsValid())
    {
        return JsonValue;
    }
    JsonValue = SerializeArrayProperty(InnerProperty, ElementPtr);
    if (JsonValue.IsValid())
    {
        return JsonValue;
    }
    JsonValue = SerializeMapProperty(InnerProperty, ElementPtr);
    if (JsonValue.IsValid())
    {
        return JsonValue;
    }

    return {};
}
