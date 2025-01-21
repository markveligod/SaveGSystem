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
    if (Compressor.IsError()) return false;

    // Serialize the uncompressed data into the compressor
    Compressor << SomeData;
    Compressor.Flush();
    return true;
}

bool USaveGLibrary::DecompressData(const TArray<uint8>& CompressedData, TArray<uint8>& OutData)
{
    FArchiveLoadCompressedProxy Decompressor(CompressedData, NAME_Zlib);
    if (Decompressor.IsError()) return false;

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

bool USaveGLibrary::SerializeBoolProperty(FProperty* Property, const void* ObjectData, FString& Str)
{
    if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        const bool Value = BoolProperty->GetPropertyValue_InContainer(ObjectData);
        Str = Value ? "true" : "false";
        return true;
    }
    return false;
}

bool USaveGLibrary::DeserializeBoolProperty(FProperty* Property, void* ObjectData, const FString& Str)
{
    if (const FBoolProperty* BoolProperty = CastField<FBoolProperty>(Property))
    {
        const bool bValue = Str == "true";
        BoolProperty->SetPropertyValue_InContainer(ObjectData, bValue);
        return true;
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
            int64 EnumValue = 0;
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

bool USaveGLibrary::SerializeByteProperty(FProperty* Property, const void* ObjectData, FString& Str)
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
            Str = FString::FromInt(EnumValue);
            return true;
        }
    }
    if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
    {
        // If there's no associated UEnum, treat it as a plain byte
        uint8 Value = ByteProperty->GetPropertyValue_InContainer(ObjectData);
        Str = FString::FromInt(Value);
        return true;
    }
    return false;
}

