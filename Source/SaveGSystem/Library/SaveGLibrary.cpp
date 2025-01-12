// Fill out your copyright notice in the Description page of Project Settings.

#include "SaveGLibrary.h"
#include "Compression/CompressedBuffer.h"
#include "SaveGSystem/Data/SaveGSystemDataTypes.h"
#include "Serialization/ArchiveLoadCompressedProxy.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"

bool USaveGLibrary::CompressData(TArray<uint8>& SomeData, TArray<uint8>& OutData)
{
    // Compress the data
    FArchiveSaveCompressedProxy Compressor(OutData, NAME_Zlib);
    if (Compressor.IsError())
    {
        LOG_SAVE_G_SYSTEM(Error, "Failed to initialize compression archive.");
        return false;
    }

    // Serialize the uncompressed data into the compressor
    Compressor << SomeData;
    Compressor.Flush();
    return true;
}

bool USaveGLibrary::DecompressData(const TArray<uint8>& CompressedData, TArray<uint8>& OutData)
{
    FArchiveLoadCompressedProxy Decompressor(CompressedData, NAME_Zlib);
    if (Decompressor.IsError())
    {
        LOG_SAVE_G_SYSTEM(Error, "Failed to initialize decompression archive.");
        return false;
    }

    // Serialize the decompressed data
    Decompressor << OutData;
    return true;
}

FString USaveGLibrary::ConvertJsonObjectToString(const TSharedPtr<FJsonObject>& JsonObject)
{
    FString OutputString;
    if (JsonObject.IsValid())
    {
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
        if (FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer))
        {
            return OutputString;
        }
    }
    return {};
}

TSharedPtr<FJsonObject> USaveGLibrary::ConvertStringToJsonObject(const FString& JsonString)
{
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    if (FJsonSerializer::Deserialize(Reader, JsonObject))
    {
        return JsonObject;
    }

    LOG_SAVE_G_SYSTEM(Error, "Failed to deserialize json object. Msg: %s", *Reader->GetErrorMessage());
    return {};
}

TArray<uint8> USaveGLibrary::ConvertStringToByte(const FString& JsonString)
{
    TArray<uint8> ByteArray;
    FTCHARToUTF8 Converter(*JsonString);  // Convert TCHAR to UTF-8
    ByteArray.Append((const uint8*)(Converter.Get()), Converter.Length());
    return ByteArray;
}

FString USaveGLibrary::ConvertByteToString(const TArray<uint8>& ByteArray)
{
    if (ByteArray.Num() > 0)
    {
        return FString(UTF8_TO_TCHAR((const char*)(ByteArray.GetData())));
    }
    return {};
}

