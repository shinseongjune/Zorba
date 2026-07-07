// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/ZorbaPlayerController.h"

AZorbaPlayerController::AZorbaPlayerController()
{
	bReplicates = true;
	bShowMouseCursor = false;
}

void AZorbaPlayerController::BeginPlay()
{
	Super::BeginPlay();
}

void AZorbaPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
}

void AZorbaPlayerController::ClientReceiveSystemMessage_Implementation(const FString& Message)
{
	UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
}
