// Copyright Epic Games, Inc. All Rights Reserved.

#include "Enemy/ZorbaEnemyCombatBrainComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Combat/ZorbaCombatAttributeSet.h"
#include "Combat/ZorbaGameplayTags.h"
#include "Combat/ZorbaMeleeCombatComponent.h"
#include "DrawDebugHelpers.h"
#include "Enemy/ZorbaEnemyBasicAttackComponent.h"
#include "Enemy/ZorbaEnemyCharacter.h"
#include "Enemy/ZorbaEnemyCrowdSubsystem.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#if !UE_BUILD_SHIPPING
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#endif

UZorbaEnemyCombatBrainComponent::UZorbaEnemyCombatBrainComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	// AddMovementInput is consumed per frame, so movement intent must also be
	// refreshed per frame. Decision timing is still throttled by the profile
	// timers instead of the component tick interval.
	PrimaryComponentTick.TickInterval = 0.0f;
}

void UZorbaEnemyCombatBrainComponent::BeginPlay()
{
	Super::BeginPlay();

	EnemyOwner = Cast<AZorbaEnemyCharacter>(GetOwner());
	if (!EnemyOwner.IsValid())
	{
		SetComponentTickEnabled(false);
		return;
	}

	HomeLocation = EnemyOwner->GetActorLocation();
	AttackExecutor = EnemyOwner->GetBasicAttackComponent();
	MeleeCombatComponent =
		EnemyOwner->FindComponentByClass<UZorbaMeleeCombatComponent>();
	if (UCharacterMovementComponent* Movement =
		EnemyOwner->GetCharacterMovement())
	{
		BaseWalkSpeed = Movement->MaxWalkSpeed;
		bOriginalOrientRotationToMovement =
			Movement->bOrientRotationToMovement;
		bMovementFacingModeCaptured = true;
	}

	RefreshCombatProfile();
}

void UZorbaEnemyCombatBrainComponent::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ReleaseCombatTokens();
	SetBrainOwnsFacing(false);
	Super::EndPlay(EndPlayReason);
}

void UZorbaEnemyCombatBrainComponent::RefreshCombatProfile()
{
	ActiveProfile = EnemyOwner.IsValid()
		? EnemyOwner->GetCombatProfile()
		: nullptr;

	if (!IsValid(ActiveProfile) || !AttackExecutor.IsValid())
	{
		if (AttackExecutor.IsValid())
		{
			AttackExecutor->SetExternallyDriven(false);
		}
		SetBrainOwnsFacing(false);
		SetState(EZorbaEnemyBrainState::Disabled);
		return;
	}

	AttackExecutor->SetExternallyDriven(true);
	SetBrainOwnsFacing(true);
	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	NextDecisionTime = CurrentTime
		+ FMath::Max(0.0f, ActiveProfile->InitialAttackDelay);
	SetState(EZorbaEnemyBrainState::Idle);
}

void UZorbaEnemyCombatBrainComponent::StopBrain()
{
	ReleaseCombatTokens();
	SetBrainOwnsFacing(false);
	SetState(EZorbaEnemyBrainState::Disabled);
	SetComponentTickEnabled(false);
	if (EnemyOwner.IsValid())
	{
		EnemyOwner->GetCharacterMovement()->StopMovementImmediately();
	}
}

void UZorbaEnemyCombatBrainComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UWorld* World = GetWorld();
	if (!World
		|| !EnemyOwner.IsValid()
		|| !EnemyOwner->HasAuthority()
		|| EnemyOwner->IsDead()
		|| !IsValid(ActiveProfile))
	{
		if (EnemyOwner.IsValid() && EnemyOwner->IsDead())
		{
			StopBrain();
		}
		return;
	}

	AActor* Player = UGameplayStatics::GetPlayerCharacter(this, 0);
	if (!CanEngageTarget(Player))
	{
		TargetActor.Reset();
		ReleaseCombatTokens();
		SetState(EZorbaEnemyBrainState::Idle);
		EnemyOwner->GetCharacterMovement()->StopMovementImmediately();
		return;
	}
	TargetActor = Player;
