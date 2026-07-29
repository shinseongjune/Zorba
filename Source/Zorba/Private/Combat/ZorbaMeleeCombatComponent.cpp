// Copyright Epic Games, Inc. All Rights Reserved.

#include "Combat/ZorbaMeleeCombatComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Combat/ZorbaAttackDefinition.h"
#include "Combat/ZorbaCombatAttributeSet.h"
#include "Combat/ZorbaGameplayTags.h"
#include "Combat/ZorbaMeleeDamageEffect.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "DrawDebugHelpers.h"
#include "Enemy/ZorbaEnemyCharacter.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "Player/ZorbaCharacter.h"
#include "TimerManager.h"

namespace
{
	const FName AttackActiveNotifyName(TEXT("AttackActive"));
	const FName HitCommitNotifyName(TEXT("HitCommit"));
}

UZorbaMeleeCombatComponent::UZorbaMeleeCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UZorbaMeleeCombatComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bAttackInProgress)
	{
		const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
		UAnimInstance* AnimInstance = BoundAnimInstance.Get();
		UAnimMontage* Montage = ActiveMontage.Get();
		const FAnimMontageInstance* MontageInstance =
			AnimInstance
				? AnimInstance->GetMontageInstanceForID(ActiveMontageInstanceId)
				: nullptr;

		const bool bRuntimeStateValid = OwnerCharacter
			&& OwnerCharacter->GetMesh()
			&& OwnerCharacter->GetMesh()->GetAnimInstance() == AnimInstance
			&& MontageInstance
			&& MontageInstance->Montage == Montage;

		if (!bRuntimeStateValid)
		{
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("Melee attack cleaned up after its Montage or AnimInstance became invalid."));
			CleanupAttack();
			return;
		}
	}

	if (bHitWindowOpen)
	{
		SampleWeaponTrace();
	}
}

bool UZorbaMeleeCombatComponent::BeginAttack(
	UZorbaAttackDefinition* AttackDefinition,
	const FVector& AttackDirection,
	AActor* LockedTarget)
{
	if (bAttackInProgress)
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("Melee attack ignored: another attack is still active."));
		return false;
	}

	if (!IsValid(AttackDefinition))
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Melee attack rejected: AttackDefinition is missing."));
		return false;
	}

	if (!CanPayAttackCost(AttackDefinition))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Melee attack rejected: insufficient combat stamina for %s."),
			*AttackDefinition->AttackId.ToString());
		return false;
	}

	FVector FlatDirection = AttackDirection;
	FlatDirection.Z = 0.0f;
	FlatDirection = FlatDirection.GetSafeNormal();

	if (FlatDirection.IsNearlyZero())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Melee attack rejected: AttackDirection is zero."));
		return false;
	}

	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());
	if (!OwnerCharacter || !OwnerCharacter->GetMesh())
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Melee attack rejected: owner is not an animated Character."));
		return false;
	}

	UAnimInstance* AnimInstance =
		OwnerCharacter->GetMesh()->GetAnimInstance();
	UAnimMontage* Montage = AttackDefinition->Montage.LoadSynchronous();

	if (!AnimInstance || !Montage)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Melee attack rejected: Data Asset Montage or AnimInstance is missing."));
		return false;
	}

	ActiveAttackDefinition = AttackDefinition;
	ActiveMontage = Montage;
	ActiveLockedTarget = LockedTarget;
	CurrentAttackDirection = FlatDirection;
	bActiveAttackGrantedInvulnerability = false;
	bAttackInProgress = true;
	bHitWindowOpen = false;
	bHitCommitted = false;
	bHitLanded = false;
	bHeavyBranchWindowOpen = false;
	ActorsHitThisAttack.Reset();
	WeaponContacts.Reset();
	ResolveTraceComponents();

	BindAnimInstance(AnimInstance);

	const float MontageDuration = AnimInstance->Montage_Play(
		Montage,
		1.0f,
		EMontagePlayReturnType::MontageLength,
		0.0f,
		false);
	if (MontageDuration <= 0.0f)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Melee attack rejected: Montage_Play failed for %s."),
			*GetNameSafe(Montage));
		CleanupAttack();
		return false;
	}

	if (const FAnimMontageInstance* MontageInstance =
		AnimInstance->GetActiveInstanceForMontage(Montage))
	{
		ActiveMontageInstanceId = MontageInstance->GetInstanceID();
	}
	else
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Melee attack rejected: active Montage instance was not created for %s."),
			*GetNameSafe(Montage));
		CleanupAttack();
		return false;
	}

	OwnerCharacter->SetActorRotation(
		FRotator(0.0f, CurrentAttackDirection.Rotation().Yaw, 0.0f));

	AddTickPrerequisiteComponent(OwnerCharacter->GetMesh());
	SetComponentTickEnabled(true);

	FOnMontageBlendingOutStarted BlendingOutDelegate;
	BlendingOutDelegate.BindUObject(
		this,
		&UZorbaMeleeCombatComponent::HandleMontageBlendingOut);
	AnimInstance->Montage_SetBlendingOutDelegate(
		BlendingOutDelegate,
		Montage);

	FOnMontageEnded EndedDelegate;
	EndedDelegate.BindUObject(
		this,
		&UZorbaMeleeCombatComponent::HandleMontageEnded);
	AnimInstance->Montage_SetEndDelegate(EndedDelegate, Montage);

	if (UAbilitySystemComponent* SourceAbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner()))
	{
		SourceAbilitySystem->AddLooseGameplayTag(
			ZorbaGameplayTags::State_Attacking);
		if (AttackDefinition->AttackKind == EZorbaAttackKind::Opportunity
			|| AttackDefinition->AttackKind == EZorbaAttackKind::Execution)
		{
			SourceAbilitySystem->AddLooseGameplayTag(
				ZorbaGameplayTags::State_Invulnerable);
			bActiveAttackGrantedInvulnerability = true;
		}
		AttackTagAbilitySystem = SourceAbilitySystem;
	}

	PayAttackCost(AttackDefinition);
	BeginLockedTargetPresentation();

	if (bDrawAttackDirection && GetWorld())
	{
		float ArrowLength = 200.0f;
		if (AttackDefinition->HitPhases.IsValidIndex(0))
		{
			ArrowLength = FMath::Max(
				AttackDefinition->HitPhases[0].Range,
				100.0f);
		}

		const FVector ArrowStart =
			OwnerCharacter->GetActorLocation()
			+ FVector(0.0f, 0.0f, 100.0f);

		DrawDebugDirectionalArrow(
			GetWorld(),
			ArrowStart,
			ArrowStart + CurrentAttackDirection * ArrowLength,
			25.0f,
			FColor::Cyan,
			false,
			1.25f,
			0,
			4.0f);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee attack started: %s, Direction=%s, Montage=%s"),
		*AttackDefinition->AttackId.ToString(),
		*CurrentAttackDirection.ToCompactString(),
		*GetNameSafe(Montage));

	return true;
}

