// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ZorbaPlayerState.generated.h"

UCLASS()
class ZORBA_API AZorbaPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AZorbaPlayerState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Zorba|Player")
	void AddMissionScore(int32 Delta);

	UFUNCTION(BlueprintPure, Category = "Zorba|Player")
	int32 GetMissionScore() const { return MissionScore; }

private:
	UFUNCTION()
	void OnRep_MissionScore();

	UPROPERTY(ReplicatedUsing = OnRep_MissionScore, VisibleInstanceOnly, Category = "Zorba|Player")
	int32 MissionScore = 0;
};
