// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/ZorbaEnemyCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/ZorbaCombatAttributeSet.h"
#include "Combat/ZorbaGameplayTags.h"
#include "Combat/ZorbaMeleeCombatComponent.h"
#include "Combat/ZorbaMeleeDamageEffect.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "DrawDebugHelpers.h"
#include "Enemy/ZorbaEnemyBasicAttackComponent.h"
#include "Enemy/ZorbaEnemyCombatBrainComponent.h"
#include "Enemy/ZorbaEnemyCombatProfile.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Player/ZorbaCharacter.h"
#include "UObject/ConstructorHelpers.h"

AZorbaEnemyCharacter::AZorbaEnemyCharacter()
{
	bReplicates = true;

	AbilitySystemComponent =
		CreateDefaultSubobject<UAbilitySystemComponent>(
			TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(
		EGameplayEffectReplicationMode::Minimal);

	CombatAttributes =
		CreateDefaultSubobject<UZorbaCombatAttributeSet>(
			TEXT("CombatAttributes"));

	MeleeCombatComponent =
		CreateDefaultSubobject<UZorbaMeleeCombatComponent>(
			TEXT("MeleeCombatComponent"));

	BasicAttackComponent =
		CreateDefaultSubobject<UZorbaEnemyBasicAttackComponent>(
			TEXT("BasicAttackComponent"));

	CombatBrainComponent =
		CreateDefaultSubobject<UZorbaEnemyCombatBrainComponent>(
			TEXT("CombatBrainComponent"));

	DebugBodyMesh =
		CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugBodyMesh"));
	DebugBodyMesh->SetupAttachment(GetCapsuleComponent());
	DebugBodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 45.0f));
	DebugBodyMesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 0.9f));
	DebugBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	CombatRoleLabel =
		CreateDefaultSubobject<UTextRenderComponent>(TEXT("CombatRoleLabel"));
	CombatRoleLabel->SetupAttachment(GetCapsuleComponent());
	CombatRoleLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
	CombatRoleLabel->SetHorizontalAlignment(EHTA_Center);
	CombatRoleLabel->SetWorldSize(22.0f);
	CombatRoleLabel->SetTextRenderColor(FColor::Orange);
	CombatRoleLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		DebugBodyMesh->SetStaticMesh(CubeMesh.Object);
	}

	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->bRunPhysicsWithNoController = true;
	GetCharacterMovement()->MaxWalkSpeed = 350.0f;
}

void AZorbaEnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		AbilitySystemComponent->SetNumericAttributeBase(
			UZorbaCombatAttributeSet::GetMaxHealthAttribute(),
			FMath::Max(1.0f, InitialHealth));
		AbilitySystemComponent->SetNumericAttributeBase(
			UZorbaCombatAttributeSet::GetHealthAttribute(),
			FMath::Max(1.0f, InitialHealth));
		AbilitySystemComponent->SetNumericAttributeBase(
			UZorbaCombatAttributeSet::GetMaxCombatStaminaAttribute(),
			FMath::Max(1.0f, InitialCombatStamina));
		AbilitySystemComponent->SetNumericAttributeBase(
			UZorbaCombatAttributeSet::GetCombatStaminaAttribute(),
			FMath::Max(1.0f, InitialCombatStamina));
	}

	const float SafeVisualScale = FMath::Max(0.1f, VisualScale);
	if (GetMesh())
	{
		GetMesh()->SetRelativeScale3D(
			GetMesh()->GetRelativeScale3D() * SafeVisualScale);
	}
	if (DebugBodyMesh)
	{
		DebugBodyMesh->SetRelativeScale3D(
			DebugBodyMesh->GetRelativeScale3D() * SafeVisualScale);
	}
	if (CombatRoleLabel)
	{
		RefreshCombatRoleLabel();
	}
}

UAbilitySystemComponent* AZorbaEnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

FGenericTeamId AZorbaEnemyCharacter::GetGenericTeamId() const
{
	return FGenericTeamId(TeamId);
}

void AZorbaEnemyCharacter::SetGenericTeamId(
	const FGenericTeamId& NewTeamId)
{
	TeamId = NewTeamId.GetId();
}

bool AZorbaEnemyCharacter::IsDead() const
{
	return !CombatAttributes
		|| CombatAttributes->GetHealth() <= 0.0f
		|| (AbilitySystemComponent
			&& AbilitySystemComponent->HasMatchingGameplayTag(
				ZorbaGameplayTags::State_Dead));
}