bool UZorbaMeleeCombatComponent::TransitionAttack(
	UZorbaAttackDefinition* AttackDefinition,
	const FVector& AttackDirection)
{
	if (!bAttackInProgress
		|| !bHeavyBranchWindowOpen
		|| !ActiveAttackDefinition
		|| ActiveAttackDefinition->AttackKind != EZorbaAttackKind::Standard
		|| !ActiveAttackDefinition->bOpenHeavyBranchAfterHitWindow
		|| !AttackDefinition
		|| AttackDefinition->AttackKind != EZorbaAttackKind::DerivedHeavy)
	{
		return false;
	}

	InterruptAttack(0.08f);
	return BeginAttack(AttackDefinition, AttackDirection);
}

AActor* UZorbaMeleeCombatComponent::FindBestEligibleTarget(
	UZorbaAttackDefinition* AttackDefinition,
	const FVector& AttackDirection) const
{
	if (!IsValid(AttackDefinition)
		|| !AttackDefinition->HitPhases.IsValidIndex(0))
	{
		return nullptr;
	}

	FVector FlatDirection = AttackDirection.GetSafeNormal2D();
	if (FlatDirection.IsNearlyZero())
	{
		return nullptr;
	}

	TArray<AActor*> EligibleTargets;
	GatherEligibleTargets(
		AttackDefinition,
		AttackDefinition->HitPhases[0],
		FlatDirection,
		false,
		EligibleTargets);
	return EligibleTargets.IsValidIndex(0) ? EligibleTargets[0] : nullptr;
}

bool UZorbaMeleeCombatComponent::CanPayAttackCost(
	const UZorbaAttackDefinition* AttackDefinition) const
{
	if (!AttackDefinition)
	{
		return false;
	}

	const float StaminaCost = FMath::Max(
		0.0f,
		AttackDefinition->CombatStaminaCost.GetValueAtLevel(1.0f));
	if (StaminaCost <= 0.0f)
	{
		return true;
	}

	const UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	return AbilitySystem
		&& !AbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead)
		&& AbilitySystem->GetNumericAttribute(
			UZorbaCombatAttributeSet::GetCombatStaminaAttribute())
			>= StaminaCost;
}

void UZorbaMeleeCombatComponent::PayAttackCost(
	const UZorbaAttackDefinition* AttackDefinition)
{
	if (!AttackDefinition)
	{
		return;
	}

	const float StaminaCost = FMath::Max(
		0.0f,
		AttackDefinition->CombatStaminaCost.GetValueAtLevel(1.0f));
	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!AbilitySystem || StaminaCost <= 0.0f)
	{
		return;
	}

	AbilitySystem->ApplyModToAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute(),
		EGameplayModOp::Additive,
		-StaminaCost);

	const float StaminaAfter = AbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute());
	if (StaminaAfter <= 0.0f)
	{
		AbilitySystem->AddLooseGameplayTag(
			ZorbaGameplayTags::State_StaminaDepleted);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee attack cost paid: Attack=%s Cost=%.1f Remaining=%.1f"),
		*AttackDefinition->AttackId.ToString(),
		StaminaCost,
		StaminaAfter);
}

void UZorbaMeleeCombatComponent::BeginLockedTargetPresentation()
{
	AActor* OwnerActor = GetOwner();
	AActor* TargetActor = ActiveLockedTarget.Get();
	if (!OwnerActor || !TargetActor || !ActiveAttackDefinition)
	{
		return;
	}

	const EZorbaAttackKind AttackKind =
		ActiveAttackDefinition->AttackKind;
	if (AttackKind != EZorbaAttackKind::Opportunity
		&& AttackKind != EZorbaAttackKind::Execution)
	{
		return;
	}

	FVector ToTarget = TargetActor->GetActorLocation()
		- OwnerActor->GetActorLocation();
	ToTarget.Z = 0.0f;
	ToTarget = ToTarget.GetSafeNormal();
	if (!ToTarget.IsNearlyZero())
	{
		const float AlignmentDistance = FMath::Max(
			0.0f,
			ActiveAttackDefinition->SpecialTargetAlignmentDistance);
		FHitResult MoveHit;
		OwnerActor->SetActorLocation(
			TargetActor->GetActorLocation() - ToTarget * AlignmentDistance,
			true,
			&MoveHit,
			ETeleportType::TeleportPhysics);
		OwnerActor->SetActorRotation(ToTarget.Rotation());
		TargetActor->SetActorRotation((-ToTarget).Rotation());
	}

	if (AZorbaEnemyCharacter* EnemyTarget =
		Cast<AZorbaEnemyCharacter>(TargetActor))
	{
		EnemyTarget->BeginSpecialAttackReaction(OwnerActor, AttackKind);
	}

	UAnimMontage* TargetMontage =
		ActiveAttackDefinition->TargetMontage.LoadSynchronous();
	ACharacter* TargetCharacter = Cast<ACharacter>(TargetActor);
	UAnimInstance* TargetAnimInstance =
		TargetCharacter && TargetCharacter->GetMesh()
			? TargetCharacter->GetMesh()->GetAnimInstance()
			: nullptr;
	if (TargetMontage && TargetAnimInstance
		&& TargetAnimInstance->Montage_Play(TargetMontage) > 0.0f)
	{
		LockedTargetAnimInstance = TargetAnimInstance;
		ActiveTargetMontage = TargetMontage;
	}
}

