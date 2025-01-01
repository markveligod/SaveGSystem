// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGSubSystem.h"
#include "SaveGSystem/Library/SaveGLibrary.h"
#include "Serialization/ArchiveSaveCompressedProxy.h"

#pragma region Actions

void USaveGSubSystem::UpdateSaveData(FString Tag, UObject* SavedObject)
{
    if (CLOG_SAVE_G_SYSTEM(SavedObject == nullptr, "Saved Object is nullptr")) return;
    if (CLOG_SAVE_G_SYSTEM(Tag.IsEmpty(), "Tag is empty")) return;

    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    TArray<FProperty*> Properties = USaveGLibrary::GetAllPropertyHasMetaSaveGame(SavedObject);
    for (FProperty* Property : Properties)
    {
        USaveGLibrary::SerializeSubProperty(Property, SavedObject, JsonObject);
    }
    FString JsonString = USaveGLibrary::ConvertJsonObjectToString(JsonObject);
    if (SaveGData.Contains(Tag))
    {
        SaveGData[Tag] = JsonString;
        LOG_SAVE_G_SYSTEM(Display, "Updated Saved Data: Tag - [%s] | Data - [%s]", *Tag, *JsonString);
    }
    else
    {
        SaveGData.Add(Tag, JsonString);
        LOG_SAVE_G_SYSTEM(Display, "Create Saved Data: Tag - [%s] | Data - [%s]", *Tag, *JsonString);
    }
    OnActionSaveComplete.Broadcast(Tag, SavedObject);
}

void USaveGSubSystem::LoadSaveData(FString Tag, UObject* SavedObject)
{
    if (CLOG_SAVE_G_SYSTEM(SavedObject == nullptr, "Saved Object is nullptr")) return;
    if (CLOG_SAVE_G_SYSTEM(Tag.IsEmpty(), "Tag is empty")) return;
    if (CLOG_SAVE_G_SYSTEM(!SaveGData.Contains(Tag), "SaveGData do not contains Tag - %s", *Tag)) return;

    TSharedPtr<FJsonObject> JsonObject = USaveGLibrary::ConvertStringToJsonObject(SaveGData[Tag]);
    TArray<FProperty*> Properties = USaveGLibrary::GetAllPropertyHasMetaSaveGame(SavedObject);
    for (FProperty* Property : Properties)
    {
        USaveGLibrary::DeserializeSubProperty(Property, SavedObject, JsonObject);
    }
    OnActionLoadComplete.Broadcast(Tag, SavedObject);
}

void USaveGSubSystem::SaveDataInFile(FString FileName)
{
    if (FileName.IsEmpty())
    {
        FileName = GenerateSaveFileName();
    }
    FString Directory = FPaths::ProjectSavedDir();
    FString FilePath = Directory + "/" + FileName;
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    for (auto& Pair : SaveGData)
    {
        JsonObject->SetStringField(Pair.Key, *Pair.Value);
    }
    FString JsonString = USaveGLibrary::ConvertJsonObjectToString(JsonObject);
    LOG_SAVE_G_SYSTEM(Display, "Convert JSON | Count bytes: %i | Data: %s", JsonString.Len(), *JsonString);

    TArray<uint8> ConvertByte = USaveGLibrary::ConvertStringToByte(JsonString);
    TArray<uint8> CompressData;
    USaveGLibrary::CompressData(ConvertByte, CompressData);
    LOG_SAVE_G_SYSTEM(Display, "Compress data | Count bytes: %i", CompressData.Num());

    FFileHelper::SaveArrayToFile(CompressData, *FilePath);
}

void USaveGSubSystem::LoadDataFromFile(const FString& FileName)
{
    if (CLOG_SAVE_G_SYSTEM(FileName.IsEmpty(), "File Name is empty")) return;

    FString Directory = FPaths::ProjectSavedDir();
    FString FilePath = Directory + "/" + FileName;
    TArray<uint8> CompressData;
    FFileHelper::LoadFileToArray(CompressData, *FilePath);

    TArray<uint8> DecompressData;
    USaveGLibrary::DecompressData(CompressData, DecompressData);
    FString JsonString = USaveGLibrary::ConvertByteToString(DecompressData);
    LOG_SAVE_G_SYSTEM(Display, "Convert JSON | Count bytes: %i | Data: %s", JsonString.Len(), *JsonString);

    TSharedPtr<FJsonObject> JsonObject = USaveGLibrary::ConvertStringToJsonObject(JsonString);
    if (CLOG_SAVE_G_SYSTEM(!JsonObject.IsValid(), "JsonObject is not valid reader")) return;

    SaveGData.Empty();
    for (auto& Pair : JsonObject->Values)
    {
        SaveGData.Add(Pair.Key, Pair.Value->AsString());
    }
}

TArray<FString> USaveGSubSystem::GetAllSaveFiles()
{
    TArray<FString> FileList;
    FString Directory = FPaths::ProjectSavedDir();

    if (FPaths::DirectoryExists(Directory))
    {
        IFileManager& FileManager = IFileManager::Get();
        FileManager.FindFiles(FileList, *Directory, TEXT("*.sav"));
    }

    return FileList;
}

FString USaveGSubSystem::GenerateSaveFileName()
{
    return FString::Printf(TEXT("SaveGame_%s.sav"), *FDateTime::Now().ToString());
}

#pragma endregion