#if !UE_BUILD_SHIPPING
	ApplyAutomationSetup(Player);
#endif

	const float CurrentTime = World->GetTimeSeconds();
	const UAbilitySystemComponent* OwnerAbilitySystem =
		EnemyOwner->GetAbilitySystemComponent();
	if (EnemyOwner->IsExhausted()
		|| EnemyOwner->IsStunned()
		|| (OwnerAbilitySystem
			&& OwnerAbilitySystem->HasMatchingGameplayTag(
				ZorbaGameplayTags::State_HitReact)))
	{
		ReleaseCombatTokens();
		EnemyOwner->GetCharacterMovement()->StopMovementImmediately();
		StateEndTime = CurrentTime + 0.15f;
		SetState(EZorbaEnemyBrainState::Recover);
		return;
	}
	if (EnemyOwner->IsDefending())
	{
		SetState(EZorbaEnemyBrainState::Defend);
		EnemyOwner->GetCharacterMovement()->StopMovementImmediately();
		FaceTarget();
		return;
	}
	if (BrainState == EZorbaEnemyBrainState::Defend)
	{
		BeginObserve(CurrentTime);
	}

	switch (BrainState)
	{
	case EZorbaEnemyBrainState::Idle:
		BeginObserve(CurrentTime);
		break;
	case EZorbaEnemyBrainState::Observe:
		UpdateObserveMovement(CurrentTime);
		if (CurrentTime >= StateEndTime && CurrentTime >= NextDecisionTime)
		{
			UpdateDecision(CurrentTime);
		}
		break;
	case EZorbaEnemyBrainState::Approach:
		UpdateApproach(CurrentTime);
		break;
	case EZorbaEnemyBrainState::Attack:
		UpdateAttack(CurrentTime);
		break;
	case EZorbaEnemyBrainState::Recover:
		FaceTarget();
		if (CurrentTime >= StateEndTime)
		{
			BeginObserve(CurrentTime);
		}
		break;
	default:
		break;
	}
#if !UE_BUILD_SHIPPING
	if (bMovementAutomation
		&& !bMovementAutomationReported
		&& CurrentTime >= MovementAutomationStartTime + 6.0f)
	{
		const FVector CurrentLocation = EnemyOwner->GetActorLocation();
		const FVector StartRadial =
			(MovementAutomationStartLocation - MovementAutomationTargetLocation)
			.GetSafeNormal2D();
		const FVector CurrentRadial =
			(CurrentLocation - MovementAutomationTargetLocation)
			.GetSafeNormal2D();
		const float AngularChange = FMath::RadiansToDegrees(FMath::Acos(
			FMath::Clamp(
				FVector::DotProduct(StartRadial, CurrentRadial),
				-1.0f,
				1.0f)));
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Enemy observe movement automation result: Enemy=%s StartDistance=%.1f EndDistance=%.1f Displacement=%.1f AngularChange=%.1f"),
			*GetNameSafe(EnemyOwner.Get()),
			FVector::Dist2D(
				MovementAutomationStartLocation,
				MovementAutomationTargetLocation),
			FVector::Dist2D(CurrentLocation, MovementAutomationTargetLocation),
			FVector::Dist2D(CurrentLocation, MovementAutomationStartLocation),
			AngularChange);
		bMovementAutomationReported = true;
	}
#endif
}