void UZorbaMeleeCombatComponent::EndLockedTargetPresentation(
	bool bAttackCompleted)
{
	if (UAnimInstance* TargetAnimInstance =
		LockedTargetAnimInstance.Get())
	{
		if (UAnimMontage* TargetMontage = ActiveTargetMontage.Get())
		{
			TargetAnimInstance->Montage_Stop(0.1f, TargetMontage);
		}
	}

	if (AZorbaEnemyCharacter* EnemyTarget =
		Cast<AZorbaEnemyCharacter>(ActiveLockedTarget.Get()))
	{
		EnemyTarget->EndSpecialAttackReaction(
			ActiveAttackDefinition
				? ActiveAttackDefinition->AttackKind
				: EZorbaAttackKind::Standard,
			bAttackCompleted);
	}
}

void UZorbaMeleeCombatComponent::ApplySourceRecovery()
{
	ApplySourceRecoveryFromDefinition(
		ActiveAttackDefinition,
		TEXT("PairedExecution"));
}

void UZorbaMeleeCombatComponent::ApplyInstantExecutionBenefits(
	const UZorbaAttackDefinition* ExecutionDefinition)
{
	if (!ExecutionDefinition
		|| ExecutionDefinition->AttackKind != EZorbaAttackKind::Execution)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Instant execution benefits rejected: execution definition is missing or has the wrong kind."));
		return;
	}

	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	UWorld* World = GetWorld();
	if (!AbilitySystem || !World)
	{
		return;
	}

	if (!bInstantExecutionInvulnerabilityActive)
	{
		AbilitySystem->AddLooseGameplayTag(
			ZorbaGameplayTags::State_Invulnerable);
		bInstantExecutionInvulnerabilityActive = true;
		InstantExecutionAbilitySystem = AbilitySystem;
	}

	const float InvulnerabilityDuration = FMath::Max(
		0.0f,
		ExecutionDefinition->InstantExecutionInvulnerabilityDuration);
	World->GetTimerManager().ClearTimer(
		InstantExecutionInvulnerabilityTimerHandle);
	if (InvulnerabilityDuration > 0.0f)
	{
		World->GetTimerManager().SetTimer(
			InstantExecutionInvulnerabilityTimerHandle,
			this,
			&UZorbaMeleeCombatComponent::ClearInstantExecutionInvulnerability,
			InvulnerabilityDuration,
			false);
	}
	else
	{
		ClearInstantExecutionInvulnerability();
	}

	ApplySourceRecoveryFromDefinition(
		ExecutionDefinition,
		TEXT("FodderParryExecution"));
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Instant execution invulnerability granted: Duration=%.2f Active=%d"),
		InvulnerabilityDuration,
		bInstantExecutionInvulnerabilityActive ? 1 : 0);
}

void UZorbaMeleeCombatComponent::ApplySourceRecoveryFromDefinition(
	const UZorbaAttackDefinition* AttackDefinition,
	const TCHAR* RecoveryReason)
{
	if (!AttackDefinition)
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner());
	if (!AbilitySystem)
	{
		return;
	}

	const float HealthRecovery = FMath::Max(
		0.0f,
		AttackDefinition->SourceHealthRecovery.GetValueAtLevel(1.0f));
	const float StaminaRecovery = FMath::Max(
		0.0f,
		AttackDefinition->SourceStaminaRecovery.GetValueAtLevel(1.0f));
	const float HealthBefore = AbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetHealthAttribute());
	const float StaminaBefore = AbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute());
	AbilitySystem->ApplyModToAttribute(
		UZorbaCombatAttributeSet::GetHealthAttribute(),
		EGameplayModOp::Additive,
		HealthRecovery);
	AbilitySystem->ApplyModToAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute(),
		EGameplayModOp::Additive,
		StaminaRecovery);

	if (AbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute()) > 0.0f)
	{
		AbilitySystem->RemoveLooseGameplayTag(
			ZorbaGameplayTags::State_StaminaDepleted);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Execution recovery applied: Health=%.1f Stamina=%.1f Reason=%s Values=%.1f->%.1f/%.1f->%.1f"),
		HealthRecovery,
		StaminaRecovery,
		RecoveryReason,
		HealthBefore,
		AbilitySystem->GetNumericAttribute(
			UZorbaCombatAttributeSet::GetHealthAttribute()),
		StaminaBefore,
		AbilitySystem->GetNumericAttribute(
			UZorbaCombatAttributeSet::GetCombatStaminaAttribute()));
}

void UZorbaMeleeCombatComponent::ClearInstantExecutionInvulnerability()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(
			InstantExecutionInvulnerabilityTimerHandle);
	}
	if (bInstantExecutionInvulnerabilityActive)
	{
		if (UAbilitySystemComponent* AbilitySystem =
			InstantExecutionAbilitySystem.Get())
		{
			AbilitySystem->RemoveLooseGameplayTag(
				ZorbaGameplayTags::State_Invulnerable);
		}
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Instant execution invulnerability ended."));
	}
	bInstantExecutionInvulnerabilityActive = false;
	InstantExecutionAbilitySystem.Reset();
}

FVector UZorbaMeleeCombatComponent::GetCurrentAttackDirection() const
{
	return CurrentAttackDirection;
}

bool UZorbaMeleeCombatComponent::InterruptAttack(float BlendOutTime)
{
	if (!bAttackInProgress)
	{
		return false;
	}

	UAnimInstance* AnimInstance = BoundAnimInstance.Get();
	UAnimMontage* Montage = ActiveMontage.Get();
	const FString MontageName = GetNameSafe(Montage);

	if (AnimInstance && Montage)
	{
		AnimInstance->Montage_Stop(
			FMath::Max(0.0f, BlendOutTime),
			Montage);
	}

	if (bAttackInProgress)
	{
		CleanupAttack();
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee attack interrupted: Owner=%s Montage=%s"),
		*GetNameSafe(GetOwner()),
		*MontageName);

	return true;
}

void UZorbaMeleeCombatComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ClearInstantExecutionInvulnerability();
	CleanupAttack();
	Super::EndPlay(EndPlayReason);
}

