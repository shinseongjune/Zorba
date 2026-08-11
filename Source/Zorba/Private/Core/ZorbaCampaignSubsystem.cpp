// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ZorbaCampaignSubsystem.h"

#include "Subsystems/SubsystemCollection.h"

void UZorbaCampaignSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UZorbaCampaignSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UZorbaCampaignSubsystem::StartNewCampaign(FName NewCampaignId, int32 NewCampaignSeed)
{
	CurrentCampaignId = NewCampaignId;
	CampaignSeed = NewCampaignSeed;
	PendingMissionId = NAME_None;
}

void UZorbaCampaignSubsystem::SetPendingMission(FName MissionId)
{
	PendingMissionId = MissionId;
}

void UZorbaCampaignSubsystem::ClearPendingMission()
{
	PendingMissionId = NAME_None;
}