bool AZorbaEnemyCharacter::IsExhausted() const
{
	return AbilitySystemComponent
		&& AbilitySystemComponent->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Exhausted);
}

bool AZorbaEnemyCharacter::IsEnraged() const
{
	return AbilitySystemComponent
		&& AbilitySystemComponent->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Enraged);
}

bool AZorbaEnemyCharacter::IsStunned() const
{
	return AbilitySystemComponent
		&& AbilitySystemComponent->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Stunned);
}

bool AZorbaEnemyCharacter::IsDefending() const
{
	return AbilitySystemComponent
		&& AbilitySystemComponent->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Defending);
}

EZorbaMeleeDefenseResult AZorbaEnemyCharacter::ResolveIncomingMeleeHit(
	AActor* SourceActor,
	const UZorbaAttackDefinition* IncomingAttack,
	float& InOutHealthDamage,
	float& InOutStaminaDamage)
{
	if (!IsDefending() || !IsValid(SourceActor))
	{
		return EZorbaMeleeDefenseResult::None;
	}

	FVector ToSource = SourceActor->GetActorLocation() - GetActorLocation();
	ToSource.Z = 0.0f;
	ToSource = ToSource.GetSafeNormal();
	const float MinimumDefenseDot = FMath::Cos(
		FMath::DegreesToRadians(DefenseHalfAngleDegrees));
	if (ToSource.IsNearlyZero()
		|| FVector::DotProduct(GetActorForwardVector().GetSafeNormal2D(), ToSource)
			< MinimumDefenseDot)
	{
		return EZorbaMeleeDefenseResult::None;
	}

	const EZorbaDefenseInteraction Interaction = IncomingAttack
		? IncomingAttack->DefenseInteraction
		: EZorbaDefenseInteraction::Standard;
	if (Interaction == EZorbaDefenseInteraction::DodgeOnly)
	{
		return EZorbaMeleeDefenseResult::None;
	}
	if (Interaction == EZorbaDefenseInteraction::GuardBreak)
	{
		StopDefend();
		OnGuardBroken(SourceActor);
#if !UE_BUILD_SHIPPING
		DrawDebugString(
			GetWorld(),
			GetActorLocation() + FVector(0.0f, 0.0f, 165.0f),
			TEXT("GUARD BREAK"),
			nullptr,
			FColor::Orange,
			1.0f,
			true,
			1.25f);
#endif
		return EZorbaMeleeDefenseResult::GuardBroken;
	}

	InOutHealthDamage *= FMath::Clamp(
		BlockedHealthDamageMultiplier,
		0.0f,
		1.0f);
	InOutStaminaDamage *= FMath::Max(
		0.0f,
		BlockedStaminaDamageMultiplier);
	return EZorbaMeleeDefenseResult::Blocked;
}

void AZorbaEnemyCharacter::StartDefend(float Duration)
{
	if (!HasAuthority()
		|| !AbilitySystemComponent
		|| IsDead()
		|| IsExhausted()
		|| IsEnraged()
		|| IsStunned()
		|| AbilitySystemComponent->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_HitReact)
		|| (MeleeCombatComponent && MeleeCombatComponent->IsAttackInProgress()))
	{
		return;
	}

	const bool bWasDefending = IsDefending();
	AbilitySystemComponent->AddLooseGameplayTag(
		ZorbaGameplayTags::State_Defending);
	GetCharacterMovement()->StopMovementImmediately();
	GetWorldTimerManager().SetTimer(
		DefenseTimerHandle,
		this,
		&AZorbaEnemyCharacter::StopDefend,
		FMath::Max(0.05f, Duration),
		false);
	if (!bWasDefending)
	{
		OnDefendStarted();
#if !UE_BUILD_SHIPPING
		DrawDebugString(
			GetWorld(),
			GetActorLocation() + FVector(0.0f, 0.0f, 165.0f),
			TEXT("GUARD"),
			nullptr,
			FColor::Cyan,
			FMath::Max(0.05f, Duration),
			true,
			1.25f);
#endif
		UE_LOG(LogTemp, Log, TEXT("Enemy defense started: %s"), *GetNameSafe(this));
	}
}

void AZorbaEnemyCharacter::StopDefend()
{
	GetWorldTimerManager().ClearTimer(DefenseTimerHandle);
	if (!AbilitySystemComponent || !IsDefending())
	{
		return;
	}
	AbilitySystemComponent->RemoveLooseGameplayTag(
		ZorbaGameplayTags::State_Defending);
	OnDefendStopped();
	UE_LOG(LogTemp, Log, TEXT("Enemy defense stopped: %s"), *GetNameSafe(this));
}