TArray<FProperty*> USaveGLibrary::GetAllPropertyHasMetaSaveGame(const UObject* ObjectData)
{
    TArray<FProperty*> Properties;
    for (TFieldIterator<FProperty> PropIt(ObjectData->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        if (!Property) continue;
        // Check if the property is marked with SaveGame metadata
        if (Property->HasMetaData(TEXT("SaveGame")) || Property->HasAnyPropertyFlags(CPF_SaveGame))
        {
            Properties.Add(Property);
        }
    }
    return Properties;
}

TArray<FProperty*> USaveGLibrary::GetAllPropertyHasCustomMeta(const UObject* ObjectData, const FName& MetaName)
{
    TArray<FProperty*> Properties;
    for (TFieldIterator<FProperty> PropIt(ObjectData->GetClass()); PropIt; ++PropIt)
    {
        FProperty* Property = *PropIt;
        // Check if the property is marked with SaveGame metadata
        if (Property->HasMetaData(MetaName))
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
        UEnum* Enum = EnumProperty->GetEnum();
        FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
        if (Enum && UnderlyingProperty)
        {
            // Cast to FNumericProperty and get the enum value
            const FNumericProperty* NumericProperty = CastFieldChecked<FNumericProperty>(UnderlyingProperty);
            const void* PropertyValuePtr = EnumProperty->ContainerPtrToValuePtr<void>(ObjectData);
            int64 EnumValue = NumericProperty->GetSignedIntPropertyValue(PropertyValuePtr);
            JsonObject->SetNumberField(Property->GetName(), EnumValue);
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
        if (Enum)
        {
            FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
            int64 EnumValue;
            if (UnderlyingProperty && JsonObject->TryGetNumberField(Property->GetName(), EnumValue))
            {
                void* PropertyValuePtr = EnumProperty->ContainerPtrToValuePtr<void>(ObjectData);
                UnderlyingProperty->SetIntPropertyValue(PropertyValuePtr, EnumValue);
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
            UInt32Property->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (const FInt64Property* Int64Property = CastField<FInt64Property>(Property))
    {
        int64 Value;
        if (JsonObject->TryGetNumberField(Int64Property->GetName(), Value))
        {
            Int64Property->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (const FUInt64Property* UInt64Property = CastField<FUInt64Property>(Property))
    {
        uint64 Value;
        if (JsonObject->TryGetNumberField(UInt64Property->GetName(), Value))
        {
            UInt64Property->SetPropertyValue_InContainer(ObjectData, Value);
            return true;
        }
    }
    else if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        float Value;
        if (JsonObject->TryGetNumberField(FloatProperty->GetName(), Value))
        {
            FloatProperty->SetPropertyValue_InContainer(ObjectData, Value);
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
    if (FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
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
    if (FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
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

    return false;
}

bool USaveGLibrary::SerializeStructProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (!Property || !ObjectData || !JsonObject.IsValid()) return false;

    if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
    {
        UStruct* Struct = StructProperty->Struct;
        if (!Struct) return false;

        // Create a JSON object for the struct
        TSharedPtr<FJsonObject> StructJsonObject = MakeShared<FJsonObject>();
        if (!StructJsonObject.IsValid()) return false;

        // Get the pointer to the struct instance
        void const* StructData = StructProperty->ContainerPtrToValuePtr<void>(ObjectData);
        if (!StructData) return false;

        // Iterate over the struct fields
        for (TFieldIterator<FProperty> It(Struct); It; ++It)
        {
            FProperty* StructField = *It;
            if (!StructField) continue;

            SerializeSubProperty(StructField, StructData, StructJsonObject);
        }

        JsonObject->SetObjectField(Property->GetName(), StructJsonObject);
        return true;
    }
    return false;
}

bool USaveGLibrary::DeserializeStructProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (!Property || !ObjectData || !JsonObject.IsValid()) return false;

    if (FStructProperty* StructProperty = CastField<FStructProperty>(Property))
    {
        UStruct* Struct = StructProperty->Struct;
        if (!Struct) return false;

        const TSharedPtr<FJsonObject>* StructJson;
        if (JsonObject->TryGetObjectField(Property->GetName(), StructJson))
        {
            void* StructData = StructProperty->ContainerPtrToValuePtr<void>(ObjectData);
            if (!StructData) return false;

            for (TFieldIterator<FProperty> It(Struct); It; ++It)
            {
                FProperty* StructField = *It;
                if (!StructField) continue;

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

FString USaveGLibrary::SerializeMapKey(FProperty* KeyProperty, const void* KeyPtr)
{
    if (KeyProperty->IsA<FStrProperty>())
    {
        return CastField<FStrProperty>(KeyProperty)->GetPropertyValue(KeyPtr);
    }
    else if (KeyProperty->IsA<FNameProperty>())
    {
        return CastField<FNameProperty>(KeyProperty)->GetPropertyValue(KeyPtr).ToString();
    }
    else if (KeyProperty->IsA<FIntProperty>())
    {
        return FString::FromInt(CastField<FIntProperty>(KeyProperty)->GetPropertyValue(KeyPtr));
    }
    else if (KeyProperty->IsA<FUInt32Property>())
    {
        return FString::FromInt(CastField<FUInt32Property>(KeyProperty)->GetPropertyValue(KeyPtr));
    }
    else if (KeyProperty->IsA<FInt64Property>())
    {
        return FString::Printf(TEXT("%lld"), CastField<FInt64Property>(KeyProperty)->GetPropertyValue(KeyPtr));
    }
    else if (KeyProperty->IsA<FUInt64Property>())
    {
        return FString::Printf(TEXT("%llu"), CastField<FUInt64Property>(KeyProperty)->GetPropertyValue(KeyPtr));
    }
    else if (KeyProperty->IsA<FFloatProperty>())
    {
        return FString::SanitizeFloat(CastField<FFloatProperty>(KeyProperty)->GetPropertyValue(KeyPtr));
    }
    else if (KeyProperty->IsA<FDoubleProperty>())
    {
        return FString::SanitizeFloat(CastField<FDoubleProperty>(KeyProperty)->GetPropertyValue(KeyPtr));
    }
    else if (KeyProperty->IsA<FEnumProperty>())
    {
        FEnumProperty* EnumProperty = CastField<FEnumProperty>(KeyProperty);
        int64 EnumValue = EnumProperty->GetUnderlyingProperty()->GetSignedIntPropertyValue(KeyPtr);
        return FString::FromInt(EnumValue);
    }

    return FString();
}

bool USaveGLibrary::DeserializeMapKey(FProperty* KeyProperty, void* KeyPtr, const FString& KeyString)
{
    if (KeyProperty->IsA<FStrProperty>())
    {
        CastField<FStrProperty>(KeyProperty)->SetPropertyValue(KeyPtr, KeyString);
        return true;
    }
    else if (KeyProperty->IsA<FNameProperty>())
    {
        CastField<FNameProperty>(KeyProperty)->SetPropertyValue(KeyPtr, FName(*KeyString));
        return true;
    }
    else if (KeyProperty->IsA<FIntProperty>())
    {
        int32 KeyValue;
        if (LexTryParseString(KeyValue, *KeyString))
        {
            CastField<FIntProperty>(KeyProperty)->SetPropertyValue(KeyPtr, KeyValue);
            return true;
        }
    }
    else if (KeyProperty->IsA<FUInt32Property>())
    {
        uint32 KeyValue;
        if (LexTryParseString(KeyValue, *KeyString))
        {
            CastField<FUInt32Property>(KeyProperty)->SetPropertyValue(KeyPtr, KeyValue);
            return true;
        }
    }
    else if (KeyProperty->IsA<FInt64Property>())
    {
        int64 KeyValue;
        if (LexTryParseString(KeyValue, *KeyString))
        {
            CastField<FInt64Property>(KeyProperty)->SetPropertyValue(KeyPtr, KeyValue);
            return true;
        }
    }
    else if (KeyProperty->IsA<FUInt64Property>())
    {
        uint64 KeyValue;
        if (LexTryParseString(KeyValue, *KeyString))
        {
            CastField<FUInt64Property>(KeyProperty)->SetPropertyValue(KeyPtr, KeyValue);
            return true;
        }
    }
    else if (KeyProperty->IsA<FFloatProperty>())
    {
        float KeyValue;
        if (LexTryParseString(KeyValue, *KeyString))
        {
            CastField<FFloatProperty>(KeyProperty)->SetPropertyValue(KeyPtr, KeyValue);
            return true;
        }
    }
    else if (KeyProperty->IsA<FDoubleProperty>())
    {
        double KeyValue;
        if (LexTryParseString(KeyValue, *KeyString))
        {
            CastField<FDoubleProperty>(KeyProperty)->SetPropertyValue(KeyPtr, KeyValue);
            return true;
        }
    }
    else if (KeyProperty->IsA<FEnumProperty>())
    {
        FEnumProperty* EnumProperty = CastField<FEnumProperty>(KeyProperty);
        int64 EnumValue;
        if (LexTryParseString(EnumValue, *KeyString))
        {
            EnumProperty->GetUnderlyingProperty()->SetIntPropertyValue(KeyPtr, EnumValue);
            return true;
        }
    }

    return false;
}

bool USaveGLibrary::SerializeMapProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
    {
        FProperty* KeyProperty = MapProperty->KeyProp;
        FProperty* ValueProperty = MapProperty->ValueProp;

        TSharedPtr<FJsonObject> MapJsonObject = MakeShared<FJsonObject>();

        FScriptMapHelper MapHelper(MapProperty, MapProperty->ContainerPtrToValuePtr<void>(ObjectData));

        for (int32 Index = 0; Index < MapHelper.Num(); Index++)
        {
            if (!MapHelper.IsValidIndex(Index)) continue;

            // Get the key and value pointers
            const void* KeyPtr = MapHelper.GetKeyPtr(Index);
            const void* ValuePtr = MapHelper.GetValuePtr(Index);

            // Serialize the key
            FString KeyString = SerializeMapKey(KeyProperty, KeyPtr);

            // Serialize the value
            TSharedPtr<FJsonObject> ValueJsonObject = MakeShared<FJsonObject>();
            SerializeSubProperty(ValueProperty, ValuePtr, ValueJsonObject);

            if (ValueJsonObject.IsValid())
            {
                MapJsonObject->SetObjectField(KeyString, ValueJsonObject);
            }
        }

        JsonObject->SetObjectField(Property->GetName(), MapJsonObject);
        return true;
    }
    return false;
}

bool USaveGLibrary::DeserializeMapProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
    {
        FProperty* KeyProperty = MapProperty->KeyProp;
        FProperty* ValueProperty = MapProperty->ValueProp;

        FScriptMapHelper MapHelper(MapProperty, MapProperty->ContainerPtrToValuePtr<void>(ObjectData));
        MapHelper.EmptyValues();

        const TSharedPtr<FJsonObject>* MapJsonObject;
        if (JsonObject->TryGetObjectField(Property->GetName(), MapJsonObject))
        {
            // Iterate over the JSON object's fields
            for (const auto& JsonPair : (*MapJsonObject)->Values)
            {
                const FString& KeyString = JsonPair.Key;
                const TSharedPtr<FJsonValue>& JsonValue = JsonPair.Value;

                int32 Index = MapHelper.AddDefaultValue_Invalid_NeedsRehash();

                void* KeyPtr = MapHelper.GetKeyPtr(Index);
                void* ValuePtr = MapHelper.GetValuePtr(Index);

                // Deserialize the key
                if (!DeserializeMapKey(KeyProperty, KeyPtr, KeyString))
                {
                    continue;  // Skip invalid keys
                }

                // Deserialize the value
                const TSharedPtr<FJsonObject>* ValueJsonObject;
                if (JsonValue->TryGetObject(ValueJsonObject))
                {
                    DeserializeSubProperty(ValueProperty, ValuePtr, *ValueJsonObject);
                }
            }

            MapHelper.Rehash();
            return true;
        }
    }
    return false;
}

void USaveGLibrary::SerializeSubProperty(FProperty* SubProperty, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (SerializeBoolProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeByteProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeStringProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeNumericProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeObjectProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeStructProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeArrayProperty(SubProperty, ObjectData, JsonObject)) return;
    if (SerializeMapProperty(SubProperty, ObjectData, JsonObject)) return;
}

void USaveGLibrary::DeserializeSubProperty(FProperty* SubProperty, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (DeserializeBoolProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeByteProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeStringProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeNumericProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeObjectProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeStructProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeArrayProperty(SubProperty, ObjectData, JsonObject)) return;
    if (DeserializeMapProperty(SubProperty, ObjectData, JsonObject)) return;
}

FString USaveGLibrary::ValidateFileName(const FString& FileName)
{
    // Find the position of the first dot in the string
    int32 DotPosition;
    if (FileName.FindChar('.', DotPosition))
    {
        // Return the substring from the start up to the dot
        return FileName.Left(DotPosition);
    }

    // If no dot is found, return the original string
    return FileName;
}
