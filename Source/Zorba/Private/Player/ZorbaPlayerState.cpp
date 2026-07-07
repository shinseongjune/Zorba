// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/ZorbaPlayerState.h"

#include "Net/UnrealNetwork.h"

AZorbaPlayerState::AZorbaPlayerState()
{
	bReplicates = true;
}

void AZorbaPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZorbaPlayerState, MissionScore);
}

void AZorbaPlayerState::AddMissionScore(int32 Delta)
{
	if (!HasAuthority())
	{
		return;
	}

	MissionScore += Delta;
	OnRep_MissionScore();
}

void AZorbaPlayerState::OnRep_MissionScore()
{
}