bool AZorbaEnemyCharacter::StartEnrage()
{
	if (!HasAuthority()
		|| !AbilitySystemComponent
		|| !bCanEnrage
		|| bEnragePatternTriggered
		|| CombatRank == EZorbaEnemyCombatRank::Fodder
		|| IsDead()
		|| IsExhausted()
		|| IsStunned())
	{
		return false;
	}

	bEnragePatternTriggered = true;
	StopDefend();
	AbilitySystemComponent->AddLooseGameplayTag(
		ZorbaGameplayTags::State_Enraged);
	RefreshCombatRoleLabel();
	OnEnrageStarted();

#if !UE_BUILD_SHIPPING
	DrawDebugString(
		GetWorld(),
		GetActorLocation() + FVector(0.0f, 0.0f, 185.0f),
		TEXT("ENRAGED - BREAK WITH FORBIDDEN 1"),
		nullptr,
		FColor::Red,
		2.5f,
		true,
		1.25f);
#endif

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Enemy enrage pattern started: Enemy=%s Health=%.1f"),
		*GetNameSafe(this),
		CombatAttributes ? CombatAttributes->GetHealth() : 0.0f);
	return true;
}

bool AZorbaEnemyCharacter::BreakEnrageWithForbiddenTechnique(
	AActor* SourceActor,
	float StunDuration)
{
	if (!HasAuthority()
		|| !AbilitySystemComponent
		|| !IsValid(SourceActor)
		|| IsDead()
		|| !IsEnraged()
		|| IsStunned())
	{
		return false;
	}

	const float SafeStunDuration = FMath::Max(0.05f, StunDuration);
	AbilitySystemComponent->RemoveLooseGameplayTag(
		ZorbaGameplayTags::State_Enraged);
	AbilitySystemComponent->AddLooseGameplayTag(
		ZorbaGameplayTags::State_Stunned);
	OnEnrageEnded(true);
	StopDefend();
	if (MeleeCombatComponent)
	{
		MeleeCombatComponent->InterruptAttack(0.0f);
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetWorldTimerManager().SetTimer(
		ForbiddenTechniqueStunTimerHandle,
		this,
		&AZorbaEnemyCharacter::ClearForbiddenTechniqueStun,
		SafeStunDuration,
		false);
	RefreshCombatRoleLabel();
	OnForbiddenTechniqueStunned(SourceActor, SafeStunDuration);

#if !UE_BUILD_SHIPPING
	DrawDebugString(
		GetWorld(),
		GetActorLocation() + FVector(0.0f, 0.0f, 185.0f),
		TEXT("FORBIDDEN BREAK - STUNNED"),
		nullptr,
		FColor(160, 64, 255),
		SafeStunDuration,
		true,
		1.35f);
#endif

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Forbidden technique broke enrage: Enemy=%s Source=%s Stun=%.2f"),
		*GetNameSafe(this),
		*GetNameSafe(SourceActor),
		SafeStunDuration);
	return true;
}

void AZorbaEnemyCharacter::HandleMeleeHit(
	AActor* SourceActor,
	const FHitResult& HitResult)
{
	if (!CombatAttributes || !AbilitySystemComponent)
	{
		return;
	}

	const float RemainingHealth = CombatAttributes->GetHealth();
	const float RemainingStamina = CombatAttributes->GetCombatStamina();

	if (RemainingStamina <= 0.0f)
	{
		EnterExhausted();
	}

	OnMeleeHitReceived(
		SourceActor,
		HitResult,
		RemainingHealth,
		RemainingStamina);

	if (RemainingHealth <= 0.0f)
	{
		Die(SourceActor, HitResult);
		return;
	}

	TryStartEnrageFromHealth();

	FVector ReactionDirection = FVector::ZeroVector;
	if (SourceActor)
	{
		ReactionDirection =
			(GetActorLocation() - SourceActor->GetActorLocation())
			.GetSafeNormal2D();
	}

	if (ReactionDirection.IsNearlyZero())
	{
		ReactionDirection = HitResult.ImpactNormal.GetSafeNormal2D();
	}

	LaunchCharacter(
		ReactionDirection * HitReactionStrength + FVector(0.0f, 0.0f, 60.0f),
		true,
		false);

	if (GetWorld())
	{
		DrawDebugSphere(
			GetWorld(),
			HitResult.ImpactPoint,
			18.0f,
			12,
			FColor::Red,
			false,
			1.0f,
			0,
			2.0f);
	}
}

