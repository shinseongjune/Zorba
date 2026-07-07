// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ZorbaGameInstance.h"

void UZorbaGameInstance::Init()
{
	Super::Init();

	bHasCompletedInitialBoot = false;
}

void UZorbaGameInstance::Shutdown()
{
	Super::Shutdown();
}

void UZorbaGameInstance::MarkInitialBootComplete()
{
	bHasCompletedInitialBoot = true;
}
