// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/ZorbaPlayerController.h"

#include "Kismet/GameplayStatics.h"

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

void AZorbaPlayerController::TogglePause()
{
	const bool bShouldPause = !UGameplayStatics::IsGamePaused(this);
	UGameplayStatics::SetGamePaused(this, bShouldPause);

	bShowMouseCursor = bShouldPause;

	if (bShouldPause)
	{
		SetInputMode(FInputModeGameAndUI());
		ClientReceiveSystemMessage(TEXT("Game paused."));
	}
	else
	{
		SetInputMode(FInputModeGameOnly());
		ClientReceiveSystemMessage(TEXT("Game resumed."));
	}
}