void AZorbaEnemyCharacter::HandleParried(
	AActor* ParryingActor,
	const FHitResult& HitResult,
	float StaminaDamage)
{
	if (!HasAuthority()
		|| !AbilitySystemComponent
		|| !CombatAttributes)
	{
		return;
	}

	if (CombatRank == EZorbaEnemyCombatRank::Fodder)
	{
		const float HealthBefore = CombatAttributes->GetHealth();
		if (MeleeCombatComponent)
		{
			MeleeCombatComponent->InterruptAttack();
		}
		OnParried(ParryingActor, HitResult, CombatAttributes->GetCombatStamina());
		OnParryInstantKill(ParryingActor, HitResult);
		ApplyCombatDeltaFrom(
			ParryingActor,
			HitResult,
			HealthBefore,
			0.0f);
		HandleMeleeHit(ParryingActor, HitResult);
		if (IsDead())
		{
			if (AZorbaCharacter* ParryingCharacter =
				Cast<AZorbaCharacter>(ParryingActor))
			{
				ParryingCharacter->ApplyFodderParryExecutionBenefits();
			}
		}
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Fodder parry instant kill: Enemy=%s Health=%.1f->%.1f"),
			*GetNameSafe(this),
			HealthBefore,
			CombatAttributes->GetHealth());
		return;
	}

	const float StaminaBefore = CombatAttributes->GetCombatStamina();
	ApplyCombatDeltaFrom(
		ParryingActor,
		HitResult,
		0.0f,
		StaminaDamage);

	if (MeleeCombatComponent)
	{
		MeleeCombatComponent->InterruptAttack();
	}

	AbilitySystemComponent->AddLooseGameplayTag(
		ZorbaGameplayTags::State_HitReact);
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			HitReactTimerHandle,
			this,
			&AZorbaEnemyCharacter::ClearHitReact,
			ParryStaggerDuration,
			false);
		DrawDebugSphere(
			World,
			HitResult.ImpactPoint,
			28.0f,
			12,
			FColor::Green,
			false,
			1.0f,
			0,
			3.0f);
	}

	const float StaminaAfter = CombatAttributes->GetCombatStamina();
	if (StaminaAfter <= 0.0f)
	{
		EnterExhausted();
	}

	OnParried(ParryingActor, HitResult, StaminaAfter);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Enemy parried: %s Stamina %.1f -> %.1f"),
		*GetNameSafe(this),
		StaminaBefore,
		StaminaAfter);
}

void AZorbaEnemyCharacter::ConsumeExhausted()
{
	if (!AbilitySystemComponent || !CombatAttributes || !IsExhausted())
	{
		return;
	}

	GetWorldTimerManager().ClearTimer(ExhaustedTimerHandle);
	AbilitySystemComponent->RemoveLooseGameplayTag(
		ZorbaGameplayTags::State_Exhausted);
	AbilitySystemComponent->RemoveLooseGameplayTag(
		ZorbaGameplayTags::State_StaminaDepleted);

	const float DesiredStamina = FMath::Min(
		CombatAttributes->GetMaxCombatStamina(),
		FMath::Max(0.0f, ExhaustedRecoveryStamina));
	AbilitySystemComponent->ApplyModToAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute(),
		EGameplayModOp::Additive,
		DesiredStamina - CombatAttributes->GetCombatStamina());

	if (!bSpecialAttackReactionActive && !IsDead())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	OnExhaustedEnded();
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Enemy exhausted state consumed: %s Stamina=%.1f"),
		*GetNameSafe(this),
		CombatAttributes->GetCombatStamina());
}

void AZorbaEnemyCharacter::HandleExecuted(
	AActor* ExecutingActor,
	const FHitResult& HitResult)
{
	OnExecuted(ExecutingActor, HitResult);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Enemy executed: %s By=%s"),
		*GetNameSafe(this),
		*GetNameSafe(ExecutingActor));
}

