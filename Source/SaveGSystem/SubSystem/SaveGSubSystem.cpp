// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGSubSystem.h"
#include "SaveGSystem/Data/SaveGSystemDataTypes.h"
#include "SaveGSystem/Library/SaveGLibrary.h"

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
}

#pragma endregion