void UZorbaMeleeCombatComponent::HandleMontageNotifyBegin(
	FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (!IsNotifyFromActiveMontage(BranchingPointPayload))
	{
		return;
	}

	if (NotifyName == AttackActiveNotifyName)
	{
		OpenHitWindow();
	}
	else if (NotifyName == HitCommitNotifyName)
	{
		CommitHit();
	}
}

void UZorbaMeleeCombatComponent::HandleMontageNotifyEnd(
	FName NotifyName,
	const FBranchingPointNotifyPayload& BranchingPointPayload)
{
	if (NotifyName == AttackActiveNotifyName
		&& IsNotifyFromActiveMontage(BranchingPointPayload))
	{
		CloseHitWindow();
		if (ActiveAttackDefinition
			&& ActiveAttackDefinition->AttackKind == EZorbaAttackKind::Standard
			&& ActiveAttackDefinition->bOpenHeavyBranchAfterHitWindow)
		{
			bHeavyBranchWindowOpen = true;
			UE_LOG(
				LogTemp,
				Verbose,
				TEXT("Derived-heavy branch window opened."));
		}
	}
}

void UZorbaMeleeCombatComponent::HandleMontageBlendingOut(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	if (Montage != ActiveMontage.Get())
	{
		return;
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee attack blending out: %s (%s)."),
		*GetNameSafe(Montage),
		bInterrupted ? TEXT("interrupted") : TEXT("completed"));

	CloseHitWindow();
}

void UZorbaMeleeCombatComponent::HandleMontageEnded(
	UAnimMontage* Montage,
	bool bInterrupted)
{
	if (Montage == ActiveMontage.Get())
	{
		CleanupAttack();
	}
}

bool UZorbaMeleeCombatComponent::IsNotifyFromActiveMontage(
	const FBranchingPointNotifyPayload& BranchingPointPayload) const
{
	return bAttackInProgress
		&& ActiveMontage.IsValid()
		&& BranchingPointPayload.SequenceAsset == ActiveMontage.Get()
		&& BranchingPointPayload.MontageInstanceID == ActiveMontageInstanceId
		&& BoundAnimInstance.IsValid()
		&& BranchingPointPayload.SkelMeshComponent
			== BoundAnimInstance->GetSkelMeshComponent();
}

void UZorbaMeleeCombatComponent::OpenHitWindow()
{
	if (!bAttackInProgress || bHitWindowOpen)
	{
		return;
	}

	bHitWindowOpen = true;
	bHasPreviousTraceSample = false;
	ResolveTraceComponents();
	SampleWeaponTrace();
	SetComponentTickEnabled(true);

	UE_LOG(LogTemp, Verbose, TEXT("Melee hit window opened."));
}

void UZorbaMeleeCombatComponent::CloseHitWindow()
{
	bHitWindowOpen = false;
	bHasPreviousTraceSample = false;
}

void UZorbaMeleeCombatComponent::CommitHit()
{
	if (!bAttackInProgress
		|| !bHitWindowOpen
		|| bHitCommitted
		|| !ActiveAttackDefinition
		|| !ActiveAttackDefinition->HitPhases.IsValidIndex(0))
	{
		return;
	}

	// Montage notifies are dispatched while the skeletal mesh ticks, before this
	// component's tick prerequisite runs. Capture the notify frame itself so the
	// resolved contact point does not lag one animation frame behind HitCommit.
	SampleWeaponTrace();

	bHitCommitted = true;

	const FZorbaAttackHitPhase& HitPhase =
		ActiveAttackDefinition->HitPhases[0];

	TArray<AActor*> EligibleTargets;
	GatherEligibleTargets(
		ActiveAttackDefinition,
		HitPhase,
		CurrentAttackDirection,
		true,
		EligibleTargets);

	const AActor* OwnerActor = GetOwner();
	const FQuat AttackRotation =
		FRotationMatrix::MakeFromXZ(
			CurrentAttackDirection,
			FVector::UpVector).ToQuat();
	const FVector PhaseOrigin =
		OwnerActor->GetActorLocation()
		+ AttackRotation.RotateVector(HitPhase.LocalOffset);

	int32 AppliedTargetCount = 0;
	const int32 MaxTargets = FMath::Max(1, HitPhase.MaxTargets);

	for (AActor* TargetActor : EligibleTargets)
	{
		if (AppliedTargetCount >= MaxTargets)
		{
			break;
		}

		const FHitResult HitResult =
			ResolveHitResult(TargetActor, PhaseOrigin);

		if (ApplyDamageToTarget(TargetActor, HitPhase, HitResult))
		{
			bHitLanded = true;
			++AppliedTargetCount;
		}
	}

	if (bDrawAttackDirection && GetWorld())
	{
		const FVector PhaseDirection =
			CurrentAttackDirection.RotateAngleAxis(
				HitPhase.LocalYawOffsetDegrees,
				FVector::UpVector);
		const FVector LeftEdge =
			PhaseDirection.RotateAngleAxis(
				-HitPhase.HalfAngleDegrees,
				FVector::UpVector);
		const FVector RightEdge =
			PhaseDirection.RotateAngleAxis(
				HitPhase.HalfAngleDegrees,
				FVector::UpVector);

		DrawDebugLine(
			GetWorld(),
			PhaseOrigin,
			PhaseOrigin + LeftEdge * HitPhase.Range,
			FColor::Yellow,
			false,
			1.0f,
			0,
			2.0f);
		DrawDebugLine(
			GetWorld(),
			PhaseOrigin,
			PhaseOrigin + RightEdge * HitPhase.Range,
			FColor::Yellow,
			false,
			1.0f,
			0,
			2.0f);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee hit committed: Candidates=%d Applied=%d."),
		EligibleTargets.Num(),
		AppliedTargetCount);
}