bool AZorbaEnemyCharacter::ApplyCombatDeltaFrom(
	AActor* SourceActor,
	const FHitResult& HitResult,
	float HealthDamage,
	float StaminaDamage)
{
	if (!AbilitySystemComponent)
	{
		return false;
	}

	UAbilitySystemComponent* SourceAbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(SourceActor);
	if (SourceAbilitySystem)
	{
		FGameplayEffectContextHandle EffectContext =
			SourceAbilitySystem->MakeEffectContext();
		EffectContext.AddSourceObject(SourceActor);
		EffectContext.AddHitResult(HitResult, true);

		FGameplayEffectSpecHandle EffectSpec =
			SourceAbilitySystem->MakeOutgoingSpec(
				UZorbaMeleeDamageEffect::StaticClass(),
				1.0f,
				EffectContext);
		if (EffectSpec.IsValid())
		{
			EffectSpec.Data->SetSetByCallerMagnitude(
				UZorbaMeleeDamageEffect::HealthDamageDataName,
				-FMath::Max(0.0f, HealthDamage));
			EffectSpec.Data->SetSetByCallerMagnitude(
				UZorbaMeleeDamageEffect::StaminaDamageDataName,
				-FMath::Max(0.0f, StaminaDamage));
			return SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(
				*EffectSpec.Data.Get(),
				AbilitySystemComponent).WasSuccessfullyApplied();
		}
	}

	AbilitySystemComponent->ApplyModToAttribute(
		UZorbaCombatAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive,
		-FMath::Max(0.0f, HealthDamage));
	AbilitySystemComponent->ApplyModToAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute(),
		EGameplayModOp::Additive,
		-FMath::Max(0.0f, StaminaDamage));
	return true;
}

void AZorbaEnemyCharacter::Die(
	AActor* SourceActor,
	const FHitResult& HitResult)
{
	if (!AbilitySystemComponent
		|| AbilitySystemComponent->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead))
	{
		return;
	}

	AbilitySystemComponent->AddLooseGameplayTag(
		ZorbaGameplayTags::State_Dead);
	GetWorldTimerManager().ClearTimer(ExhaustedTimerHandle);
	GetWorldTimerManager().ClearTimer(HitReactTimerHandle);
	GetWorldTimerManager().ClearTimer(ForbiddenTechniqueStunTimerHandle);
	if (IsEnraged())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(
			ZorbaGameplayTags::State_Enraged);
		OnEnrageEnded(false);
	}
	if (IsStunned())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(
			ZorbaGameplayTags::State_Stunned);
	}
	StopDefend();
	if (MeleeCombatComponent)
	{
		MeleeCombatComponent->InterruptAttack();
	}
	if (BasicAttackComponent)
	{
		BasicAttackComponent->StopCrowdCombat();
	}
	if (CombatBrainComponent)
	{
		CombatBrainComponent->StopBrain();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionResponseToChannel(
		ECC_Pawn,
		ECR_Ignore);
	if (CombatRoleLabel)
	{
		CombatRoleLabel->SetVisibility(false);
	}

	OnCombatDeath(SourceActor, HitResult);
	OnEnemyDied.Broadcast(this, SourceActor, HitResult);
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Enemy combat death: Enemy=%s Rank=%s"),
		*GetNameSafe(this),
		CombatRank == EZorbaEnemyCombatRank::Fodder
			? TEXT("Fodder")
			: CombatRank == EZorbaEnemyCombatRank::Elite
				? TEXT("Elite")
				: TEXT("Boss"));
}

void AZorbaEnemyCharacter::BeginSpecialAttackReaction(
	AActor* AttackingActor,
	EZorbaAttackKind AttackKind)
{
	if (!AbilitySystemComponent || IsDead())
	{
		return;
	}

	bSpecialAttackReactionActive = true;
	GetWorldTimerManager().ClearTimer(HitReactTimerHandle);
	if (IsExhausted())
	{
		GetWorldTimerManager().PauseTimer(ExhaustedTimerHandle);
	}
	AbilitySystemComponent->AddLooseGameplayTag(
		ZorbaGameplayTags::State_HitReact);
	if (MeleeCombatComponent)
	{
		MeleeCombatComponent->InterruptAttack();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	OnSpecialAttackReactionStarted(AttackingActor, AttackKind);
}

void AZorbaEnemyCharacter::EndSpecialAttackReaction(
	EZorbaAttackKind AttackKind,
	bool bAttackLanded)
{
	bSpecialAttackReactionActive = false;
	if (AbilitySystemComponent && !IsDead())
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(
			ZorbaGameplayTags::State_HitReact);
		if (IsExhausted())
		{
			GetWorldTimerManager().UnPauseTimer(ExhaustedTimerHandle);
		}
		else
		{
			GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		}
	}

	OnSpecialAttackReactionEnded(AttackKind, bAttackLanded);
}

