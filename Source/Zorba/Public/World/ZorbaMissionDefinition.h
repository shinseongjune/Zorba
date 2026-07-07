// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "ZorbaMissionDefinition.generated.h"

class UWorld;

UCLASS(BlueprintType)
class ZORBA_API UZorbaMissionDefinition : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	virtual FPrimaryAssetId GetPrimaryAssetId() const override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Mission")
	FName MissionId = NAME_None;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Mission")
	FText DisplayName;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Mission")
	TSoftObjectPtr<UWorld> MissionMap;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Mission", meta = (ClampMin = "1", ClampMax = "3"))
	int32 MaxPlayers = 1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Mission")
	bool bSupportsCoop = false;
};
