// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/ZorbaEnemyBasicAttackComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/ZorbaAttackDefinition.h"
#include "Combat/ZorbaCombatAttributeSet.h"
#include "Combat/ZorbaGameplayTags.h"
#include "Combat/ZorbaMeleeCombatComponent.h"
#include "Engine/World.h"
#include "Enemy/ZorbaEnemyCharacter.h"
#include "Enemy/ZorbaEnemyCrowdSubsystem.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/GameplayStatics.h"
#if !UE_BUILD_SHIPPING
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Player/ZorbaCharacter.h"
#endif

UZorbaEnemyBasicAttackComponent::UZorbaEnemyBasicAttackComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.05f;
}

void UZorbaEnemyBasicAttackComponent::BeginPlay()
{
	Super::BeginPlay();

	MeleeCombatComponent =
		GetOwner()
			? GetOwner()->FindComponentByClass<UZorbaMeleeCombatComponent>()
			: nullptr;

	if (const UWorld* World = GetWorld())
	{
		NextAttackTime = World->GetTimeSeconds() + InitialAttackDelay;
		if (UZorbaEnemyCrowdSubsystem* CrowdSubsystem =
			World->GetSubsystem<UZorbaEnemyCrowdSubsystem>())
		{
			CrowdSubsystem->RegisterEnemy(
				Cast<AZorbaEnemyCharacter>(GetOwner()));
		}
	}
}

void UZorbaEnemyBasicAttackComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	AZorbaEnemyCharacter* EnemyOwner =
		Cast<AZorbaEnemyCharacter>(OwnerActor);
	if (bExternallyDriven
		|| !bAutoAttack
		|| !OwnerActor
		|| !OwnerActor->HasAuthority()
		|| !World
		|| !MeleeCombatComponent.IsValid()
		|| !EnemyOwner
		|| EnemyOwner->IsDead())
	{
		if (EnemyOwner && EnemyOwner->IsDead())
		{
			StopCrowdCombat();
		}
		return;
	}

	ACharacter* PlayerCharacter =
		UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!CanEngageTarget(PlayerCharacter))
	{
		ReleaseAttackToken();
		if (UCharacterMovementComponent* Movement =
			EnemyOwner->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
		return;
	}

	if (AttackTokenTarget.IsValid()
		&& AttackTokenTarget.Get() != PlayerCharacter)
	{
		ReleaseAttackToken();
	}

	if (MeleeCombatComponent->IsAttackInProgress())
	{
		if (UCharacterMovementComponent* Movement =
			EnemyOwner->GetCharacterMovement())
		{
			Movement->StopMovementImmediately();
		}
		return;
	}

	if (bHasAttackToken
		&& World->GetTimeSeconds() < NextAttackTime)
	{
		ReleaseAttackToken();
	}

	if (World->GetTimeSeconds() >= NextAttackTime
		&& !bHasAttackToken)
	{
		AcquireAttackToken(PlayerCharacter);
	}

	if (bHasAttackToken)
	{
		if (CanAttackTarget(PlayerCharacter))
		{
			TryBasicAttack(PlayerCharacter);
		}
		else
		{
			UpdateCrowdMovement(PlayerCharacter, true);
		}
	}
	else
	{
		UpdateCrowdMovement(PlayerCharacter, false);
	}
}

void UZorbaEnemyBasicAttackComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ReleaseAttackToken();
	if (UWorld* World = GetWorld())
	{
		if (UZorbaEnemyCrowdSubsystem* CrowdSubsystem =
			World->GetSubsystem<UZorbaEnemyCrowdSubsystem>())
		{
			CrowdSubsystem->UnregisterEnemy(
				Cast<AZorbaEnemyCharacter>(GetOwner()));
		}
	}
	Super::EndPlay(EndPlayReason);
}

bool UZorbaEnemyBasicAttackComponent::TryBasicAttack(AActor* TargetActor)
{
	return TryAttackDefinition(TargetActor, AttackDefinition);
}

bool UZorbaEnemyBasicAttackComponent::TryAttackDefinition(
	AActor* TargetActor,
	UZorbaAttackDefinition* RequestedAttack)
{
	AActor* OwnerActor = GetOwner();
	UWorld* World = GetWorld();
	if (!OwnerActor
		|| !OwnerActor->HasAuthority()
		|| !World
		|| !IsValid(RequestedAttack)
		|| !MeleeCombatComponent.IsValid()
		|| MeleeCombatComponent->IsAttackInProgress()
		|| !IsTargetInAttackRange(
			TargetActor,
			RequestedAttack,
			AttackRangePadding))
	{
		return false;
	}

	if (!bExternallyDriven && !AcquireAttackToken(TargetActor))
	{
		return false;
	}

	FVector AttackDirection =
		TargetActor->GetActorLocation() - OwnerActor->GetActorLocation();
	AttackDirection.Z = 0.0f;
	AttackDirection = AttackDirection.GetSafeNormal();
	if (AttackDirection.IsNearlyZero())
	{
		if (!bExternallyDriven)
		{
			ReleaseAttackToken();
		}
		return false;
	}

#if !UE_BUILD_SHIPPING
	ApplyDefenseAutomation(TargetActor);
#endif

	if (!MeleeCombatComponent->BeginAttack(
		RequestedAttack,
		AttackDirection))
	{
		if (!bExternallyDriven)
		{
			ReleaseAttackToken();
		}
		return false;
	}

	NextAttackTime = World->GetTimeSeconds() + AttackCooldown;
	OnBasicAttackStarted(TargetActor);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Enemy attack started: Owner=%s Target=%s Attack=%s Cooldown=%.2f"),
		*GetNameSafe(OwnerActor),
		*GetNameSafe(TargetActor),
		*RequestedAttack->AttackId.ToString(),
		AttackCooldown);

	return true;
}