void AZorbaEnemyCharacter::EnterExhausted()
{
	if (!AbilitySystemComponent || IsDead())
	{
		return;
	}

	const bool bWasExhausted = IsExhausted();
	if (!AbilitySystemComponent->HasMatchingGameplayTag(
		ZorbaGameplayTags::State_StaminaDepleted))
	{
		AbilitySystemComponent->AddLooseGameplayTag(
			ZorbaGameplayTags::State_StaminaDepleted);
	}
	if (!bWasExhausted)
	{
		AbilitySystemComponent->AddLooseGameplayTag(
			ZorbaGameplayTags::State_Exhausted);
	}
	if (MeleeCombatComponent)
	{
		MeleeCombatComponent->InterruptAttack();
	}
	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();

	GetWorldTimerManager().SetTimer(
		ExhaustedTimerHandle,
		this,
		&AZorbaEnemyCharacter::RecoverFromExhausted,
		FMath::Max(0.1f, ExhaustedDuration),
		false);
	if (bSpecialAttackReactionActive)
	{
		GetWorldTimerManager().PauseTimer(ExhaustedTimerHandle);
	}

	if (!bWasExhausted)
	{
		OnCombatStaminaDepleted();
		OnExhaustedStarted();
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Enemy exhausted: %s Duration=%.1f"),
			*GetNameSafe(this),
			ExhaustedDuration);
	}
}

void AZorbaEnemyCharacter::RecoverFromExhausted()
{
	ConsumeExhausted();
}

void AZorbaEnemyCharacter::ClearHitReact()
{
	if (AbilitySystemComponent && !bSpecialAttackReactionActive)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(
			ZorbaGameplayTags::State_HitReact);
	}
}

void AZorbaEnemyCharacter::TryStartEnrageFromHealth()
{
	if (!CombatAttributes
		|| bEnragePatternTriggered
		|| CombatRank == EZorbaEnemyCombatRank::Fodder)
	{
		return;
	}

	const float MaxHealth = CombatAttributes->GetMaxHealth();
	if (MaxHealth > 0.0f
		&& CombatAttributes->GetHealth()
			<= MaxHealth * FMath::Clamp(EnrageHealthThresholdFraction, 0.05f, 0.95f))
	{
		StartEnrage();
	}
}

void AZorbaEnemyCharacter::ClearForbiddenTechniqueStun()
{
	GetWorldTimerManager().ClearTimer(ForbiddenTechniqueStunTimerHandle);
	if (!AbilitySystemComponent || !IsStunned())
	{
		return;
	}

	AbilitySystemComponent->RemoveLooseGameplayTag(
		ZorbaGameplayTags::State_Stunned);
	const bool bHasOtherHitReaction =
		AbilitySystemComponent->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_HitReact);
	if (!IsDead()
		&& !IsExhausted()
		&& !bSpecialAttackReactionActive
		&& !bHasOtherHitReaction)
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}
	RefreshCombatRoleLabel();
	OnForbiddenTechniqueStunEnded();
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Forbidden technique stun ended: Enemy=%s Enraged=%s"),
		*GetNameSafe(this),
		IsEnraged() ? TEXT("true") : TEXT("false"));
}

void AZorbaEnemyCharacter::RefreshCombatRoleLabel()
{
	if (!CombatRoleLabel)
	{
		return;
	}

	const bool bFodder = CombatRank == EZorbaEnemyCombatRank::Fodder;
	const TCHAR* RankLabel = bFodder
		? TEXT("FODDER")
		: CombatRank == EZorbaEnemyCombatRank::Boss
			? TEXT("BOSS")
			: TEXT("ELITE");
	if (IsStunned())
	{
		CombatRoleLabel->SetText(
			FText::FromString(FString::Printf(TEXT("%s\nSTUNNED"), RankLabel)));
		CombatRoleLabel->SetTextRenderColor(FColor(160, 64, 255));
	}
	else if (IsEnraged())
	{
		CombatRoleLabel->SetText(
			FText::FromString(FString::Printf(TEXT("%s\nENRAGED"), RankLabel)));
		CombatRoleLabel->SetTextRenderColor(FColor::Red);
	}
	else
	{
		CombatRoleLabel->SetText(FText::FromString(RankLabel));
		CombatRoleLabel->SetTextRenderColor(
			bFodder ? FColor::Yellow : FColor::Orange);
	}
	CombatRoleLabel->SetVisibility(bShowCombatRoleLabel && !IsDead());
}
