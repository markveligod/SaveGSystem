// Fill out your copyright notice in the Description page of Project Settings.

#include "UpdateSaveDataAsyncTask.h"
#include "SaveGSystem/Interface/SaveGInterface.h"
#include "SaveGSystem/Library/SaveGLibrary.h"

UUpdateSaveDataAsyncTask* UUpdateSaveDataAsyncTask::Create(const FInitDataAsyncTask_SaveGSystem& InitData)
{
    if (CLOG_SAVE_G_SYSTEM(!InitData.IsValid(), "Init Data is not valid")) return nullptr;

    UUpdateSaveDataAsyncTask* Node = NewObject<UUpdateSaveDataAsyncTask>();
    if (CLOG_SAVE_G_SYSTEM(Node == nullptr, "Node is nullptr")) return nullptr;
    Node->SetupInitData(InitData);
    Node->RegisterWithGameInstance(InitData.Object.Get());
    return Node;
}

void UUpdateSaveDataAsyncTask::Activate()
{
    Super::Activate();
    ActivateRemainTimer();
    if (UGameInstance* GameInstance = RegisteredWithGameInstance.Get())
    {
        GameInstance->GetTimerManager().SetTimerForNextTick(this, &ThisClass::ActionTask);
    }
}

void UUpdateSaveDataAsyncTask::SetReadyToDestroy()
{
    if (UGameInstance* GameInstance = RegisteredWithGameInstance.Get())
    {
        GameInstance->GetTimerManager().ClearTimer(RemainDelay_TimerHandle);
    }
    CompleteTask.Broadcast(InitData.Tag, InitData.Object.Get());
    Super::SetReadyToDestroy();
}

void UUpdateSaveDataAsyncTask::ActivateRemainTimer()
{
    if (UGameInstance* GameInstance = RegisteredWithGameInstance.Get())
    {
        GameInstance->GetTimerManager().ClearTimer(RemainDelay_TimerHandle);
        GameInstance->GetTimerManager().SetTimer(RemainDelay_TimerHandle, this, &ThisClass::SetReadyToDestroy, InitData.Delay, false);
    }
    else
    {
        SetReadyToDestroy();
    }
}

void UUpdateSaveDataAsyncTask::ActionTask()
{
    if (CLOG_SAVE_G_SYSTEM(!InitData.IsValid(), "Init Data is not valid")) return;
    if (InitData.Action == ETaskAction_SaveGSystem::Save)
    {
        SaveData();
    }
    else if (InitData.Action == ETaskAction_SaveGSystem::Load)
    {
        LoadData();
    }
}

void UUpdateSaveDataAsyncTask::SaveData()
{
    if (CLOG_SAVE_G_SYSTEM(!InitData.IsValid(), "Init Data is not valid")) return;
    if (InitData.GetObjectClass()->ImplementsInterface(USaveGInterface::StaticClass()))
    {
        ISaveGInterface::Execute_PreSave(InitData.GetObject());
    }

    TSharedPtr<FJsonObject> JsonObject = MakeShared<FJsonObject>();
    TArray<FProperty*> Properties = USaveGLibrary::GetAllPropertyHasMetaSaveGame(InitData.Object.Get());
    for (FProperty* Property : Properties)
    {
        USaveGLibrary::SerializeSubProperty(Property, InitData.Object.Get(), JsonObject);
    }
    InitData.JsonSaveData = USaveGLibrary::ConvertJsonObjectToString(JsonObject);

    if (InitData.GetObjectClass()->ImplementsInterface(USaveGInterface::StaticClass()))
    {
        ISaveGInterface::Execute_PostSave(InitData.GetObject());
    }
    if (UGameInstance* GameInstance = RegisteredWithGameInstance.Get())
    {
        GameInstance->GetTimerManager().SetTimerForNextTick(this, &ThisClass::SetReadyToDestroy);
    }
}

void UUpdateSaveDataAsyncTask::LoadData()
{
    if (CLOG_SAVE_G_SYSTEM(!InitData.IsValid(), "Init Data is not valid")) return;
    if (CLOG_SAVE_G_SYSTEM(InitData.JsonSaveData.IsEmpty(), "JsonSaveData is empty")) return;
    if (InitData.GetObjectClass()->ImplementsInterface(USaveGInterface::StaticClass()))
    {
        ISaveGInterface::Execute_PreLoad(InitData.GetObject());
    }

    TSharedPtr<FJsonObject> JsonObject = USaveGLibrary::ConvertStringToJsonObject(InitData.JsonSaveData);
    TArray<FProperty*> Properties = USaveGLibrary::GetAllPropertyHasMetaSaveGame(InitData.Object.Get());
    for (FProperty* Property : Properties)
    {
        USaveGLibrary::DeserializeSubProperty(Property, InitData.Object.Get(), JsonObject);
    }

    if (InitData.GetObjectClass()->ImplementsInterface(USaveGInterface::StaticClass()))
    {
        ISaveGInterface::Execute_PostLoad(InitData.GetObject());
    }

    if (UGameInstance* GameInstance = RegisteredWithGameInstance.Get())
    {
        GameInstance->GetTimerManager().SetTimerForNextTick(this, &ThisClass::SetReadyToDestroy);
    }
}