// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IO/IoContainerHeader.h"
#include "Kismet/BlueprintAsyncActionBase.h"
#include "SaveGSystem/Data/SaveGSystemDataTypes.h"
#include "UpdateSaveDataAsyncTask.generated.h"

/**
 * @Task This is necessary for sequential processing of a stack of requests to save/load objects by tag.
 */
UCLASS(NotBlueprintable)
class SAVEGSYSTEM_API UUpdateSaveDataAsyncTask : public UBlueprintAsyncActionBase
{
    GENERATED_BODY()

public:

    /** @public **/
    static UUpdateSaveDataAsyncTask* Create(const FInitDataAsyncTask_SaveGSystem& InitData);

    /** @public **/
    virtual void Activate() override;

    /** @public **/
    virtual void SetReadyToDestroy() override;

    /** @public **/
    void SetupInitData(const FInitDataAsyncTask_SaveGSystem& InInitData) { InitData = InInitData; }

    /** @public **/
    FInitDataAsyncTask_SaveGSystem GetInitData() const { return InitData; }

    /** @public **/
    virtual UWorld* GetWorld() const override;

    /** @public **/
    FActionSaveGSystemSignature& GetCompleteTaskSignature() { return CompleteTask; }

protected:

    /** @protected **/
    virtual void ActivateRemainTimer();

    /** @protected **/
    virtual void ActionTask();

private:

    /** @private **/
    void SaveData();

    /** @private **/
    void LoadData();

    /** @private **/
    FTimerHandle RemainDelay_TimerHandle;

    /** @private **/
    FInitDataAsyncTask_SaveGSystem InitData;

    /** @private **/
    FActionSaveGSystemSignature CompleteTask;
};
