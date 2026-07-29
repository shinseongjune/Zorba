// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AbilitySystemInterface.h"
#include "Combat/ZorbaAttackDefinition.h"
#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"
#include "GenericTeamAgentInterface.h"
#include "TimerManager.h"
#include "ZorbaCharacter.generated.h"

class UCameraComponent;
class UAbilitySystemComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;
class UZorbaMeleeCombatComponent;

UCLASS()
class ZORBA_API AZorbaCharacter :
	public ACharacter,
	public IAbilitySystemInterface,
	public IGenericTeamAgentInterface
{
	GENERATED_BODY()

public:
	AZorbaCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual FGenericTeamId GetGenericTeamId() const override
	{
		return FGenericTeamId(0);
	}
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void Tick(float DeltaSeconds) override;

	EZorbaMeleeDefenseResult ResolveIncomingMeleeHit(
		AActor* SourceActor,
		const FHitResult& HitResult,
		const UZorbaAttackDefinition* IncomingAttack,
		float& InOutHealthDamage,
		float& InOutStaminaDamage,
		float& OutParryStaminaDamage);

	void HandleMeleeHit(
		AActor* SourceActor,
		const FHitResult& HitResult,
		EZorbaMeleeDefenseResult DefenseResult);
	void ApplyFodderParryExecutionBenefits();

	UFUNCTION(BlueprintPure, Category = "Zorba|Combat|Defense")
	bool IsDefending() const { return bIsDefending; }

#if !UE_BUILD_SHIPPING
	void ConfigureDefenseForAutomation(bool bKeepParryWindowOpen);
#endif

protected:
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaMeleeCombatComponent> MeleeCombatComponent;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaAttackDefinition> PrimaryAttackDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaAttackDefinition> HeavyAttackDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaAttackDefinition> DerivedHeavyAttackDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaAttackDefinition> OpportunityAttackDefinition;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat")
	TObjectPtr<UZorbaAttackDefinition> ExecutionAttackDefinition;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Debug")
	TObjectPtr<UStaticMeshComponent> DebugBodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Debug")
	TObjectPtr<UStaticMeshComponent> DebugFacingMesh;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	int32 DefaultMappingPriority = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> LookRateAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> PrimaryAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> HeavyAttackAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> DefendAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> DodgeAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> DarkFormAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> AbilityLayerAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> ContextActionInput;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> ClassActionInput;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> UseAbilitySlot1Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> UseAbilitySlot2Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> UseAbilitySlot3Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> UseAbilitySlot4Action;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> UseRelicAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> ShowObjectiveAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Input")
	TObjectPtr<UInputAction> PauseAction;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Movement")
	float WalkSpeed = 500.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Movement")
	float SprintSpeed = 800.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Movement")
	float DodgeStrength = 650.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Movement")
	float DarkFormSpeed = 1400.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Movement")
	float DarkFormDuration = 0.8f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Movement")
	float DarkFormCooldown = 4.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Defense", meta = (ClampMin = "0.0", Units = "s"))
	float ParryWindowDuration = 0.22f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Defense", meta = (ClampMin = "0.0", ClampMax = "180.0", Units = "Degrees"))
	float DefenseHalfAngleDegrees = 75.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Defense", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BlockedHealthDamageMultiplier = 0.2f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Defense", meta = (ClampMin = "0.0"))
	float BlockedStaminaDamageMultiplier = 1.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Defense", meta = (ClampMin = "0.0"))
	float ParryStaminaDamage = 35.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Stamina", meta = (ClampMin = "0.0", Units = "s"))
	float CombatStaminaRecoveryDelay = 1.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Combat|Stamina", meta = (ClampMin = "0.0"))
	float CombatStaminaRecoveryPerSecond = 30.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Camera")
	float GamepadTurnRate = 140.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Camera")
	float GamepadLookUpRate = 100.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnOpportunityAttackStarted(AActor* TargetActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnExecutionStarted(AActor* TargetActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnDefendStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnDefendStopped();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnGuardBroken(AActor* SourceActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnMeleeHitReceived(
		AActor* SourceActor,
		const FHitResult& HitResult,
		EZorbaMeleeDefenseResult DefenseResult,
		float RemainingHealth,
		float RemainingStamina);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnDodgeRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnDarkFormStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnDarkFormEnded();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnDarkFormDenied();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnDarkFormPassedThroughActor(AActor* OtherActor);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnContextActionRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnClassActionRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnAbilitySlotRequested(int32 SlotIndex);

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnRelicRequested();

private:
	void InitializeAbilitySystem();
	void AddDefaultMappingContext();
	void BindEnhancedInput(UInputComponent* PlayerInputComponent);

	void Move(const FInputActionValue& Value);
	void StopMoveInput();
	void Look(const FInputActionValue& Value);
	void LookAtRate(const FInputActionValue& Value);
	void MoveForward(float Value);
	void MoveRight(float Value);
	void Turn(float Value);
	void LookUp(float Value);
	void TurnAtRate(float Value);
	void LookUpAtRate(float Value);
	void FaceCameraYaw();
	FVector ResolveAttackDirection() const;

	void StartSprintInput();
	void StopSprintInput();
	void RefreshSprintState();
	void SetSprinting(bool bNewIsSprinting);
	void ApplyCurrentMovementSpeed();
	void StartAbilityLayer();
	void StopAbilityLayer();
	void RequestPrimaryAttack();
	void RequestHeavyAttack();
	bool TryStartOpportunityAttack(const FVector& AttackDirection);
	bool TryStartExecution(const FVector& AttackDirection);
	bool StartAttackDefinition(
		UZorbaAttackDefinition* AttackDefinition,
		const FVector& AttackDirection,
		AActor* LockedTarget = nullptr,
		bool bTransitionFromActiveAttack = false);
	void StopMovementForAttack();
	void RecoverCombatStamina(float DeltaSeconds);
#if !UE_BUILD_SHIPPING
	void ConfigureAdvancedCombatAutomation();
#endif
	void StartDefend();
	void StopDefend();
	void CloseParryWindow();
	void RequestDodge();
	void RequestDarkForm();
	void FinishDarkForm();
	void RequestContextAction();
	void RequestClassAction();
	void RequestAbilitySlot1();
	void RequestAbilitySlot2();
	void RequestAbilitySlot3();
	void RequestAbilitySlot4();
	void RequestAbilitySlot(int32 SlotIndex);
	bool TryRouteAbilityLayerFaceButton(int32 SlotIndex);
	void RequestRelic();
	void ShowObjective();
	void RequestPause();

	bool bIsSprinting = false;
	bool bHasMoveInput = false;
	bool bSprintInputHeld = false;
	bool bSprintToggledOn = false;
	bool bAbilityLayerHeld = false;
	bool bIsInDarkForm = false;
	bool bIsDefending = false;
	float LastDarkFormTime = -1000.0f;
	float CombatStaminaRecoveryDelayRemaining = 0.0f;
	FTimerHandle DarkFormTimerHandle;
	FTimerHandle ParryWindowTimerHandle;
	ECollisionResponse DefaultPawnCollisionResponse = ECR_Block;
};