void UZorbaEnemyBasicAttackComponent::SetExternallyDriven(
	bool bNewExternallyDriven)
{
	bExternallyDriven = bNewExternallyDriven;
	if (bExternallyDriven)
	{
		ReleaseAttackToken();
	}
}

void UZorbaEnemyBasicAttackComponent::StopCrowdCombat()
{
	bAutoAttack = false;
	ReleaseAttackToken();
	if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		OwnerCharacter->GetCharacterMovement()->StopMovementImmediately();
	}
	SetComponentTickEnabled(false);
}

bool UZorbaEnemyBasicAttackComponent::CanEngageTarget(
	AActor* TargetActor) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(AttackDefinition)
		|| !IsValid(TargetActor)
		|| !OwnerActor
		|| TargetActor == OwnerActor)
	{
		return false;
	}

	const IGenericTeamAgentInterface* SourceTeamAgent =
		Cast<IGenericTeamAgentInterface>(OwnerActor);
	const IGenericTeamAgentInterface* TargetTeamAgent =
		Cast<IGenericTeamAgentInterface>(TargetActor);
	if (!SourceTeamAgent
		|| !TargetTeamAgent
		|| SourceTeamAgent->GetGenericTeamId()
			== TargetTeamAgent->GetGenericTeamId())
	{
		return false;
	}

	UAbilitySystemComponent* SourceAbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
			const_cast<AActor*>(OwnerActor));
	UAbilitySystemComponent* TargetAbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!SourceAbilitySystem
		|| !TargetAbilitySystem
		|| SourceAbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_HitReact)
		|| SourceAbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_StaminaDepleted)
		|| SourceAbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Exhausted)
		|| SourceAbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead)
		|| TargetAbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead)
		|| TargetAbilitySystem->GetNumericAttribute(
			UZorbaCombatAttributeSet::GetHealthAttribute()) <= 0.0f)
	{
		return false;
	}

	return FVector::DistSquared2D(
		OwnerActor->GetActorLocation(),
		TargetActor->GetActorLocation())
		<= FMath::Square(FMath::Max(0.0f, EngagementRange));
}

bool UZorbaEnemyBasicAttackComponent::CanAttackTarget(
	AActor* TargetActor) const
{
	return IsTargetInAttackRange(
		TargetActor,
		AttackDefinition,
		AttackRangePadding);
}

bool UZorbaEnemyBasicAttackComponent::IsTargetInAttackRange(
	AActor* TargetActor,
	const UZorbaAttackDefinition* RequestedAttack,
	float AdditionalPadding) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(RequestedAttack)
		|| !IsValid(TargetActor)
		|| !OwnerActor)
	{
		return false;
	}

	const IGenericTeamAgentInterface* SourceTeamAgent =
		Cast<IGenericTeamAgentInterface>(OwnerActor);
	const IGenericTeamAgentInterface* TargetTeamAgent =
		Cast<IGenericTeamAgentInterface>(TargetActor);
	UAbilitySystemComponent* SourceAbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(
			const_cast<AActor*>(OwnerActor));
	UAbilitySystemComponent* TargetAbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!SourceTeamAgent
		|| !TargetTeamAgent
		|| SourceTeamAgent->GetGenericTeamId()
			== TargetTeamAgent->GetGenericTeamId()
		|| !SourceAbilitySystem
		|| !TargetAbilitySystem
		|| SourceAbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead)
		|| TargetAbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead))
	{
		return false;
	}

	return FVector::DistSquared2D(
		OwnerActor->GetActorLocation(),
		TargetActor->GetActorLocation())
		<= FMath::Square(
			ResolveAttackRange(RequestedAttack)
			+ FMath::Max(0.0f, AdditionalPadding));
}

bool UZorbaEnemyBasicAttackComponent::AcquireAttackToken(
	AActor* TargetActor)
{
	if (bHasAttackToken && AttackTokenTarget.Get() == TargetActor)
	{
		return true;
	}

	ReleaseAttackToken();
	if (!bUseCrowdCoordination)
	{
		bHasAttackToken = true;
		AttackTokenTarget = TargetActor;
		return true;
	}

	AZorbaEnemyCharacter* EnemyOwner =
		Cast<AZorbaEnemyCharacter>(GetOwner());
	UWorld* World = GetWorld();
	UZorbaEnemyCrowdSubsystem* CrowdSubsystem =
		World ? World->GetSubsystem<UZorbaEnemyCrowdSubsystem>() : nullptr;
	if (!CrowdSubsystem
		|| !CrowdSubsystem->TryAcquireAttackToken(
			EnemyOwner,
			TargetActor,
			FMath::Max(1, MaxConcurrentAttackers)))
	{
		return false;
	}

	bHasAttackToken = true;
	AttackTokenTarget = TargetActor;
	return true;
}