bool UZorbaEnemyCombatBrainComponent::CanEngageTarget(
	AActor* Candidate) const
{
	if (!EnemyOwner.IsValid() || !IsValid(Candidate) || !IsValid(ActiveProfile))
	{
		return false;
	}

	const UAbilitySystemComponent* TargetAbilitySystem =
		UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Candidate);
	if (!TargetAbilitySystem
		|| TargetAbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead))
	{
		return false;
	}

	const float TargetDistance = FVector::Dist2D(
		EnemyOwner->GetActorLocation(),
		Candidate->GetActorLocation());
	const float HomeDistance = FVector::Dist2D(
		EnemyOwner->GetActorLocation(),
		HomeLocation);
	const bool bAlreadyEngaged = TargetActor.Get() == Candidate;
	const float MaximumTargetDistance = bAlreadyEngaged
		? ActiveProfile->LeashRange
		: ActiveProfile->EngagementRange;
	return TargetDistance <= FMath::Max(0.0f, MaximumTargetDistance)
		&& HomeDistance <= FMath::Max(0.0f, ActiveProfile->LeashRange);
}

void UZorbaEnemyCombatBrainComponent::UpdateDecision(float CurrentTime)
{
	const float ActionDelayMultiplier = EnemyOwner->IsEnraged()
		? FMath::Clamp(ActiveProfile->EnragedActionDelayMultiplier, 0.1f, 1.0f)
		: 1.0f;
	NextDecisionTime = CurrentTime
		+ FMath::Max(
			0.05f,
			ActiveProfile->DecisionInterval * ActionDelayMultiplier);
#if !UE_BUILD_SHIPPING
	if (bMovementAutomation)
	{
		BeginObserve(CurrentTime);
		return;
	}
#endif
	if (TryBeginDefense(CurrentTime) || TrySelectAttack(CurrentTime))
	{
		return;
	}
	BeginObserve(CurrentTime);
}

void UZorbaEnemyCombatBrainComponent::BeginObserve(float CurrentTime)
{
	ReleaseCombatTokens();
	SelectedAttackIndex = INDEX_NONE;
	const float WatchWeight = FMath::Max(0.0f, ActiveProfile->WatchWeight);
	const float StrafeWeight = FMath::Max(0.0f, ActiveProfile->StrafeWeight);
	const float SpecialWeight =
		FMath::Max(0.0f, ActiveProfile->SpecialMotionWeight);
	const float TotalWeight = WatchWeight + StrafeWeight + SpecialWeight;
	const float Roll = TotalWeight > 0.0f
		? FMath::FRandRange(0.0f, TotalWeight)
		: 0.0f;
	ObserveAction = Roll < WatchWeight
		? EZorbaEnemyObserveAction::Watch
		: Roll < WatchWeight + StrafeWeight
			? EZorbaEnemyObserveAction::Strafe
			: EZorbaEnemyObserveAction::SpecialMotion;
#if !UE_BUILD_SHIPPING
	if (bMovementAutomation)
	{
		ObserveAction = EZorbaEnemyObserveAction::Strafe;
	}
#endif
	ObserveStrafeDirection =
#if !UE_BUILD_SHIPPING
		bMovementAutomation ? 1.0f :
#endif
		FMath::RandBool() ? 1.0f : -1.0f;
	const float MinimumDuration =
		FMath::Max(
			0.05f,
			ActiveProfile->MinimumObserveDuration
				* (EnemyOwner->IsEnraged()
					? FMath::Clamp(
						ActiveProfile->EnragedActionDelayMultiplier,
						0.1f,
						1.0f)
					: 1.0f));
	const float MaximumDuration = FMath::Max(
		MinimumDuration,
		ActiveProfile->MaximumObserveDuration
			* (EnemyOwner->IsEnraged()
				? FMath::Clamp(
					ActiveProfile->EnragedActionDelayMultiplier,
					0.1f,
					1.0f)
				: 1.0f));
	StateEndTime = CurrentTime
		+ FMath::FRandRange(MinimumDuration, MaximumDuration);
	SetState(EZorbaEnemyBrainState::Observe);
	OnObserveActionStarted(ObserveAction);
}

