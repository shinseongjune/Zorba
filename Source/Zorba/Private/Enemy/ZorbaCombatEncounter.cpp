// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/ZorbaCombatEncounter.h"

#include "AbilitySystemComponent.h"
#include "Combat/ZorbaCombatAttributeSet.h"
#include "Enemy/ZorbaEnemyBasicAttackComponent.h"
#include "Enemy/ZorbaEnemyCharacter.h"
#include "Kismet/GameplayStatics.h"
#if !UE_BUILD_SHIPPING
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/ZorbaCharacter.h"
#endif

AZorbaCombatEncounter::AZorbaCombatEncounter()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AZorbaCombatEncounter::BeginPlay()
{
	Super::BeginPlay();
	if (bAutoStart)
	{
		StartEncounter();
	}
}

void AZorbaCombatEncounter::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	for (const TWeakObjectPtr<AZorbaEnemyCharacter>& Enemy : TrackedEnemies)
	{
		if (Enemy.IsValid())
		{
			Enemy->OnEnemyDied.RemoveDynamic(
				this,
				&AZorbaCombatEncounter::HandleEnemyDied);
		}
	}
	TrackedEnemies.Reset();
	Super::EndPlay(EndPlayReason);
}

void AZorbaCombatEncounter::StartEncounter()
{
	if (bEncounterActive)
	{
		return;
	}

	TrackedEnemies.Reset();
	TArray<AActor*> EnemyActors;
	UGameplayStatics::GetAllActorsOfClass(
		this,
		AZorbaEnemyCharacter::StaticClass(),
		EnemyActors);
	for (AActor* Candidate : EnemyActors)
	{
		AZorbaEnemyCharacter* Enemy =
			Cast<AZorbaEnemyCharacter>(Candidate);
		if (!Enemy
			|| Enemy->IsDead()
			|| FVector::DistSquared2D(
				GetActorLocation(),
				Enemy->GetActorLocation())
				> FMath::Square(FMath::Max(0.0f, DiscoveryRadius)))
		{
			continue;
		}

		TrackedEnemies.Add(Enemy);
		Enemy->OnEnemyDied.AddUniqueDynamic(
			this,
			&AZorbaCombatEncounter::HandleEnemyDied);
	}

	RemainingEnemyCount = TrackedEnemies.Num();
	bEncounterActive = RemainingEnemyCount > 0;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Combat encounter started: Id=%s Enemies=%d"),
		*EncounterId.ToString(),
		RemainingEnemyCount);
	OnEncounterStarted.Broadcast(RemainingEnemyCount);

#if !UE_BUILD_SHIPPING
	ConfigureCrowdAutomation();
#endif
}

void AZorbaCombatEncounter::HandleEnemyDied(
	AZorbaEnemyCharacter* Enemy,
	AActor* Killer,
	FHitResult HitResult)
{
	if (!bEncounterActive || !Enemy)
	{
		return;
	}

	Enemy->OnEnemyDied.RemoveDynamic(
		this,
		&AZorbaCombatEncounter::HandleEnemyDied);
	RemainingEnemyCount = FMath::Max(0, RemainingEnemyCount - 1);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Combat encounter enemy defeated: Id=%s Enemy=%s Remaining=%d"),
		*EncounterId.ToString(),
		*GetNameSafe(Enemy),
		RemainingEnemyCount);
	if (RemainingEnemyCount == 0)
	{
		CompleteEncounter();
	}
}

void AZorbaCombatEncounter::CompleteEncounter()
{
	if (!bEncounterActive)
	{
		return;
	}

	bEncounterActive = false;
	OnEncounterCompleted.Broadcast();
	OnEncounterClearPresentation();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Combat encounter completed: Id=%s"),
		*EncounterId.ToString());
}

#if !UE_BUILD_SHIPPING
void AZorbaCombatEncounter::ConfigureCrowdAutomation()
{
	FString TestMode;
	if (!FParse::Value(
		FCommandLine::Get(),
		TEXT("ZorbaCrowdTest="),
		TestMode))
	{
		return;
	}

	FTimerHandle AutomationTimer;
	if (TestMode.Equals(TEXT("FodderParry"), ESearchCase::IgnoreCase))
	{
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&AZorbaCombatEncounter::RunFodderParryAutomation,
			0.35f,
			false);
	}
	else if (TestMode.Equals(TEXT("Complete"), ESearchCase::IgnoreCase))
	{
		GetWorldTimerManager().SetTimer(
			AutomationTimer,
			this,
			&AZorbaCombatEncounter::RunCompletionAutomation,
			0.35f,
			false);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Crowd automation armed: Mode=%s"),
		*TestMode);
}

