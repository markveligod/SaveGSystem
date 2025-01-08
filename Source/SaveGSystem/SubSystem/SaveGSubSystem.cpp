// Fill out your copyright notice in the Description page of Project Settings.

#include "SaveGSubSystem.h"
#include "SaveGSystem/Library/SaveGLibrary.h"
#include "SaveGSystem/Settings/SaveGSettings.h"
#include "SaveGSystem/Task/UpdateSaveDataAsyncTask.h"

#pragma region Actions

USaveGSubSystem* USaveGSubSystem::Get(const UWorld* World)
{
    return World != nullptr ? USaveGSubSystem::Get(World->GetGameInstance()) : nullptr;
}

USaveGSubSystem* USaveGSubSystem::Get(const UGameInstance* GameInstance)
{
    return GameInstance != nullptr ? GameInstance->GetSubsystem<USaveGSubSystem>() : nullptr;
}

void USaveGSubSystem::UpdateSaveData(FString Tag, UObject* SavedObject)
{
    if (CLOG_SAVE_G_SYSTEM(SavedObject == nullptr, "Saved Object is nullptr")) return;
    if (CLOG_SAVE_G_SYSTEM(Tag.IsEmpty(), "Tag is empty")) return;

    FInitDataAsyncTask_SaveGSystem NewDataTask;
    NewDataTask.Action = ETaskAction_SaveGSystem::Save;
    NewDataTask.Object = SavedObject;
    NewDataTask.Tag = Tag;
    RequestActionData.Add(NewDataTask);
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        GameInstance->GetTimerManager().SetTimerForNextTick(this, &ThisClass::NextRequestActionData);
    }
}

bool USaveGSubSystem::IsHaveTag(FString Tag) const
{
    if (CLOG_SAVE_G_SYSTEM(Tag.IsEmpty(), "Tag is empty")) return false;
    return SaveGData.Contains(Tag);
}

void USaveGSubSystem::LoadSaveData(FString Tag, UObject* SavedObject)
{
    if (CLOG_SAVE_G_SYSTEM(SavedObject == nullptr, "Saved Object is nullptr")) return;
    if (CLOG_SAVE_G_SYSTEM(Tag.IsEmpty(), "Tag is empty")) return;
    if (CLOG_SAVE_G_SYSTEM(!SaveGData.Contains(Tag), "SaveGData do not contains Tag - %s", *Tag)) return;

    FInitDataAsyncTask_SaveGSystem NewDataTask;
    NewDataTask.Action = ETaskAction_SaveGSystem::Load;
    NewDataTask.Object = SavedObject;
    NewDataTask.Tag = Tag;
    NewDataTask.JsonSaveData = SaveGData[Tag];
    RequestActionData.Add(NewDataTask);
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        GameInstance->GetTimerManager().SetTimerForNextTick(this, &ThisClass::NextRequestActionData);
    }
}

void USaveGSubSystem::SaveDataInFile(FString FileName)
{
    if (FileName.IsEmpty())
    {
        FileName = GenerateSaveFileName();
    }
    FileName = USaveGLibrary::ValidateFileName(FileName);
    FString Directory = FPaths::ProjectSavedDir();
    FString FilePath = Directory + "SaveGame/" + FileName + ".SaveG";
    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    for (auto& Pair : SaveGData)
    {
        JsonObject->SetStringField(Pair.Key, *Pair.Value);
    }
    FString JsonString = USaveGLibrary::ConvertJsonObjectToString(JsonObject);
    LOG_SAVE_G_SYSTEM(Display, "Convert JSON | Count bytes: %i | Data: %s", JsonString.Len(), *JsonString);

    if (USaveGSettings::IsEnableDataJSONFileStatic())
    {
        FString FilePathJson = Directory + "SaveGame/" + FileName + ".json";
        FFileHelper::SaveStringToFile(JsonString, *FilePathJson);
    }

    FString TrimStr = JsonString.TrimStartAndEnd();
    TArray<uint8> ConvertByte = USaveGLibrary::ConvertStringToByte(TrimStr);
    TArray<uint8> CompressData;
    USaveGLibrary::CompressData(ConvertByte, CompressData);
    LOG_SAVE_G_SYSTEM(Display, "Compress data | Count bytes: %i", CompressData.Num());

    FFileHelper::SaveArrayToFile(CompressData, *FilePath);
}

