// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ZorbaPlayerController.generated.h"

UCLASS()
class ZORBA_API AZorbaPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AZorbaPlayerController();

	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "Zorba|Player")
	void ClientReceiveSystemMessage(const FString& Message);

	UFUNCTION(BlueprintCallable, Category = "Zorba|Player")
	void TogglePause();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
};