void UZorbaEnemyCombatBrainComponent::UpdateObserveMovement(float CurrentTime)
{
	if (!EnemyOwner.IsValid() || !TargetActor.IsValid())
	{
		return;
	}

	const FVector OwnerLocation = EnemyOwner->GetActorLocation();
	const FVector TargetLocation = TargetActor->GetActorLocation();
	FVector Radial =
		(OwnerLocation - TargetLocation).GetSafeNormal2D();
	if (Radial.IsNearlyZero())
	{
		Radial = -TargetActor->GetActorForwardVector().GetSafeNormal2D();
	}
	if (Radial.IsNearlyZero())
	{
		Radial = FVector::ForwardVector;
	}

	const float Distance = FVector::Dist2D(OwnerLocation, TargetLocation);
	const float PreferredDistance =
		FMath::Max(0.0f, ActiveProfile->PreferredObserveDistance);
	const float DistanceTolerance =
		FMath::Max(0.0f, ActiveProfile->ObserveDistanceTolerance);
	if (Distance > PreferredDistance + DistanceTolerance)
	{
		MoveToward(
			TargetLocation + Radial * PreferredDistance,
			ActiveProfile->ObserveSpeedMultiplier);
		FaceTarget();
		return;
	}
	if (Distance < FMath::Max(0.0f, PreferredDistance - DistanceTolerance))
	{
		MoveToward(
			TargetLocation + Radial * PreferredDistance,
			ActiveProfile->RetreatSpeedMultiplier);
		FaceTarget();
		return;
	}

	if (ObserveAction != EZorbaEnemyObserveAction::Strafe)
	{
		EnemyOwner->GetCharacterMovement()->StopMovementImmediately();
		FaceTarget();
		return;
	}

	const FVector Tangent = FVector::CrossProduct(FVector::UpVector, Radial);
	const FVector DesiredLocation = OwnerLocation
		+ Tangent * ObserveStrafeDirection
			* FMath::Max(1.0f, ActiveProfile->StrafeStepDistance)
		+ Radial * (PreferredDistance - Distance);
	MoveToward(DesiredLocation, ActiveProfile->ObserveSpeedMultiplier);
	FaceTarget();
}

bool UZorbaEnemyCombatBrainComponent::TryBeginDefense(float CurrentTime)
{
	if (EnemyOwner->IsEnraged()
		|| CurrentTime < NextDefenseTime
		|| ActiveProfile->DefenseChance <= 0.0f
		|| FMath::FRand() > ActiveProfile->DefenseChance)
	{
		return false;
	}

	NextDefenseTime = CurrentTime
		+ FMath::Max(0.0f, ActiveProfile->DefenseCooldown);
	EnemyOwner->StartDefend(ActiveProfile->DefenseDuration);
	if (EnemyOwner->IsDefending())
	{
		SetState(EZorbaEnemyBrainState::Defend);
		return true;
	}
	return false;
}