void UZorbaMeleeCombatComponent::CleanupAttack()
{
	EndLockedTargetPresentation(bHitLanded);
	CloseHitWindow();
	SetComponentTickEnabled(false);

	if (const ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
	{
		RemoveTickPrerequisiteComponent(OwnerCharacter->GetMesh());
	}

	if (UAbilitySystemComponent* TaggedAbilitySystem =
		AttackTagAbilitySystem.Get())
	{
		TaggedAbilitySystem->RemoveLooseGameplayTag(
			ZorbaGameplayTags::State_Attacking);
		if (bActiveAttackGrantedInvulnerability)
		{
			TaggedAbilitySystem->RemoveLooseGameplayTag(
				ZorbaGameplayTags::State_Invulnerable);
		}
	}
	AttackTagAbilitySystem.Reset();
	bActiveAttackGrantedInvulnerability = false;

	UnbindAnimInstance();
	bAttackInProgress = false;
	bHitCommitted = false;
	bHitLanded = false;
	bHeavyBranchWindowOpen = false;
	ActiveAttackDefinition = nullptr;
	ActiveMontage.Reset();
	ActiveLockedTarget.Reset();
	LockedTargetAnimInstance.Reset();
	ActiveTargetMontage.Reset();
	ActiveMontageInstanceId = INDEX_NONE;
	TraceBaseComponent.Reset();
	TraceTipComponent.Reset();
	WeaponContacts.Reset();
	ActorsHitThisAttack.Reset();
}

void UZorbaMeleeCombatComponent::BindAnimInstance(
	UAnimInstance* AnimInstance)
{
	UnbindAnimInstance();

	if (!AnimInstance)
	{
		return;
	}

	BoundAnimInstance = AnimInstance;
	AnimInstance->OnPlayMontageNotifyBegin.AddDynamic(
		this,
		&UZorbaMeleeCombatComponent::HandleMontageNotifyBegin);
	AnimInstance->OnPlayMontageNotifyEnd.AddDynamic(
		this,
		&UZorbaMeleeCombatComponent::HandleMontageNotifyEnd);
}

void UZorbaMeleeCombatComponent::UnbindAnimInstance()
{
	if (UAnimInstance* AnimInstance = BoundAnimInstance.Get())
	{
		AnimInstance->OnPlayMontageNotifyBegin.RemoveDynamic(
			this,
			&UZorbaMeleeCombatComponent::HandleMontageNotifyBegin);
		AnimInstance->OnPlayMontageNotifyEnd.RemoveDynamic(
			this,
			&UZorbaMeleeCombatComponent::HandleMontageNotifyEnd);
	}

	BoundAnimInstance.Reset();
}

void UZorbaMeleeCombatComponent::ResolveTraceComponents()
{
	TraceBaseComponent = FindSceneComponentByName(TraceBaseComponentName);
	TraceTipComponent = FindSceneComponentByName(TraceTipComponentName);

	if ((!TraceBaseComponent.IsValid() || !TraceTipComponent.IsValid())
		&& bDrawAttackDirection)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Weapon trace markers were not found. Broad attack coverage remains active."));
	}
}

USceneComponent* UZorbaMeleeCombatComponent::FindSceneComponentByName(
	FName ComponentName) const
{
	const AActor* OwnerActor = GetOwner();
	if (!OwnerActor || ComponentName.IsNone())
	{
		return nullptr;
	}

	TInlineComponentArray<USceneComponent*> SceneComponents;
	OwnerActor->GetComponents(SceneComponents);
	const FString ComponentPrefix = ComponentName.ToString();

	for (USceneComponent* SceneComponent : SceneComponents)
	{
		if (SceneComponent
			&& (SceneComponent->GetFName() == ComponentName
				|| SceneComponent->ComponentHasTag(ComponentName)
				|| SceneComponent->GetName().StartsWith(ComponentPrefix)))
		{
			return SceneComponent;
		}
	}

	return nullptr;
}

void UZorbaMeleeCombatComponent::SampleWeaponTrace()
{
	USceneComponent* TraceBase = TraceBaseComponent.Get();
	USceneComponent* TraceTip = TraceTipComponent.Get();
	if (!TraceBase || !TraceTip)
	{
		return;
	}

	const FVector CurrentTraceBase = TraceBase->GetComponentLocation();
	const FVector CurrentTraceTip = TraceTip->GetComponentLocation();

	if (bHasPreviousTraceSample)
	{
		SweepTraceSegment(PreviousTraceBase, CurrentTraceBase);
		SweepTraceSegment(PreviousTraceTip, CurrentTraceTip);
		SweepTraceSegment(CurrentTraceBase, CurrentTraceTip);
	}

	PreviousTraceBase = CurrentTraceBase;
	PreviousTraceTip = CurrentTraceTip;
	bHasPreviousTraceSample = true;
}

void UZorbaMeleeCombatComponent::SweepTraceSegment(
	const FVector& Start,
	const FVector& End)
{
	UWorld* World = GetWorld();
	if (!World || WeaponTraceRadius <= 0.0f)
	{
		return;
	}

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(ZorbaWeaponTrace),
		false,
		GetOwner());

	TArray<FHitResult> HitResults;
	World->SweepMultiByObjectType(
		HitResults,
		Start,
		End,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(WeaponTraceRadius),
		QueryParams);

	for (const FHitResult& HitResult : HitResults)
	{
		RememberWeaponContact(HitResult);
	}

	if (bDrawAttackDirection)
	{
		DrawDebugLine(
			World,
			Start,
			End,
			FColor::Green,
			false,
			0.15f,
			0,
			1.0f);
	}
}

void UZorbaMeleeCombatComponent::RememberWeaponContact(
	const FHitResult& HitResult)
{
	AActor* HitActor = HitResult.GetActor();
	if (!IsValid(HitActor) || HitActor == GetOwner())
	{
		return;
	}

	const TWeakObjectPtr<AActor> ActorKey(HitActor);
	// Keep the most recent contact so HitCommit, impact VFX, and sound use the
	// visible blade position instead of the first point touched by the window.
	WeaponContacts.Add(ActorKey, HitResult);
}