void USaveGSubSystem::LoadDataFromFile(const FString& FileName)
{
    if (CLOG_SAVE_G_SYSTEM(FileName.IsEmpty(), "File Name is empty")) return;

    FString Directory = FPaths::ProjectSavedDir();
    FString FilePath = Directory + "SaveGame/" + FileName + ".SaveG";
    TArray<uint8> CompressData;
    FFileHelper::LoadFileToArray(CompressData, *FilePath);

    TArray<uint8> DecompressData;
    USaveGLibrary::DecompressData(CompressData, DecompressData);
    FString JsonString = USaveGLibrary::ConvertByteToString(DecompressData);
    LOG_SAVE_G_SYSTEM(Display, "Convert JSON | Count bytes: %i | Data: %s", JsonString.Len(), *JsonString);

    FString TrimStr = JsonString.TrimStartAndEnd();
    TSharedPtr<FJsonObject> JsonObject = USaveGLibrary::ConvertStringToJsonObject(TrimStr);
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
        FileManager.FindFiles(FileList, *Directory, TEXT("*.SaveG"));
    }

    return FileList;
}

FString USaveGSubSystem::GenerateSaveFileName()
{
    return FString::Printf(TEXT("SaveGame_%s"), *FDateTime::Now().ToString());
}

void USaveGSubSystem::NextRequestActionData()
{
    if (ActionDataAsyncTask.Get() != nullptr) return;
    RequestActionData.RemoveAll([](const FInitDataAsyncTask_SaveGSystem& Data) { return !Data.IsValid(); });
    if (RequestActionData.Num() == 0) return;
    if (!RequestActionData.IsValidIndex(0)) return;

    FInitDataAsyncTask_SaveGSystem InitData = RequestActionData[0];
    if (auto* Node = UUpdateSaveDataAsyncTask::Create(InitData))
    {
        Node->GetCompleteTaskSignature().AddDynamic(this, &ThisClass::RegisterCompleteActionDataAsyncTask);
        Node->Activate();
        ActionDataAsyncTask = Node;
    }
    RequestActionData.RemoveAt(0);
}

void USaveGSubSystem::RegisterCompleteActionDataAsyncTask(const FString& Tag, UObject* SavedObject)
{
    if (ActionDataAsyncTask.Get() == nullptr) return;
    ActionDataAsyncTask->GetCompleteTaskSignature().RemoveDynamic(this, &ThisClass::RegisterCompleteActionDataAsyncTask);

    FInitDataAsyncTask_SaveGSystem InitData = ActionDataAsyncTask->GetInitData();
    if (InitData.Action == ETaskAction_SaveGSystem::Save)
    {
        if (SaveGData.Contains(Tag))
        {
            SaveGData[Tag] = InitData.JsonSaveData;
            LOG_SAVE_G_SYSTEM(Display, "Updated Saved Data: Tag - [%s] | Data - [%s]", *Tag, *InitData.JsonSaveData);
        }
        else
        {
            SaveGData.Add(Tag, InitData.JsonSaveData);
            LOG_SAVE_G_SYSTEM(Display, "Create Saved Data: Tag - [%s] | Data - [%s]", *Tag, *InitData.JsonSaveData);
        }
        OnActionSaveComplete.Broadcast(Tag, SavedObject);
    }
    else if (InitData.Action == ETaskAction_SaveGSystem::Load)
    {
        OnActionLoadComplete.Broadcast(Tag, SavedObject);
    }

    ActionDataAsyncTask.Reset();
    if (UGameInstance* GameInstance = GetGameInstance())
    {
        GameInstance->GetTimerManager().SetTimerForNextTick(this, &ThisClass::NextRequestActionData);
    }
}

#pragma endregion