bool UZorbaEnemyCombatBrainComponent::TrySelectAttack(float CurrentTime)
{
	if (!TargetActor.IsValid() || ActiveProfile->Attacks.IsEmpty())
	{
		return false;
	}

	UZorbaEnemyCrowdSubsystem* CrowdSubsystem =
		GetWorld()->GetSubsystem<UZorbaEnemyCrowdSubsystem>();
	if (!CrowdSubsystem
		|| !CrowdSubsystem->TryAcquireAttackToken(
			EnemyOwner.Get(),
			TargetActor.Get(),
			FMath::Max(1, ActiveProfile->MaxConcurrentAttackers)))
	{
		return false;
	}
	bHasAttackToken = true;

	const float Distance = FVector::Dist2D(
		EnemyOwner->GetActorLocation(),
		TargetActor->GetActorLocation());
	TArray<int32> Candidates;
	float TotalWeight = 0.0f;
#if !UE_BUILD_SHIPPING
	FString AutomationMode;
	const bool bHasAutomationMode = FParse::Value(
		FCommandLine::Get(),
		TEXT("ZorbaAITest="),
		AutomationMode);
	EZorbaAttackSignal ForcedSignal = EZorbaAttackSignal::None;
	if (bHasAutomationMode)
	{
		ForcedSignal = AutomationMode.Equals(TEXT("Ambush"), ESearchCase::IgnoreCase)
			? EZorbaAttackSignal::Ambush
			: AutomationMode.Equals(TEXT("Dodge"), ESearchCase::IgnoreCase)
				? EZorbaAttackSignal::Dodge
				: AutomationMode.Equals(TEXT("Parry"), ESearchCase::IgnoreCase)
					? EZorbaAttackSignal::Parry
					: EZorbaAttackSignal::None;
	}
#endif
	for (int32 Index = 0; Index < ActiveProfile->Attacks.Num(); ++Index)
	{
		const FZorbaEnemyAttackOption& Option = ActiveProfile->Attacks[Index];
#if !UE_BUILD_SHIPPING
		if (ForcedSignal != EZorbaAttackSignal::None
			&& IsValid(Option.AttackDefinition)
			&& Option.AttackDefinition->AttackSignal != ForcedSignal)
		{
			continue;
		}
#endif
		if (IsOptionAvailable(Option, Distance, CurrentTime))
		{
			Candidates.Add(Index);
			TotalWeight += FMath::Max(0.0f, Option.SelectionWeight);
		}
	}

	while (!Candidates.IsEmpty())
	{
		float Roll = TotalWeight > 0.0f
			? FMath::FRandRange(0.0f, TotalWeight)
			: 0.0f;
		int32 CandidateArrayIndex = 0;
		for (; CandidateArrayIndex < Candidates.Num() - 1; ++CandidateArrayIndex)
		{
			Roll -= FMath::Max(
				0.0f,
				ActiveProfile->Attacks[Candidates[CandidateArrayIndex]].SelectionWeight);
			if (Roll <= 0.0f)
			{
				break;
			}
		}

		const int32 AttackIndex = Candidates[CandidateArrayIndex];
		const FZorbaEnemyAttackOption& Option = ActiveProfile->Attacks[AttackIndex];
		const bool bNeedsSpecialToken = Option.AttackDefinition->AttackSignal
			!= EZorbaAttackSignal::None;
		if (!bNeedsSpecialToken
			|| CrowdSubsystem->TryAcquireSpecialAttackToken(
				EnemyOwner.Get(),
				TargetActor.Get()))
		{
			bHasSpecialToken = bNeedsSpecialToken;
			SelectedAttackIndex = AttackIndex;
			const float ApproachSpeed = BaseWalkSpeed
				* FMath::Max(0.1f, Option.ApproachSpeedMultiplier);
			const float EstimatedApproachTime = ApproachSpeed > 0.0f
				? Distance / ApproachSpeed
				: 5.0f;
			StateEndTime = CurrentTime
				+ FMath::Max(8.0f, EstimatedApproachTime * 4.0f);
			SetState(EZorbaEnemyBrainState::Approach);
			UE_LOG(
				LogTemp,
				Log,
				TEXT("Enemy attack selected: Enemy=%s Attack=%s Signal=%d Distance=%.1f Cooldown=%.1f"),
				*GetNameSafe(EnemyOwner.Get()),
				*Option.AttackDefinition->AttackId.ToString(),
				static_cast<int32>(Option.AttackDefinition->AttackSignal),
				Distance,
				ResolveOptionCooldown(Option));
			if (bNeedsSpecialToken)
			{
				OnAttackSignalStarted(
					Option.AttackDefinition->AttackSignal,
					Option.AttackDefinition);
#if !UE_BUILD_SHIPPING
				const FColor SignalColor =
					Option.AttackDefinition->AttackSignal == EZorbaAttackSignal::Parry
						? FColor::Cyan
						: Option.AttackDefinition->AttackSignal == EZorbaAttackSignal::Dodge
							? FColor::Orange
							: FColor::Purple;
				DrawDebugString(
					GetWorld(),
					EnemyOwner->GetActorLocation() + FVector(0.0f, 0.0f, 150.0f),
					UEnum::GetValueAsString(Option.AttackDefinition->AttackSignal),
					nullptr,
					SignalColor,
					1.5f,
					true,
					1.2f);
#endif
			}
			UpdateApproach(CurrentTime);
			return true;
		}

		TotalWeight -= FMath::Max(0.0f, Option.SelectionWeight);
		Candidates.RemoveAtSwap(CandidateArrayIndex);
	}

	ReleaseCombatTokens();
	return false;
}