void UZorbaMeleeCombatComponent::GatherEligibleTargets(
	const UZorbaAttackDefinition* AttackDefinition,
	const FZorbaAttackHitPhase& HitPhase,
	const FVector& AttackDirection,
	bool bRespectAlreadyHit,
	TArray<AActor*>& OutTargets) const
{
	OutTargets.Reset();

	UWorld* World = GetWorld();
	const AActor* OwnerActor = GetOwner();
	if (!World || !OwnerActor)
	{
		return;
	}

	const FVector PhaseDirection =
		AttackDirection.RotateAngleAxis(
			HitPhase.LocalYawOffsetDegrees,
			FVector::UpVector).GetSafeNormal2D();
	const FQuat AttackRotation =
		FRotationMatrix::MakeFromXZ(
			AttackDirection,
			FVector::UpVector).ToQuat();
	const FVector PhaseOrigin =
		OwnerActor->GetActorLocation()
		+ AttackRotation.RotateVector(HitPhase.LocalOffset);

	const float HorizontalSearchRadius = FMath::Max3(
		HitPhase.Range,
		HitPhase.Radius,
		static_cast<float>(HitPhase.BoxHalfExtent.Size()));
	const float SearchRadius = FMath::Sqrt(
		FMath::Square(HorizontalSearchRadius)
		+ FMath::Square(FMath::Max(0.0f, HitPhase.HalfHeight)));

	FCollisionObjectQueryParams ObjectQueryParams;
	ObjectQueryParams.AddObjectTypesToQuery(ECC_Pawn);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_WorldDynamic);
	ObjectQueryParams.AddObjectTypesToQuery(ECC_PhysicsBody);

	FCollisionQueryParams QueryParams(
		SCENE_QUERY_STAT(ZorbaAttackCandidates),
		false,
		OwnerActor);

	TArray<FOverlapResult> OverlapResults;
	World->OverlapMultiByObjectType(
		OverlapResults,
		PhaseOrigin,
		FQuat::Identity,
		ObjectQueryParams,
		FCollisionShape::MakeSphere(FMath::Max(SearchRadius, 1.0f)),
		QueryParams);

	TSet<TWeakObjectPtr<AActor>> UniqueActors;
	for (const FOverlapResult& OverlapResult : OverlapResults)
	{
		AActor* TargetActor = OverlapResult.GetActor();
		const TWeakObjectPtr<AActor> ActorKey(TargetActor);
		if (bRespectAlreadyHit
			&& ActiveLockedTarget.IsValid()
			&& TargetActor != ActiveLockedTarget.Get())
		{
			continue;
		}

		if (UniqueActors.Contains(ActorKey))
		{
			continue;
		}
		UniqueActors.Add(ActorKey);

		if (IsEligibleTarget(
				TargetActor,
				AttackDefinition,
				HitPhase,
				PhaseOrigin,
				PhaseDirection,
				bRespectAlreadyHit))
		{
			OutTargets.Add(TargetActor);
		}
	}

	OutTargets.Sort(
		[PhaseOrigin](const AActor& Left, const AActor& Right)
		{
			return FVector::DistSquared(
				PhaseOrigin,
				Left.GetActorLocation())
				< FVector::DistSquared(
					PhaseOrigin,
					Right.GetActorLocation());
		});
}

bool UZorbaMeleeCombatComponent::IsEligibleTarget(
	AActor* TargetActor,
	const UZorbaAttackDefinition* AttackDefinition,
	const FZorbaAttackHitPhase& HitPhase,
	const FVector& PhaseOrigin,
	const FVector& PhaseDirection,
	bool bRespectAlreadyHit) const
{
	const AActor* OwnerActor = GetOwner();
	if (!IsValid(TargetActor)
		|| TargetActor == OwnerActor
		|| (bRespectAlreadyHit
			&& ActorsHitThisAttack.Contains(
				TWeakObjectPtr<AActor>(TargetActor))))
	{
		return false;
	}

	const IGenericTeamAgentInterface* SourceTeamAgent =
		Cast<IGenericTeamAgentInterface>(OwnerActor);
	const IGenericTeamAgentInterface* TargetTeamAgent =
		Cast<IGenericTeamAgentInterface>(TargetActor);

	if (!SourceTeamAgent || !TargetTeamAgent)
	{
		return false;
	}

	const FGenericTeamId SourceTeamId =
		SourceTeamAgent->GetGenericTeamId();
	const FGenericTeamId TargetTeamId =
		TargetTeamAgent->GetGenericTeamId();

	if (SourceTeamId.GetId() == FGenericTeamId::NoTeam.GetId()
		|| TargetTeamId.GetId() == FGenericTeamId::NoTeam.GetId()
		|| SourceTeamId.GetId() == TargetTeamId.GetId())
	{
		return false;
	}

	UAbilitySystemComponent* TargetAbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);
	if (!TargetAbilitySystem
		|| !TargetAbilitySystem->HasAttributeSetForAttribute(
			UZorbaCombatAttributeSet::GetHealthAttribute())
		|| TargetAbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead)
		|| TargetAbilitySystem->GetNumericAttribute(
			UZorbaCombatAttributeSet::GetHealthAttribute()) <= 0.0f)
	{
		return false;
	}
	if (TargetAbilitySystem->HasMatchingGameplayTag(
		ZorbaGameplayTags::State_Invulnerable))
	{
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Melee target rejected while invulnerable: Target=%s Attack=%s"),
			*GetNameSafe(TargetActor),
			AttackDefinition
				? *AttackDefinition->AttackId.ToString()
				: TEXT("None"));
		return false;
	}

	if (AttackDefinition)
	{
		if (AttackDefinition->AttackKind == EZorbaAttackKind::Opportunity
			&& !TargetAbilitySystem->HasMatchingGameplayTag(
				ZorbaGameplayTags::State_Exhausted))
		{
			return false;
		}

		if (AttackDefinition->AttackKind == EZorbaAttackKind::Execution)
		{
			const float Health = TargetAbilitySystem->GetNumericAttribute(
				UZorbaCombatAttributeSet::GetHealthAttribute());
			const float MaxHealth = TargetAbilitySystem->GetNumericAttribute(
				UZorbaCombatAttributeSet::GetMaxHealthAttribute());
			const float HealthRatio =
				MaxHealth > 0.0f ? Health / MaxHealth : 1.0f;
			if (HealthRatio > FMath::Clamp(
				AttackDefinition->ExecutionHealthThresholdRatio,
				0.0f,
				1.0f))
			{
				return false;
			}
		}
	}

	FVector ToTarget = TargetActor->GetActorLocation() - PhaseOrigin;
	if (HitPhase.HalfHeight > 0.0f
		&& FMath::Abs(ToTarget.Z) > HitPhase.HalfHeight)
	{
		return false;
	}

	ToTarget.Z = 0.0f;
	const float DistanceSquared = ToTarget.SizeSquared();
	const float RangeSquared = FMath::Square(FMath::Max(0.0f, HitPhase.Range));

	switch (HitPhase.Shape)
	{
	case EZorbaAttackShape::ForwardArc:
	{
		if (DistanceSquared > RangeSquared)
		{
			return false;
		}

		const FVector DirectionToTarget = ToTarget.GetSafeNormal();
		const float MinimumDot =
			FMath::Cos(FMath::DegreesToRadians(HitPhase.HalfAngleDegrees));
		return DirectionToTarget.IsNearlyZero()
			|| FVector::DotProduct(PhaseDirection, DirectionToTarget)
				>= MinimumDot;
	}

	case EZorbaAttackShape::Radial:
		return DistanceSquared <= RangeSquared;

	case EZorbaAttackShape::SphereSweep:
	case EZorbaAttackShape::CapsuleSweep:
	{
		const FVector SegmentEnd =
			PhaseOrigin + PhaseDirection * HitPhase.Range;
		return FMath::PointDistToSegment(
			TargetActor->GetActorLocation(),
			PhaseOrigin,
			SegmentEnd) <= HitPhase.Radius;
	}

	case EZorbaAttackShape::BoxSweep:
	{
		const FQuat PhaseRotation =
			FRotationMatrix::MakeFromXZ(
				PhaseDirection,
				FVector::UpVector).ToQuat();
		const FVector LocalTarget = PhaseRotation.UnrotateVector(
			TargetActor->GetActorLocation() - PhaseOrigin);
		return FMath::Abs(LocalTarget.X) <= HitPhase.BoxHalfExtent.X
			&& FMath::Abs(LocalTarget.Y) <= HitPhase.BoxHalfExtent.Y
			&& FMath::Abs(LocalTarget.Z) <= HitPhase.BoxHalfExtent.Z;
	}

	default:
		return false;
	}
}

