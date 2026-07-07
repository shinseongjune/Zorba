// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameStateBase.h"
#include "Core/ZorbaSessionTypes.h"
#include "ZorbaGameState.generated.h"

UCLASS()
class ZORBA_API AZorbaGameState : public AGameStateBase
{
	GENERATED_BODY()

public:
	AZorbaGameState();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, Category = "Zorba|Session")
	void SetSessionPhase(EZorbaSessionPhase NewPhase);

	UFUNCTION(BlueprintCallable, Category = "Zorba|Session")
	void SetActiveMissionId(FName MissionId);

	UFUNCTION(BlueprintPure, Category = "Zorba|Session")
	EZorbaSessionPhase GetSessionPhase() const { return SessionPhase; }

	UFUNCTION(BlueprintPure, Category = "Zorba|Session")
	FName GetActiveMissionId() const { return ActiveMissionId; }

private:
	UFUNCTION()
	void OnRep_SessionPhase();

	UFUNCTION()
	void OnRep_ActiveMissionId();

	UPROPERTY(ReplicatedUsing = OnRep_SessionPhase, VisibleInstanceOnly, Category = "Zorba|Session")
	EZorbaSessionPhase SessionPhase = EZorbaSessionPhase::Boot;

	UPROPERTY(ReplicatedUsing = OnRep_ActiveMissionId, VisibleInstanceOnly, Category = "Zorba|Session")
	FName ActiveMissionId = NAME_None;
};