bool UZorbaEnemyCombatBrainComponent::IsOptionAvailable(
	const FZorbaEnemyAttackOption& Option,
	float Distance,
	float CurrentTime) const
{
	if (!IsValid(Option.AttackDefinition)
		|| Option.SelectionWeight <= 0.0f
		|| Distance < FMath::Max(0.0f, Option.MinimumSelectionDistance)
		|| Distance > FMath::Max(
			Option.MinimumSelectionDistance,
			Option.MaximumSelectionDistance))
	{
		return false;
	}

	const float* CooldownEnd = CooldownEndTimes.Find(
		TWeakObjectPtr<UZorbaAttackDefinition>(Option.AttackDefinition));
	return !CooldownEnd || CurrentTime >= *CooldownEnd;
}

void UZorbaEnemyCombatBrainComponent::UpdateApproach(float CurrentTime)
{
	if (!ActiveProfile->Attacks.IsValidIndex(SelectedAttackIndex)
		|| !TargetActor.IsValid()
		|| CurrentTime >= StateEndTime)
	{
		ReleaseCombatTokens();
		BeginObserve(CurrentTime);
		return;
	}

	const FZorbaEnemyAttackOption& Option =
		ActiveProfile->Attacks[SelectedAttackIndex];
	if (AttackExecutor->IsTargetInAttackRange(
		TargetActor.Get(),
		Option.AttackDefinition,
		ActiveProfile->AttackRangePadding))
	{
		EnemyOwner->GetCharacterMovement()->StopMovementImmediately();
		FaceTarget();
		if (AttackExecutor->TryAttackDefinition(
			TargetActor.Get(),
			Option.AttackDefinition))
		{
			CooldownEndTimes.Add(
				TWeakObjectPtr<UZorbaAttackDefinition>(Option.AttackDefinition),
				CurrentTime + ResolveOptionCooldown(Option));
			SetState(EZorbaEnemyBrainState::Attack);
		}
		return;
	}

	FVector Away =
		(EnemyOwner->GetActorLocation() - TargetActor->GetActorLocation())
		.GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = -TargetActor->GetActorForwardVector().GetSafeNormal2D();
	}
	const float AttackRange =
		AttackExecutor->ResolveAttackRange(Option.AttackDefinition);
	const FVector DesiredLocation = TargetActor->GetActorLocation()
		+ Away * FMath::Max(80.0f, AttackRange * 0.7f);
	MoveToward(DesiredLocation, Option.ApproachSpeedMultiplier);
	FaceTarget();
}

void UZorbaEnemyCombatBrainComponent::UpdateAttack(float CurrentTime)
{
	EnemyOwner->GetCharacterMovement()->StopMovementImmediately();
	if (MeleeCombatComponent.IsValid()
		&& MeleeCombatComponent->IsAttackInProgress())
	{
		return;
	}

	ReleaseCombatTokens();
	const float ActionDelayMultiplier = EnemyOwner->IsEnraged()
		? FMath::Clamp(ActiveProfile->EnragedActionDelayMultiplier, 0.1f, 1.0f)
		: 1.0f;
	StateEndTime = CurrentTime
		+ FMath::Max(
			0.0f,
			ActiveProfile->PostAttackRecovery * ActionDelayMultiplier);
	NextDecisionTime = StateEndTime;
	SetState(EZorbaEnemyBrainState::Recover);
}

