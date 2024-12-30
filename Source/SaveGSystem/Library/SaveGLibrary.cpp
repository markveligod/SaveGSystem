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

bool USaveGLibrary::DeserializeBoolProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        if (JsonObject->GetBoolField(Property->GetName()))
        {
            const bool Value = JsonObject->GetBoolField(Property->GetName());
            BoolProperty->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    return false;
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
    if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
    {
        // If there's no associated UEnum, treat it as a plain byte
        uint8 Value = ByteProperty->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), Value);
        return true;
    }
    return false;
}

bool USaveGLibrary::DeserializeByteProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
    {
        UEnum* Enum = EnumProperty->GetEnum();
        FString EnumName;
        if (Enum && JsonObject->TryGetStringField(FString::Printf(TEXT("%s_Name"), *Property->GetName()), EnumName))
        {
            int64 EnumValue = Enum->GetValueByName(FName(*EnumName));
            if (EnumValue != INDEX_NONE)
            {
                EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(ObjectData, EnumValue);
                return true;
            }
        }
    }
    else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
    {
        int32 Value;
        if (JsonObject->TryGetNumberField(Property->GetName(), Value))
        {
            ByteProperty->SetPropertyValue_InContainer(ObjectData, static_cast<uint8>(Value));
            return true;
        }
    }
    return false;
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

bool USaveGLibrary::DeserializeStringProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FStrProperty* StrProperty = CastField<FStrProperty>(Property))
    {
        FString Value;
        if (JsonObject->TryGetStringField(Property->GetName(), Value))
        {
            StrProperty->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        FString Value;
        if (JsonObject->TryGetStringField(Property->GetName(), Value))
        {
            NameProperty->SetPropertyValue_InContainer(ObjectData, FName(*Value));
            return true;
        }
    }
    else if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
    {
        FString Value;
        if (JsonObject->TryGetStringField(Property->GetName(), Value))
        {
            TextProperty->SetPropertyValue_InContainer(ObjectData, FText::FromString(Value));
            return true;
        }
    }
    return false;
}

bool USaveGLibrary::SerializeNumericProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (const FInt8Property* Int8Property = CastField<FInt8Property>(Property))
    {
        const int8 Value = Int8Property->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), Value);
        return true;
    }
    if (const FUInt16Property* UInt16Property = CastField<FUInt16Property>(Property))
    {
        const uint16 Value = UInt16Property->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), Value);
        return true;
    }
    if (const FInt16Property* Int16Property = CastField<FInt16Property>(Property))
    {
        const int16 Value = Int16Property->GetPropertyValue_InContainer(ObjectData);
        JsonObject->SetNumberField(Property->GetName(), Value);
        return true;
    }
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

