// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ZorbaGameInstance.generated.h"

UCLASS(BlueprintType)
class ZORBA_API UZorbaGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;
	virtual void Shutdown() override;

	UFUNCTION(BlueprintPure, Category = "Zorba|Boot")
	bool HasCompletedInitialBoot() const { return bHasCompletedInitialBoot; }

	UFUNCTION(BlueprintCallable, Category = "Zorba|Boot")
	void MarkInitialBootComplete();

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Zorba|Boot")
	bool bHasCompletedInitialBoot = false;
};