void UZorbaEnemyCombatBrainComponent::MoveToward(
	const FVector& DesiredLocation,
	float SpeedMultiplier)
{
	FVector Delta = DesiredLocation - EnemyOwner->GetActorLocation();
	Delta.Z = 0.0f;
	UCharacterMovementComponent* Movement =
		EnemyOwner->GetCharacterMovement();
	const float EnragedSpeedMultiplier = EnemyOwner->IsEnraged()
		? FMath::Max(1.0f, ActiveProfile->EnragedMovementSpeedMultiplier)
		: 1.0f;
	Movement->MaxWalkSpeed = BaseWalkSpeed
		* FMath::Max(0.1f, SpeedMultiplier)
		* EnragedSpeedMultiplier;
	if (Delta.SizeSquared() <= FMath::Square(
		FMath::Max(1.0f, ActiveProfile->MovementAcceptanceRadius)))
	{
		Movement->StopMovementImmediately();
		return;
	}
	EnemyOwner->AddMovementInput(Delta.GetSafeNormal(), 1.0f, true);
}

void UZorbaEnemyCombatBrainComponent::FaceTarget() const
{
	if (!EnemyOwner.IsValid() || !TargetActor.IsValid())
	{
		return;
	}
	const FVector Facing =
		(TargetActor->GetActorLocation() - EnemyOwner->GetActorLocation())
		.GetSafeNormal2D();
	if (!Facing.IsNearlyZero())
	{
		EnemyOwner->SetActorRotation(FRotator(0.0f, Facing.Rotation().Yaw, 0.0f));
	}
}

void UZorbaEnemyCombatBrainComponent::SetBrainOwnsFacing(bool bOwnsFacing)
{
	if (!EnemyOwner.IsValid())
	{
		return;
	}

	if (UCharacterMovementComponent* Movement =
		EnemyOwner->GetCharacterMovement())
	{
		if (!bMovementFacingModeCaptured)
		{
			bOriginalOrientRotationToMovement =
				Movement->bOrientRotationToMovement;
			bMovementFacingModeCaptured = true;
		}
		Movement->bOrientRotationToMovement = bOwnsFacing
			? false
			: bOriginalOrientRotationToMovement;
	}
}

void UZorbaEnemyCombatBrainComponent::SetState(
	EZorbaEnemyBrainState NewState)
{
	if (BrainState == NewState)
	{
		return;
	}
	const EZorbaEnemyBrainState PreviousState = BrainState;
	BrainState = NewState;
	OnBrainStateChanged(PreviousState, NewState);
	UE_LOG(
		LogTemp,
		Verbose,
		TEXT("Enemy brain state: Enemy=%s %d -> %d"),
		*GetNameSafe(GetOwner()),
		static_cast<int32>(PreviousState),
		static_cast<int32>(NewState));
}

void UZorbaEnemyCombatBrainComponent::ReleaseCombatTokens()
{
	if (UWorld* World = GetWorld())
	{
		if (UZorbaEnemyCrowdSubsystem* CrowdSubsystem =
			World->GetSubsystem<UZorbaEnemyCrowdSubsystem>())
		{
			if (bHasAttackToken)
			{
				CrowdSubsystem->ReleaseAttackToken(EnemyOwner.Get());
			}
			if (bHasSpecialToken)
			{
				CrowdSubsystem->ReleaseSpecialAttackToken(EnemyOwner.Get());
			}
		}
	}
	bHasAttackToken = false;
	bHasSpecialToken = false;
}

float UZorbaEnemyCombatBrainComponent::ResolveOptionCooldown(
	const FZorbaEnemyAttackOption& Option) const
{
	const float ActionDelayMultiplier = EnemyOwner.IsValid()
		&& EnemyOwner->IsEnraged()
		&& IsValid(ActiveProfile)
		? FMath::Clamp(
			ActiveProfile->EnragedActionDelayMultiplier,
			0.1f,
			1.0f)
		: 1.0f;
	return FMath::Max(0.0f, Option.Cooldown * ActionDelayMultiplier);
}

