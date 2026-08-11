// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Core/ZorbaSessionTypes.h"
#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ZorbaPlayerController.generated.h"

class AZorbaGameState;
class UZorbaMissionFlowWidget;

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

	UFUNCTION(BlueprintCallable, Category = "Zorba|Mission")
	void StartDefaultMission();

	UFUNCTION(BlueprintCallable, Category = "Zorba|Mission")
	void RestartActiveMission();

	UFUNCTION(BlueprintCallable, Category = "Zorba|Mission")
	void ReturnToBoot();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void SetupInputComponent() override;

private:
	void BindMissionUI();
	void ApplyMissionInputMode(EZorbaSessionPhase NewPhase);

	UFUNCTION()
	void HandleSessionPhaseChanged(EZorbaSessionPhase NewPhase);

	UFUNCTION(Server, Reliable)
	void ServerStartDefaultMission();

	UFUNCTION(Server, Reliable)
	void ServerRestartActiveMission();

	UFUNCTION(Server, Reliable)
	void ServerReturnToBoot();

	UPROPERTY(Transient)
	TObjectPtr<UZorbaMissionFlowWidget> MissionFlowWidget;

	UPROPERTY(Transient)
	TObjectPtr<AZorbaGameState> BoundGameState;
};
