// Copyright 1998-2016 Epic Games, Inc. All Rights Reserved.

#include "LinuxDeviceProfileSelectorPrivatePCH.h"

IMPLEMENT_MODULE(FLinuxDeviceProfileSelectorModule, LinuxDeviceProfileSelector);


void FLinuxDeviceProfileSelectorModule::StartupModule()
{
}


void FLinuxDeviceProfileSelectorModule::ShutdownModule()
{
}


FString const FLinuxDeviceProfileSelectorModule::GetRuntimeDeviceProfileName()
{
	// [RCL] 2015-09-22 FIXME: support different environments
	FString ProfileName = FPlatformProperties::PlatformName();
	UE_LOG(LogLinux, Log, TEXT("Selected Device Profile: [%s]"), *ProfileName);
	return ProfileName;
}

