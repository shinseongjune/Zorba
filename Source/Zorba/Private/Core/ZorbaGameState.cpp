// Copyright Epic Games, Inc. All Rights Reserved.

#include "Core/ZorbaGameState.h"

#include "Net/UnrealNetwork.h"

AZorbaGameState::AZorbaGameState()
{
	bReplicates = true;
}

void AZorbaGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AZorbaGameState, SessionPhase);
	DOREPLIFETIME(AZorbaGameState, ActiveMissionId);
}

void AZorbaGameState::SetSessionPhase(EZorbaSessionPhase NewPhase)
{
	if (!HasAuthority())
	{
		return;
	}
	if (SessionPhase == NewPhase)
	{
		return;
	}

	SessionPhase = NewPhase;
	OnRep_SessionPhase();
}

void AZorbaGameState::SetActiveMissionId(FName MissionId)
{
	if (!HasAuthority())
	{
		return;
	}
	if (ActiveMissionId == MissionId)
	{
		return;
	}

	ActiveMissionId = MissionId;
	OnRep_ActiveMissionId();
}

void AZorbaGameState::OnRep_SessionPhase()
{
	OnSessionPhaseChanged.Broadcast(SessionPhase);
}

void AZorbaGameState::OnRep_ActiveMissionId()
{
	OnActiveMissionChanged.Broadcast(ActiveMissionId);
}