FHitResult UZorbaMeleeCombatComponent::ResolveHitResult(
	AActor* TargetActor,
	const FVector& PhaseOrigin) const
{
	const TWeakObjectPtr<AActor> ActorKey(TargetActor);
	if (const FHitResult* WeaponHit = WeaponContacts.Find(ActorKey))
	{
		return *WeaponHit;
	}

	UPrimitiveComponent* TargetPrimitive =
		TargetActor ? TargetActor->FindComponentByClass<UPrimitiveComponent>() : nullptr;
	FVector ImpactPoint =
		TargetActor ? TargetActor->GetActorLocation() : PhaseOrigin;

	if (TargetPrimitive)
	{
		FVector ClosestPoint = FVector::ZeroVector;
		if (TargetPrimitive->GetClosestPointOnCollision(
			PhaseOrigin,
			ClosestPoint) >= 0.0f)
		{
			ImpactPoint = ClosestPoint;
		}
	}

	const FVector ImpactNormal =
		(PhaseOrigin - ImpactPoint).GetSafeNormal();
	FHitResult FallbackHit(
		TargetActor,
		TargetPrimitive,
		ImpactPoint,
		ImpactNormal);
	FallbackHit.TraceStart = PhaseOrigin;
	FallbackHit.TraceEnd = ImpactPoint;
	return FallbackHit;
}