#if !UE_BUILD_SHIPPING
void UZorbaEnemyCombatBrainComponent::ApplyAutomationSetup(
	AActor* CurrentTarget)
{
	if (bAutomationConfigured || !IsValid(CurrentTarget) || !IsValid(ActiveProfile))
	{
		return;
	}

	FString AutomationMode;
	if (!FParse::Value(
		FCommandLine::Get(),
		TEXT("ZorbaAITest="),
		AutomationMode))
	{
		bAutomationConfigured = true;
		return;
	}

	if (AutomationMode.Equals(TEXT("Movement"), ESearchCase::IgnoreCase))
	{
		if (EnemyOwner->IsFodder())
		{
			bAutomationConfigured = true;
			StopBrain();
			return;
		}

		FVector Away =
			(EnemyOwner->GetActorLocation() - CurrentTarget->GetActorLocation())
			.GetSafeNormal2D();
		if (Away.IsNearlyZero())
		{
			Away = CurrentTarget->GetActorForwardVector().GetSafeNormal2D();
		}
		if (Away.IsNearlyZero())
		{
			Away = FVector::ForwardVector;
		}
		FVector TestLocation = CurrentTarget->GetActorLocation() + Away * 800.0f;
		TestLocation.Z = EnemyOwner->GetActorLocation().Z;
		EnemyOwner->SetActorLocation(TestLocation, false);
		HomeLocation = TestLocation;
		bMovementAutomation = true;
		MovementAutomationStartTime = GetWorld()->GetTimeSeconds();
		MovementAutomationStartLocation = TestLocation;
		MovementAutomationTargetLocation = CurrentTarget->GetActorLocation();
		bAutomationConfigured = true;
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Enemy observe movement automation prepared: Enemy=%s Distance=%.1f"),
			*GetNameSafe(EnemyOwner.Get()),
			FVector::Dist2D(TestLocation, CurrentTarget->GetActorLocation()));
		return;
	}

	if (AutomationMode.Equals(TEXT("GuardBreak"), ESearchCase::IgnoreCase))
	{
		if (!EnemyOwner->IsFodder())
		{
			const FVector Facing =
				(CurrentTarget->GetActorLocation() - EnemyOwner->GetActorLocation())
				.GetSafeNormal2D();
			if (!Facing.IsNearlyZero())
			{
				EnemyOwner->SetActorRotation(
					FRotator(0.0f, Facing.Rotation().Yaw, 0.0f));
			}
			EnemyOwner->StartDefend(3.0f);
			UE_LOG(
				LogTemp,
				Display,
				TEXT("Enemy guard-break automation prepared: Enemy=%s"),
				*GetNameSafe(EnemyOwner.Get()));
		}
		bAutomationConfigured = true;
		return;
	}

	if (!AutomationMode.Equals(TEXT("Ambush"), ESearchCase::IgnoreCase))
	{
		bAutomationConfigured = true;
		return;
	}

	const bool bHasAmbush = ActiveProfile->Attacks.ContainsByPredicate(
		[](const FZorbaEnemyAttackOption& Option)
		{
			return IsValid(Option.AttackDefinition)
				&& Option.AttackDefinition->AttackSignal
					== EZorbaAttackSignal::Ambush;
		});
	if (!bHasAmbush)
	{
		bAutomationConfigured = true;
		return;
	}

	FVector Away =
		(EnemyOwner->GetActorLocation() - CurrentTarget->GetActorLocation())
		.GetSafeNormal2D();
	if (Away.IsNearlyZero())
	{
		Away = CurrentTarget->GetActorForwardVector().GetSafeNormal2D();
	}
	if (Away.IsNearlyZero())
	{
		Away = FVector::ForwardVector;
	}
	FVector TestLocation = CurrentTarget->GetActorLocation() + Away * 800.0f;
	TestLocation.Z = EnemyOwner->GetActorLocation().Z;
	EnemyOwner->SetActorLocation(TestLocation, false);
	HomeLocation = TestLocation;
	bAutomationConfigured = true;
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Enemy AI ambush automation positioned: Enemy=%s Distance=%.1f"),
		*GetNameSafe(EnemyOwner.Get()),
		FVector::Dist2D(TestLocation, CurrentTarget->GetActorLocation()));
}
#endif
