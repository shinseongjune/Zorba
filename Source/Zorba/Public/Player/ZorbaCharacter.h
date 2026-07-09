// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "ZorbaCharacter.generated.h"

class UCameraComponent;
class UInputAction;
class UInputMappingContext;
class USpringArmComponent;
class UStaticMeshComponent;
struct FInputActionValue;

UCLASS()
class ZORBA_API AZorbaCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AZorbaCharacter();

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

protected:
	virtual void BeginPlay() override;
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Zorba|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;

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

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Camera")
	float GamepadTurnRate = 140.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Zorba|Camera")
	float GamepadLookUpRate = 100.0f;

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnPrimaryAttackRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnHeavyAttackRequested();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnDefendStarted();

	UFUNCTION(BlueprintImplementableEvent, Category = "Zorba|Combat")
	void OnDefendStopped();

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

	void StartSprintInput();
	void StopSprintInput();
	void RefreshSprintState();
	void SetSprinting(bool bNewIsSprinting);
	void ApplyCurrentMovementSpeed();
	void StartAbilityLayer();
	void StopAbilityLayer();
	void RequestPrimaryAttack();
	void RequestHeavyAttack();
	void StartDefend();
	void StopDefend();
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
	float LastDarkFormTime = -1000.0f;
	FTimerHandle DarkFormTimerHandle;
	ECollisionResponse DefaultPawnCollisionResponse = ECR_Block;
};
