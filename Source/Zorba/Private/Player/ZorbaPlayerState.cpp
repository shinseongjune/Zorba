// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/ZorbaPlayerState.h"

#include "AbilitySystemComponent.h"
#include "Combat/ZorbaCombatAttributeSet.h"
#include "Net/UnrealNetwork.h"

AZorbaPlayerState::AZorbaPlayerState()
{
	bReplicates = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	CombatAttributes = CreateDefaultSubobject<UZorbaCombatAttributeSet>(TEXT("CombatAttributes"));
}

UAbilitySystemComponent* AZorbaPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
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