bool USaveGLibrary::DeserializeByteProperty(FProperty* Property, void* ObjectData, const FString& Str)
{
    if (FEnumProperty* EnumProperty = CastField<FEnumProperty>(Property))
    {
        UEnum* Enum = EnumProperty->GetEnum();
        if (Enum)
        {
            FNumericProperty* UnderlyingProperty = EnumProperty->GetUnderlyingProperty();
            int64 EnumValue = FCString::Atoi64(*Str);
            if (UnderlyingProperty)
            {
                void* PropertyValuePtr = EnumProperty->ContainerPtrToValuePtr<void>(ObjectData);
                UnderlyingProperty->SetIntPropertyValue(PropertyValuePtr, EnumValue);
                return true;
            }
        }
    }
    else if (const FByteProperty* ByteProperty = CastField<FByteProperty>(Property))
    {
        int32 Value = FCString::Atoi(*Str);
        ByteProperty->SetPropertyValue_InContainer(ObjectData, static_cast<uint8>(Value));
        return true;
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

bool USaveGLibrary::SerializeStringProperty(FProperty* Property, const void* ObjectData, FString& Str)
{
    if (const FStrProperty* StrProperty = CastField<FStrProperty>(Property))
    {
        const FString Value = StrProperty->GetPropertyValue_InContainer(ObjectData);
        Str = Value;
        return true;
    }
    if (const FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        const FName Value = NameProperty->GetPropertyValue_InContainer(ObjectData);
        Str = Value.ToString();
        return true;
    }
    if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
    {
        const FText Value = TextProperty->GetPropertyValue_InContainer(ObjectData);
        Str = Value.ToString();
        return true;
    }
    return false;
}

bool USaveGLibrary::DeserializeStringProperty(FProperty* Property, void* ObjectData, const FString& Str)
{
    if (FStrProperty* StrProperty = CastField<FStrProperty>(Property))
    {
        StrProperty->SetPropertyValue_InContainer(ObjectData, Str);
        return true;
    }
    if (FNameProperty* NameProperty = CastField<FNameProperty>(Property))
    {
        NameProperty->SetPropertyValue_InContainer(ObjectData, FName(*Str));
        return true;
    }
    if (FTextProperty* TextProperty = CastField<FTextProperty>(Property))
    {
        TextProperty->SetPropertyValue_InContainer(ObjectData, FText::FromString(Str));
        return true;
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

bool USaveGLibrary::SerializeNumericProperty(FProperty* Property, const void* ObjectData, FString& Str)
{
    if (const FInt8Property* Int8Property = CastField<FInt8Property>(Property))
    {
        const int8 Value = Int8Property->GetPropertyValue_InContainer(ObjectData);
        Str = FString::Printf(TEXT("%i"), Value);
        return true;
    }
    if (const FUInt16Property* UInt16Property = CastField<FUInt16Property>(Property))
    {
        const uint16 Value = UInt16Property->GetPropertyValue_InContainer(ObjectData);
        Str = FString::Printf(TEXT("%i"), Value);
        return true;
    }
    if (const FInt16Property* Int16Property = CastField<FInt16Property>(Property))
    {
        const int16 Value = Int16Property->GetPropertyValue_InContainer(ObjectData);
        Str = FString::Printf(TEXT("%i"), Value);
        return true;
    }
    if (const FIntProperty* IntProperty = CastField<FIntProperty>(Property))
    {
        const int32 Value = IntProperty->GetPropertyValue_InContainer(ObjectData);
        Str = FString::Printf(TEXT("%i"), Value);
        return true;
    }
    if (const FUInt32Property* UInt32Property = CastField<FUInt32Property>(Property))
    {
        const uint32 Value = UInt32Property->GetPropertyValue_InContainer(ObjectData);
        Str = FString::Printf(TEXT("%i"), Value);
        return true;
    }
    if (const FInt64Property* Int64Property = CastField<FInt64Property>(Property))
    {
        const int64 Value = Int64Property->GetPropertyValue_InContainer(ObjectData);
        Str = FString::Printf(TEXT("%lld"), Value);
        return true;
    }
    if (const FUInt64Property* UInt64Property = CastField<FUInt64Property>(Property))
    {
        const uint64 Value = UInt64Property->GetPropertyValue_InContainer(ObjectData);
        Str = FString::Printf(TEXT("%lld"), Value);
        return true;
    }
    if (const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        const float Value = FloatProperty->GetPropertyValue_InContainer(ObjectData);
        Str = FString::Printf(TEXT("%f"), Value);
        return true;
    }
    if (const FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
    {
        const double Value = DoubleProperty->GetPropertyValue_InContainer(ObjectData);
        Str = FString::Printf(TEXT("%f"), Value);
        return true;
    }
    return false;
}

bool USaveGLibrary::DeserializeNumericProperty(FProperty* Property, void* ObjectData, const FString& Str)
{
    if (FInt8Property* Int8Property = CastField<FInt8Property>(Property))
    {
        int8 Value = FCString::Atoi(*Str);
        Int8Property->SetPropertyValue_InContainer(ObjectData, Value);
        return true;
    }
    if (FInt16Property* Int16Property = CastField<FInt16Property>(Property))
    {
        int16 Value = FCString::Atoi(*Str);
        Int16Property->SetPropertyValue_InContainer(ObjectData, Value);
        return true;
    }
    if (FUInt16Property* UInt16Property = CastField<FUInt16Property>(Property))
    {
        uint16 Value = FCString::Atoi(*Str);
        UInt16Property->SetPropertyValue_InContainer(ObjectData, Value);
        return true;
    }
    if (FIntProperty* IntProperty = CastField<FIntProperty>(Property))
    {
        int32 Value = FCString::Atoi(*Str);
        IntProperty->SetPropertyValue_InContainer(ObjectData, Value);
        return true;
    }
    if (const FUInt32Property* UInt32Property = CastField<FUInt32Property>(Property))
    {
        uint32 Value = FCString::Atoi(*Str);
        UInt32Property->SetPropertyValue_InContainer(ObjectData, Value);
        return true;
    }
    if (const FInt64Property* Int64Property = CastField<FInt64Property>(Property))
    {
        int64 Value = FCString::Atoi64(*Str);
        Int64Property->SetPropertyValue_InContainer(ObjectData, Value);
        return true;
    }
    if (const FUInt64Property* UInt64Property = CastField<FUInt64Property>(Property))
    {
        uint64 Value = FCString::Atoi64(*Str);
        UInt64Property->SetPropertyValue_InContainer(ObjectData, Value);
        return true;
    }
    if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Property))
    {
        float Value = FCString::Atof(*Str);
        FloatProperty->SetPropertyValue_InContainer(ObjectData, Value);
        return true;
    }
    if (FDoubleProperty* DoubleProperty = CastField<FDoubleProperty>(Property))
    {
        double Value = FCString::Atod(*Str);
        DoubleProperty->SetPropertyValue_InContainer(ObjectData, Value);
        return true;
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
        else
        {
            // If the class reference is invalid, serialize it as null
            JsonObject->SetField(Property->GetName(), MakeShared<FJsonValueNull>());
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
        else
        {
            // If the object reference is invalid, serialize it as null
            JsonObject->SetField(Property->GetName(), MakeShared<FJsonValueNull>());
            return true;
        }
    }

    return false;
}

bool USaveGLibrary::DeserializeObjectProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FSoftClassProperty* SoftClassProperty = CastField<FSoftClassProperty>(Property))
    {
        FString AssetPath;
        if (JsonObject->TryGetStringField(Property->GetName(), AssetPath))
        {
            FSoftObjectPtr SoftClass(AssetPath);
            SoftClassProperty->SetPropertyValue_InContainer(ObjectData, SoftClass);
            return true;
        }
        else
        {
            // If the field is null, set the property to null
            SoftClassProperty->SetPropertyValue_InContainer(ObjectData, FSoftObjectPtr());
            return true;
        }
    }
    else if (FSoftObjectProperty* SoftObjectProperty = CastField<FSoftObjectProperty>(Property))
    {
        FString AssetPath;
        if (JsonObject->TryGetStringField(Property->GetName(), AssetPath))
        {
            FSoftObjectPtr SoftObject(AssetPath);
            SoftObjectProperty->SetPropertyValue_InContainer(ObjectData, SoftObject);
            return true;
        }
        else
        {
            // If the field is null, set the property to null
            SoftObjectProperty->SetPropertyValue_InContainer(ObjectData, FSoftObjectPtr());
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

FString USaveGLibrary::SerializeMapKeyValue(FProperty* Property, const void* Ptr)
{
    FString Result;
    if (SerializeBoolProperty(Property, Ptr, Result)) return Result;
    if (SerializeByteProperty(Property, Ptr, Result)) return Result;
    if (SerializeStringProperty(Property, Ptr, Result)) return Result;
    if (SerializeNumericProperty(Property, Ptr, Result)) return Result;

    return Result;
}

bool USaveGLibrary::DeserializeMapKeyValue(FProperty* Property, void* Ptr, const FString& DataString)
{
    if (DeserializeBoolProperty(Property, Ptr, DataString)) return true;
    if (DeserializeByteProperty(Property, Ptr, DataString)) return true;
    if (DeserializeStringProperty(Property, Ptr, DataString)) return true;
    if (DeserializeNumericProperty(Property, Ptr, DataString)) return true;

    return false;
}

bool USaveGLibrary::SerializeMapProperty(FProperty* Property, const void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
    {
        // Get the key and value properties
        FProperty* KeyProperty = MapProperty->KeyProp;
        FProperty* ValueProperty = MapProperty->ValueProp;

        // Get the map helper to access the map data
        FScriptMapHelper MapHelper(MapProperty, MapProperty->ContainerPtrToValuePtr<void>(ObjectData));

        // Create a JSON object to store the map
        TSharedPtr<FJsonObject> MapJsonObject = MakeShared<FJsonObject>();

        // Iterate over the map and serialize each key-value pair
        for (int32 Index = 0; Index < MapHelper.Num(); ++Index)
        {
            if (MapHelper.IsValidIndex(Index))
            {
                // Get the key and value pointers
                const void* PairPtr = MapHelper.GetPairPtr(Index);
                if (!PairPtr) continue;

                // Serialize the key into a string
                FString KeyString = SerializeMapKeyValue(KeyProperty, PairPtr);

                // Serialize the value into a JSON object
                TSharedPtr<FJsonObject> ValueJsonObject = MakeShared<FJsonObject>();
                SerializeSubProperty(ValueProperty, PairPtr, ValueJsonObject);

                // Add the key-value pair to the JSON object
                MapJsonObject->SetObjectField(KeyString, ValueJsonObject);
            }
        }

        // Add the serialized map to the parent JSON object
        JsonObject->SetObjectField(Property->GetName(), MapJsonObject);
        return true;
    }
    return false;
}

bool USaveGLibrary::DeserializeMapProperty(FProperty* Property, void* ObjectData, TSharedPtr<FJsonObject> JsonObject)
{
    if (FMapProperty* MapProperty = CastField<FMapProperty>(Property))
    {
        // Get the key and value properties
        FProperty* KeyProperty = MapProperty->KeyProp;
        FProperty* ValueProperty = MapProperty->ValueProp;

        // Get the map helper to access the map data
        FScriptMapHelper MapHelper(MapProperty, MapProperty->ContainerPtrToValuePtr<void>(ObjectData));

        // Clear the existing map
        MapHelper.EmptyValues();

        // Get the JSON object representing the map
        const TSharedPtr<FJsonObject>* MapJsonObject;
        if (JsonObject->TryGetObjectField(Property->GetName(), MapJsonObject))
        {
            // Iterate over the JSON object and deserialize each key-value pair
            for (const auto& Entry : (*MapJsonObject)->Values)
            {
                // Allocate space for the key and value
                int32 MapIndex = MapHelper.AddDefaultValue_Invalid_NeedsRehash();
                if (MapIndex == INDEX_NONE)
                {
                    continue;  // Skip if we couldn't add a new pair
                }

                // Get pointers to the key and value in the map
                void* PairPtr = MapHelper.GetPairPtr(MapIndex);
                if (!PairPtr) continue;

                // Deserialize the key
                if (!DeserializeMapKeyValue(KeyProperty, PairPtr, Entry.Key))
                {
                    // If the key couldn't be deserialized, remove the pair
                    MapHelper.RemoveAt(MapIndex);
                    continue;
                }

                // Deserialize the value
                const TSharedPtr<FJsonObject>* ValueJsonObject;
                if (Entry.Value->TryGetObject(ValueJsonObject))
                {
                    DeserializeSubProperty(ValueProperty, PairPtr, *ValueJsonObject);
                }
                else
                {
                    // If the value couldn't be deserialized, remove the pair
                    MapHelper.RemoveAt(MapIndex);
                    continue;
                }
            }

            // Rehash the map after adding all elements
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
