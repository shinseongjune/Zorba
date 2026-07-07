// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "ZorbaGameMode.generated.h"

UCLASS()
class ZORBA_API AZorbaGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AZorbaGameMode();

protected:
	virtual void BeginPlay() override;
};
