// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ZorbaCampaignSubsystem.generated.h"

UCLASS()
class ZORBA_API UZorbaCampaignSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Zorba|Campaign")
	void StartNewCampaign(FName NewCampaignId, int32 NewCampaignSeed);

	UFUNCTION(BlueprintCallable, Category = "Zorba|Campaign")
	void SetPendingMission(FName MissionId);

	UFUNCTION(BlueprintPure, Category = "Zorba|Campaign")
	FName GetCurrentCampaignId() const { return CurrentCampaignId; }

	UFUNCTION(BlueprintPure, Category = "Zorba|Campaign")
	FName GetPendingMissionId() const { return PendingMissionId; }

	UFUNCTION(BlueprintPure, Category = "Zorba|Campaign")
	int32 GetCampaignSeed() const { return CampaignSeed; }

private:
	UPROPERTY(VisibleInstanceOnly, Category = "Zorba|Campaign")
	FName CurrentCampaignId = NAME_None;

	UPROPERTY(VisibleInstanceOnly, Category = "Zorba|Campaign")
	FName PendingMissionId = NAME_None;

	UPROPERTY(VisibleInstanceOnly, Category = "Zorba|Campaign")
	int32 CampaignSeed = 0;
};