void UZorbaEnemyBasicAttackComponent::ReleaseAttackToken()
{
	if (bUseCrowdCoordination)
	{
		if (UWorld* World = GetWorld())
		{
			if (UZorbaEnemyCrowdSubsystem* CrowdSubsystem =
				World->GetSubsystem<UZorbaEnemyCrowdSubsystem>())
			{
				CrowdSubsystem->ReleaseAttackToken(
					Cast<AZorbaEnemyCharacter>(GetOwner()));
			}
		}
	}
	bHasAttackToken = false;
	AttackTokenTarget.Reset();
}

void UZorbaEnemyBasicAttackComponent::UpdateCrowdMovement(
	AActor* TargetActor,
	bool bApproachToAttack)
{
	AZorbaEnemyCharacter* EnemyOwner =
		Cast<AZorbaEnemyCharacter>(GetOwner());
	if (!EnemyOwner || EnemyOwner->IsDead())
	{
		return;
	}

	UCharacterMovementComponent* Movement =
		EnemyOwner->GetCharacterMovement();
	if (!Movement || Movement->MovementMode == MOVE_None)
	{
		return;
	}

	if (!IsValid(TargetActor))
	{
		Movement->StopMovementImmediately();
		return;
	}

	FVector DesiredLocation = TargetActor->GetActorLocation();
	if (bApproachToAttack)
	{
		FVector AwayFromTarget =
			(EnemyOwner->GetActorLocation() - DesiredLocation).GetSafeNormal2D();
		if (AwayFromTarget.IsNearlyZero())
		{
			AwayFromTarget = -TargetActor->GetActorForwardVector().GetSafeNormal2D();
		}
		DesiredLocation += AwayFromTarget
			* FMath::Max(
				100.0f,
				ResolveAttackRange(AttackDefinition) * 0.7f);
	}
	else if (UWorld* World = GetWorld())
	{
		if (UZorbaEnemyCrowdSubsystem* CrowdSubsystem =
			World->GetSubsystem<UZorbaEnemyCrowdSubsystem>())
		{
			DesiredLocation = CrowdSubsystem->GetStandbyLocation(
				EnemyOwner,
				TargetActor,
				StandbyRadius);
		}
	}

	FVector MoveDelta = DesiredLocation - EnemyOwner->GetActorLocation();
	MoveDelta.Z = 0.0f;
	if (MoveDelta.SizeSquared()
		<= FMath::Square(FMath::Max(1.0f, MovementAcceptanceRadius)))
	{
		Movement->StopMovementImmediately();
		if (!bApproachToAttack)
		{
			const FVector Facing =
				(TargetActor->GetActorLocation() - EnemyOwner->GetActorLocation())
				.GetSafeNormal2D();
			if (!Facing.IsNearlyZero())
			{
				EnemyOwner->SetActorRotation(
					FRotator(0.0f, Facing.Rotation().Yaw, 0.0f));
			}
		}
		return;
	}

	EnemyOwner->AddMovementInput(MoveDelta.GetSafeNormal(), 1.0f, true);
}

float UZorbaEnemyBasicAttackComponent::ResolveAttackRange(
	const UZorbaAttackDefinition* RequestedAttack) const
{
	return IsValid(RequestedAttack)
		&& RequestedAttack->HitPhases.IsValidIndex(0)
		? FMath::Max(0.0f, RequestedAttack->HitPhases[0].Range)
		: 0.0f;
}

#if !UE_BUILD_SHIPPING
void UZorbaEnemyBasicAttackComponent::ApplyDefenseAutomation(
	AActor* TargetActor)
{
	if (bDefenseAutomationApplied)
	{
		return;
	}

	FString DefenseTestMode;
	if (!FParse::Value(
		FCommandLine::Get(),
		TEXT("ZorbaDefenseTest="),
		DefenseTestMode))
	{
		return;
	}

	const bool bTestBlock =
		DefenseTestMode.Equals(TEXT("Block"), ESearchCase::IgnoreCase);
	const bool bTestParry =
		DefenseTestMode.Equals(TEXT("Parry"), ESearchCase::IgnoreCase);
	if (!bTestBlock && !bTestParry)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Unknown ZorbaDefenseTest mode: %s"),
			*DefenseTestMode);
		bDefenseAutomationApplied = true;
		return;
	}

	AZorbaCharacter* PlayerCharacter = Cast<AZorbaCharacter>(TargetActor);
	if (!PlayerCharacter)
	{
		return;
	}

	PlayerCharacter->ConfigureDefenseForAutomation(bTestParry);
	bDefenseAutomationApplied = true;
}
#endif