bool UZorbaMeleeCombatComponent::ApplyDamageToTarget(
	AActor* TargetActor,
	const FZorbaAttackHitPhase& HitPhase,
	const FHitResult& HitResult)
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor || !OwnerActor->HasAuthority())
	{
		return false;
	}

	UAbilitySystemComponent* SourceAbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(OwnerActor);
	UAbilitySystemComponent* TargetAbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(TargetActor);

	if (!SourceAbilitySystem
		|| !TargetAbilitySystem
		|| TargetAbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Invulnerable)
		|| ActorsHitThisAttack.Contains(TWeakObjectPtr<AActor>(TargetActor)))
	{
		return false;
	}

	const float DamageMultiplier =
		FMath::Max(0.0f, HitPhase.DamageMultiplier.GetValueAtLevel(1.0f));
	float HealthDamage =
		FMath::Max(
			0.0f,
			ActiveAttackDefinition->HealthDamage.GetValueAtLevel(1.0f)
				* DamageMultiplier);
	float StaminaDamage =
		FMath::Max(
			0.0f,
			ActiveAttackDefinition->StaminaDamage.GetValueAtLevel(1.0f)
				* DamageMultiplier);
	if (ActiveAttackDefinition->AttackKind == EZorbaAttackKind::Execution)
	{
		HealthDamage = TargetAbilitySystem->GetNumericAttribute(
			UZorbaCombatAttributeSet::GetHealthAttribute());
	}
	const bool bSacredDoctrineRearAttack =
		ActiveAttackDefinition->AttackKind != EZorbaAttackKind::Execution
		&& IsSacredDoctrineRearAttack(OwnerActor, TargetActor);
	if (bSacredDoctrineRearAttack)
	{
		HealthDamage *= FMath::Max(1.0f, RearAttackHealthDamageMultiplier);
	}

	EZorbaMeleeDefenseResult DefenseResult =
		EZorbaMeleeDefenseResult::None;
	float ParryStaminaDamage = 0.0f;
	AZorbaCharacter* PlayerCharacter = Cast<AZorbaCharacter>(TargetActor);
	AZorbaEnemyCharacter* EnemyCharacter =
		Cast<AZorbaEnemyCharacter>(TargetActor);
	if (PlayerCharacter)
	{
		DefenseResult = PlayerCharacter->ResolveIncomingMeleeHit(
			OwnerActor,
			HitResult,
			ActiveAttackDefinition,
			HealthDamage,
			StaminaDamage,
			ParryStaminaDamage);
	}
	else if (EnemyCharacter && !bSacredDoctrineRearAttack)
	{
		DefenseResult = EnemyCharacter->ResolveIncomingMeleeHit(
			OwnerActor,
			ActiveAttackDefinition,
			HealthDamage,
			StaminaDamage);
	}

	if (DefenseResult == EZorbaMeleeDefenseResult::Parried)
	{
		ActorsHitThisAttack.Add(TWeakObjectPtr<AActor>(TargetActor));

		if (AZorbaEnemyCharacter* EnemyAttacker =
			Cast<AZorbaEnemyCharacter>(OwnerActor))
		{
			EnemyAttacker->HandleParried(
				TargetActor,
				HitResult,
				ParryStaminaDamage);
		}

		PlayerCharacter->HandleMeleeHit(
			OwnerActor,
			HitResult,
			DefenseResult);

		UE_LOG(
			LogTemp,
			Log,
			TEXT("Melee attack parried by %s. AttackerStaminaDamage=%.1f"),
			*GetNameSafe(TargetActor),
			ParryStaminaDamage);
		return true;
	}

	TSubclassOf<UGameplayEffect> DamageEffectClass =
		ActiveAttackDefinition->DamageEffect;
	if (!DamageEffectClass)
	{
		DamageEffectClass = UZorbaMeleeDamageEffect::StaticClass();
	}

	FGameplayEffectContextHandle EffectContext =
		SourceAbilitySystem->MakeEffectContext();
	EffectContext.AddSourceObject(ActiveAttackDefinition);
	EffectContext.AddHitResult(HitResult, true);

	FGameplayEffectSpecHandle EffectSpecHandle =
		SourceAbilitySystem->MakeOutgoingSpec(
			DamageEffectClass,
			1.0f,
			EffectContext);

	if (!EffectSpecHandle.IsValid())
	{
		return false;
	}

	EffectSpecHandle.Data->SetSetByCallerMagnitude(
		UZorbaMeleeDamageEffect::HealthDamageDataName,
		-HealthDamage);
	EffectSpecHandle.Data->SetSetByCallerMagnitude(
		UZorbaMeleeDamageEffect::StaminaDamageDataName,
		-StaminaDamage);

	const float HealthBefore = TargetAbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetHealthAttribute());
	const float StaminaBefore = TargetAbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute());

	const FActiveGameplayEffectHandle AppliedEffectHandle =
		SourceAbilitySystem->ApplyGameplayEffectSpecToTarget(
		*EffectSpecHandle.Data.Get(),
		TargetAbilitySystem);

	if (!AppliedEffectHandle.WasSuccessfullyApplied())
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("Melee damage rejected by GameplayEffect rules for %s."),
			*GetNameSafe(TargetActor));
		return false;
	}

	ActorsHitThisAttack.Add(TWeakObjectPtr<AActor>(TargetActor));

	const float HealthAfter = TargetAbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetHealthAttribute());
	const float StaminaAfter = TargetAbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute());

	if (EnemyCharacter)
	{
		EnemyCharacter->HandleMeleeHit(OwnerActor, HitResult);
		if (ActiveAttackDefinition->AttackKind
			== EZorbaAttackKind::Opportunity
			&& !EnemyCharacter->IsDead())
		{
			EnemyCharacter->ConsumeExhausted();
		}
		else if (ActiveAttackDefinition->AttackKind
			== EZorbaAttackKind::Execution)
		{
			EnemyCharacter->HandleExecuted(OwnerActor, HitResult);
			ApplySourceRecovery();
		}
	}
	else if (PlayerCharacter)
	{
		PlayerCharacter->HandleMeleeHit(
			OwnerActor,
			HitResult,
			DefenseResult);
	}

	if (bDrawAttackDirection && GetWorld())
	{
		DrawDebugPoint(
			GetWorld(),
			HitResult.ImpactPoint,
			18.0f,
			FColor::Red,
			false,
			1.25f,
			0);
		if (bSacredDoctrineRearAttack)
		{
			DrawDebugString(
				GetWorld(),
				TargetActor->GetActorLocation() + FVector(0.0f, 0.0f, 165.0f),
				FString::Printf(
					TEXT("SACRED REAR x%.2f"),
					FMath::Max(1.0f, RearAttackHealthDamageMultiplier)),
				nullptr,
				FColor::Yellow,
				1.25f,
				true,
				1.25f);
		}
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Melee damage applied once to %s: Health %.1f -> %.1f, Stamina %.1f -> %.1f, Defense=%s, SacredDoctrine=%s, Contact=%s"),
		*GetNameSafe(TargetActor),
		HealthBefore,
		HealthAfter,
		StaminaBefore,
		StaminaAfter,
		DefenseResult == EZorbaMeleeDefenseResult::Blocked
			? TEXT("Blocked")
			: DefenseResult == EZorbaMeleeDefenseResult::GuardBroken
				? TEXT("GuardBroken")
				: TEXT("None"),
		bSacredDoctrineRearAttack ? TEXT("RearAttack") : TEXT("None"),
		WeaponContacts.Contains(TWeakObjectPtr<AActor>(TargetActor))
			? TEXT("WeaponTrace")
			: TEXT("BroadCoverageFallback"));

	return true;
}

bool UZorbaMeleeCombatComponent::IsSacredDoctrineRearAttack(
	const AActor* SourceActor,
	const AActor* TargetActor) const
{
	if (!bRearAttackDoctrineEnabled
		|| !Cast<AZorbaCharacter>(SourceActor)
		|| !Cast<AZorbaEnemyCharacter>(TargetActor))
	{
		return false;
	}

	FVector TargetToSource =
		SourceActor->GetActorLocation() - TargetActor->GetActorLocation();
	TargetToSource.Z = 0.0f;
	TargetToSource = TargetToSource.GetSafeNormal();
	const FVector TargetForward =
		TargetActor->GetActorForwardVector().GetSafeNormal2D();
	if (TargetToSource.IsNearlyZero() || TargetForward.IsNearlyZero())
	{
		return false;
	}

	const float RearDotThreshold = -FMath::Cos(FMath::DegreesToRadians(
		FMath::Clamp(RearAttackHalfAngleDegrees, 0.0f, 180.0f)));
	return FVector::DotProduct(TargetForward, TargetToSource)
		<= RearDotThreshold;
}
