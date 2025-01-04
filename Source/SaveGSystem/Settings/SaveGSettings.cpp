// Fill out your copyright notice in the Description page of Project Settings.


#include "SaveGSettings.h"

bool USaveGSettings::IsEnableDataJSONFileStatic()
{
    if (const USaveGSettings* SaveGSettings = GetDefault<USaveGSettings>())
    {
        return SaveGSettings->IsEnableDataJSONFile();
    }
    return false;
}