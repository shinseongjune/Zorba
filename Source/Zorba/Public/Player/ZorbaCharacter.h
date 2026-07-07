// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ZorbaCharacter.generated.h"

class UCameraComponent;
class USpringArmComponent;

UCLASS()
class ZORBA_API AZorbaCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZorbaCharacter();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;
};