void AZorbaCombatEncounter::RunFodderParryAutomation()
{
	AZorbaCharacter* PlayerCharacter =
		Cast<AZorbaCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (!PlayerCharacter)
	{
		return;
	}

	AZorbaEnemyCharacter* FodderEnemy = nullptr;
	AZorbaEnemyCharacter* FollowupEnemy = nullptr;
	for (const TWeakObjectPtr<AZorbaEnemyCharacter>& Candidate : TrackedEnemies)
	{
		if (Candidate.IsValid() && Candidate->IsFodder() && !Candidate->IsDead())
		{
			if (!FodderEnemy)
			{
				FodderEnemy = Candidate.Get();
			}
			else if (!FollowupEnemy)
			{
				FollowupEnemy = Candidate.Get();
			}
		}
	}
	if (!FodderEnemy || !FodderEnemy->GetBasicAttackComponent())
	{
		return;
	}

	if (UAbilitySystemComponent* PlayerAbilitySystem =
		PlayerCharacter->GetAbilitySystemComponent())
	{
		PlayerAbilitySystem->ApplyModToAttribute(
			UZorbaCombatAttributeSet::GetHealthAttribute(),
			EGameplayModOp::Additive,
			-60.0f);
		PlayerAbilitySystem->ApplyModToAttribute(
			UZorbaCombatAttributeSet::GetCombatStaminaAttribute(),
			EGameplayModOp::Additive,
			-75.0f);
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Crowd fodder-parry benefits prepared: Health=%.1f Stamina=%.1f"),
			PlayerAbilitySystem->GetNumericAttribute(
				UZorbaCombatAttributeSet::GetHealthAttribute()),
			PlayerAbilitySystem->GetNumericAttribute(
				UZorbaCombatAttributeSet::GetCombatStaminaAttribute()));
	}

	PlayerCharacter->ConfigureDefenseForAutomation(true);
	FVector PlayerForward = PlayerCharacter->GetActorForwardVector().GetSafeNormal2D();
	if (PlayerForward.IsNearlyZero())
	{
		PlayerForward = FVector::ForwardVector;
	}
	FVector AttackLocation =
		PlayerCharacter->GetActorLocation() + PlayerForward * 150.0f;
	AttackLocation.Z = FodderEnemy->GetActorLocation().Z;
	FodderEnemy->SetActorLocation(AttackLocation, false);
	if (FollowupEnemy)
	{
		FVector FollowupLocation =
			PlayerCharacter->GetActorLocation() - PlayerForward * 150.0f;
		FollowupLocation.Z = FollowupEnemy->GetActorLocation().Z;
		FollowupEnemy->SetActorLocation(FollowupLocation, false);
	}
	FodderEnemy->GetBasicAttackComponent()->TryBasicAttack(PlayerCharacter);
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Crowd fodder-parry automation triggered: Enemy=%s"),
		*GetNameSafe(FodderEnemy));
}

void AZorbaCombatEncounter::RunCompletionAutomation()
{
	AActor* PlayerActor = UGameplayStatics::GetPlayerCharacter(this, 0);
	for (const TWeakObjectPtr<AZorbaEnemyCharacter>& Candidate : TrackedEnemies)
	{
		AZorbaEnemyCharacter* Enemy = Candidate.Get();
		if (!Enemy || Enemy->IsDead() || !Enemy->GetAbilitySystemComponent())
		{
			continue;
		}

		const float CurrentHealth = Enemy->GetCombatAttributes()
			? Enemy->GetCombatAttributes()->GetHealth()
			: 0.0f;
		Enemy->GetAbilitySystemComponent()->ApplyModToAttribute(
			UZorbaCombatAttributeSet::GetHealthAttribute(),
			EGameplayModOp::Additive,
			-CurrentHealth);
		FHitResult HitResult;
		HitResult.ImpactPoint = Enemy->GetActorLocation();
		Enemy->HandleMeleeHit(PlayerActor, HitResult);
	}
}
#endif