bool USaveGLibrary::DeserializeNumericProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FInt8Property* Int8Property = CastField<FInt8Property>(Property))
    {
        int8 Value;
        if (JsonObject->TryGetNumberField(Property->GetName(), Value))
        {
            Int8Property->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (FInt16Property* Int16Property = CastField<FInt16Property>(Property))
    {
        int16 Value;
        if (JsonObject->TryGetNumberField(Property->GetName(), Value))
        {
            Int16Property->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (FUInt16Property* UInt16Property = CastField<FUInt16Property>(Property))
    {
        uint16 Value;
        if (JsonObject->TryGetNumberField(Property->GetName(), Value))
        {
            UInt16Property->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
    {
        int32 Value;
        if (JsonObject->TryGetNumberField(Property->GetName(), Value))
        {
            IntProperty->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (const FUInt32Property* UInt32Property = CastField<FUInt32Property>(Property))
    {
        uint32 Value;
        if (JsonObject->TryGetNumberField(UInt32Property->GetName(), Value))
        {
            IntProperty->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (const FInt64Property* Int64Property = CastField<FInt64Property>(Property))
    {
        int64 Value;
        if (JsonObject->TryGetNumberField(Int64Property->GetName(), Value))
        {
            IntProperty->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (const FUInt64Property* UInt64Property = CastField<FUInt64Property>(Property))
    {
        uint64 Value;
        if (JsonObject->TryGetNumberField(UInt64Property->GetName(), Value))
        {
            IntProperty->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        float Value;
        if (JsonObject->TryGetNumberField(FloatProperty->GetName(), Value))
        {
            IntProperty->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
    {
        double Value;
        if (JsonObject->TryGetNumberField(Property->GetName(), Value))
        {
            DoubleProperty->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    return false;
}

bool USaveGLibrary::SerializeObjectProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (const FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
    {
        FClassProperty::TCppType ClassValue = ClassProperty->GetPropertyValue_InContainer(ObjectData);
        if (ClassValue.Get())
        {
            // Serialize the class name
            JsonObject->SetStringField(Property->GetName(), ClassValue->GetName());
            return true;
        }
    }
    else if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
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

bool USaveGLibrary::DeserializeObjectProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FClassProperty* ClassProperty = CastField<FClassProperty>(Property))
    {
        FString ClassPath;
        if (JsonObject->TryGetStringField(Property->GetName(), ClassPath))
        {
            UClass* LoadedClass = StaticLoadClass(UObject::StaticClass(), nullptr, *ClassPath);
            if (LoadedClass)
            {
                ClassProperty->SetPropertyValue_InContainer(ObjectData, LoadedClass);
                return true;
            }
        }
    }
    else if (FObjectProperty* ObjectProperty = CastField<FObjectProperty>(Property))
    {
        const TSharedPtr<FJsonObject>* ObjectJson;
        if (JsonObject->TryGetObjectField(Property->GetName(), ObjectJson))
        {
            UObject* SubObject = ObjectProperty->GetObjectPropertyValue_InContainer(ObjectData);
            if (!SubObject)
            {
                SubObject = NewObject<UObject>(GetTransientPackage(), ObjectProperty->PropertyClass);
                ObjectProperty->SetObjectPropertyValue_InContainer(ObjectData, SubObject);
            }

            for (TFieldIterator<FProperty> It(SubObject->GetClass()); It; ++It)
            {
                FProperty* SubProperty = *It;
                DeserializeSubProperty(SubProperty, SubObject, JsonObject);
            }
            return true;
        }
    }
    else if (FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
    {
        FString AssetPath;
        if (JsonObject->TryGetStringField(Property->GetName(), AssetPath))
        {
            const TProperty<FSoftObjectPtr, FObjectPropertyBase>::TCppType SoftClass(AssetPath);
            SoftClassProperty->SetPropertyValue_InContainer(ObjectData, SoftClass);
            return true;
        }
    }
    else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
    {
        FString AssetPath;
        if (JsonObject->TryGetStringField(Property->GetName(), AssetPath))
        {
            const TProperty<FSoftObjectPtr, FObjectPropertyBase>::TCppType SoftObject(AssetPath);
            SoftObjectProperty->SetPropertyValue_InContainer(ObjectData, SoftObject);
            return true;
        }
    }
    else if (FWeakObjectProperty* WeakObjectProperty = CastField<FWeakObjectProperty>(Property))
    {
        FString ObjectPath;
        if (JsonObject->TryGetStringField(Property->GetName(), ObjectPath))
        {
            UObject* LoadedObject = FindObject<UObject>(nullptr, *ObjectPath);
            if (!LoadedObject)
            {
                LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath);
            }

            if (LoadedObject)
            {
                FWeakObjectPtr WeakObject = LoadedObject;
                WeakObjectProperty->SetPropertyValue_InContainer(ObjectData, WeakObject);
                return true;
            }
        }
    }
    else if (FLazyObjectProperty* LazyObjectProperty = CastField<FLazyObjectProperty>(Property))
    {
        FString ObjectPath;
        if (JsonObject->TryGetStringField(Property->GetName(), ObjectPath))
        {
            UObject* LoadedObject = FindObject<UObject>(nullptr, *ObjectPath);
            if (!LoadedObject)
            {
                LoadedObject = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath);
            }

            if (LoadedObject)
            {
                FLazyObjectPtr LazyObject;
                LazyObject = LoadedObject;
                LazyObjectProperty->SetPropertyValue_InContainer(ObjectData, LazyObject);
                return true;
            }
        }
    }

    return false;
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

            SerializeSubProperty(StructField, StructData, StructJsonObject);
        }

        JsonObject->SetObjectField(Property->GetName(), StructJsonObject);
        return true;
    }
    return false;
}

bool USaveGLibrary::DeserializeStructProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
    {
        const TSharedPtr<FJsonObject>* StructJson;
        if (JsonObject->TryGetObjectField(Property->GetName(), StructJson))
        {
            void* StructData = StructProperty->ContainerPtrToValuePtr<void>(ObjectData);
            UStruct* Struct = StructProperty->Struct;
            for (TFieldIterator<FProperty> It(Struct); It; ++It)
            {
                FProperty* StructField = *It;
                if (!StructField) continue;
                if (!StructField->HasMetaData("SaveGame")) continue;
                DeserializeSubProperty(StructField, StructData, *StructJson);
            }
            return true;
        }
    }

    return false;
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

            TSharedPtr<FJsonObject> JsonObjectValue = MakeShared<FJsonObject>();
            SerializeSubProperty(InnerProperty, ElementPtr, JsonObjectValue);
            if (JsonObjectValue.IsValid())
            {
                JsonArray.Add(MakeShared<FJsonValueObject>(JsonObjectValue));
            }
        }

        JsonObject->SetArrayField(Property->GetName(), JsonArray);
        return true;
    }
    return false;
}

bool USaveGLibrary::DeserializeArrayProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FArrayProperty* ArrayProperty = CastField<FArrayProperty>(Property))
    {
        const TArray<TSharedPtr<FJsonValue>>* JsonArray;
        if (JsonObject->TryGetArrayField(Property->GetName(), JsonArray))
        {
            FScriptArrayHelper Helper(ArrayProperty, ArrayProperty->ContainerPtrToValuePtr<void>(ObjectData));
            Helper.Resize(JsonArray->Num());

            for (int32 i = 0; i < JsonArray->Num(); ++i)
            {
                const TSharedPtr<FJsonValue>& JsonValue = (*JsonArray)[i];
                void* ElementPtr = Helper.GetRawPtr(i);
                TSharedPtr<FJsonObject>* JsonObjectValue;
                if (JsonValue->TryGetObject(JsonObjectValue))
                {
                    DeserializeSubProperty(ArrayProperty->Inner, ElementPtr, *JsonObjectValue);
                }
            }
            return true;
        }
    }
    return false;
}

void USaveGLibrary::SerializeSubProperty(FProperty* SubProperty, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (SerializeBoolProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeStringProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeNumericProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeStructProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeArrayProperty(SubProperty, ObjectData, JsonObject)) return;
}

void USaveGLibrary::DeserializeSubProperty(FProperty* SubProperty, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (DeserializeBoolProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeStringProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeNumericProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeStructProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeArrayProperty(SubProperty, ObjectData, JsonObject)) return;
}
