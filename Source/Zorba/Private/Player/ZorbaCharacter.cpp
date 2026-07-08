// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/ZorbaCharacter.h"

#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/ZorbaPlayerController.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"
#include "UObject/ConstructorHelpers.h"

AZorbaCharacter::AZorbaCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 600.0f;
	CameraBoom->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	DebugBodyMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugBodyMesh"));
	DebugBodyMesh->SetupAttachment(RootComponent);
	DebugBodyMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 60.0f));
	DebugBodyMesh->SetRelativeScale3D(FVector(0.45f, 0.45f, 1.0f));
	DebugBodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugBodyMesh->SetHiddenInGame(false);

	DebugFacingMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DebugFacingMesh"));
	DebugFacingMesh->SetupAttachment(RootComponent);
	DebugFacingMesh->SetRelativeLocation(FVector(65.0f, 0.0f, 60.0f));
	DebugFacingMesh->SetRelativeScale3D(FVector(0.45f, 0.16f, 0.16f));
	DebugFacingMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	DebugFacingMesh->SetHiddenInGame(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		DebugBodyMesh->SetStaticMesh(CubeMesh.Object);
		DebugFacingMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void AZorbaCharacter::BeginPlay()
{
	Super::BeginPlay();
}

void AZorbaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	AddDefaultMappingContext();
	BindEnhancedInput(PlayerInputComponent);
}

void AZorbaCharacter::AddDefaultMappingContext()
{
	if (!DefaultMappingContext)
	{
		return;
	}

	const APlayerController* PlayerController = Cast<APlayerController>(GetController());
	if (!PlayerController)
	{
		return;
	}

	ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer();
	if (!LocalPlayer)
	{
		return;
	}

	if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
	{
		InputSubsystem->AddMappingContext(DefaultMappingContext, DefaultMappingPriority);
	}
}

void AZorbaCharacter::BindEnhancedInput(UInputComponent* PlayerInputComponent)
{
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("ZorbaCharacter requires Enhanced Input. Check DefaultInputComponentClass in project settings."));
		return;
	}

	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AZorbaCharacter::Move);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Completed, this, &AZorbaCharacter::StopMoveInput);
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Canceled, this, &AZorbaCharacter::StopMoveInput);
	}

	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction, ETriggerEvent::Triggered, this, &AZorbaCharacter::Look);
	}

	if (LookRateAction)
	{
		EnhancedInputComponent->BindAction(LookRateAction, ETriggerEvent::Triggered, this, &AZorbaCharacter::LookAtRate);
	}

	if (PrimaryAttackAction)
	{
		EnhancedInputComponent->BindAction(PrimaryAttackAction, ETriggerEvent::Started, this, &AZorbaCharacter::RequestPrimaryAttack);
	}

	if (HeavyAttackAction)
	{
		EnhancedInputComponent->BindAction(HeavyAttackAction, ETriggerEvent::Started, this, &AZorbaCharacter::RequestHeavyAttack);
	}

	if (DefendAction)
	{
		EnhancedInputComponent->BindAction(DefendAction, ETriggerEvent::Started, this, &AZorbaCharacter::StartDefend);
		EnhancedInputComponent->BindAction(DefendAction, ETriggerEvent::Completed, this, &AZorbaCharacter::StopDefend);
		EnhancedInputComponent->BindAction(DefendAction, ETriggerEvent::Canceled, this, &AZorbaCharacter::StopDefend);
	}

	if (DodgeAction)
	{
		EnhancedInputComponent->BindAction(DodgeAction, ETriggerEvent::Started, this, &AZorbaCharacter::RequestDodge);
	}

	if (ProfaneDashAction)
	{
		EnhancedInputComponent->BindAction(ProfaneDashAction, ETriggerEvent::Started, this, &AZorbaCharacter::RequestProfaneDash);
	}

	if (SprintAction)
	{
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Started, this, &AZorbaCharacter::StartSprintInput);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Completed, this, &AZorbaCharacter::StopSprintInput);
		EnhancedInputComponent->BindAction(SprintAction, ETriggerEvent::Canceled, this, &AZorbaCharacter::StopSprintInput);
	}

	if (AbilityLayerAction)
	{
		EnhancedInputComponent->BindAction(AbilityLayerAction, ETriggerEvent::Started, this, &AZorbaCharacter::StartAbilityLayer);
		EnhancedInputComponent->BindAction(AbilityLayerAction, ETriggerEvent::Completed, this, &AZorbaCharacter::StopAbilityLayer);
		EnhancedInputComponent->BindAction(AbilityLayerAction, ETriggerEvent::Canceled, this, &AZorbaCharacter::StopAbilityLayer);
	}

	if (ContextActionInput)
	{
		EnhancedInputComponent->BindAction(ContextActionInput, ETriggerEvent::Started, this, &AZorbaCharacter::RequestContextAction);
	}

	if (ClassActionInput)
	{
		EnhancedInputComponent->BindAction(ClassActionInput, ETriggerEvent::Started, this, &AZorbaCharacter::RequestClassAction);
	}

	if (UseAbilitySlot1Action)
	{
		EnhancedInputComponent->BindAction(UseAbilitySlot1Action, ETriggerEvent::Started, this, &AZorbaCharacter::RequestAbilitySlot1);
	}

	if (UseAbilitySlot2Action)
	{
		EnhancedInputComponent->BindAction(UseAbilitySlot2Action, ETriggerEvent::Started, this, &AZorbaCharacter::RequestAbilitySlot2);
	}

	if (UseAbilitySlot3Action)
	{
		EnhancedInputComponent->BindAction(UseAbilitySlot3Action, ETriggerEvent::Started, this, &AZorbaCharacter::RequestAbilitySlot3);
	}

	if (UseAbilitySlot4Action)
	{
		EnhancedInputComponent->BindAction(UseAbilitySlot4Action, ETriggerEvent::Started, this, &AZorbaCharacter::RequestAbilitySlot4);
	}

	if (UseRelicAction)
	{
		EnhancedInputComponent->BindAction(UseRelicAction, ETriggerEvent::Started, this, &AZorbaCharacter::RequestRelic);
	}

	if (ShowObjectiveAction)
	{
		EnhancedInputComponent->BindAction(ShowObjectiveAction, ETriggerEvent::Started, this, &AZorbaCharacter::ShowObjective);
	}

	if (CameraResetAction)
	{
		EnhancedInputComponent->BindAction(CameraResetAction, ETriggerEvent::Started, this, &AZorbaCharacter::ResetCamera);
	}

	if (PauseAction)
	{
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AZorbaCharacter::RequestPause);
	}
}

void AZorbaCharacter::Move(const FInputActionValue& Value)
{
	const FVector2D MovementVector = Value.Get<FVector2D>();

	if (MovementVector.IsNearlyZero())
	{
		StopMoveInput();
		return;
	}

	if (!bHasMoveInput && bSprintInputHeld)
	{
		bSprintToggledOn = true;
	}

	bHasMoveInput = true;
	MoveForward(MovementVector.Y);
	MoveRight(MovementVector.X);
	RefreshSprintState();
}

void AZorbaCharacter::StopMoveInput()
{
	bHasMoveInput = false;
	bSprintToggledOn = false;
	RefreshSprintState();
}

void AZorbaCharacter::Look(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();
	Turn(LookVector.X);
	LookUp(LookVector.Y);
}

void AZorbaCharacter::LookAtRate(const FInputActionValue& Value)
{
	const FVector2D LookVector = Value.Get<FVector2D>();
	TurnAtRate(LookVector.X);
	LookUpAtRate(LookVector.Y);
}

void AZorbaCharacter::MoveForward(float Value)
{
	if (FMath::IsNearlyZero(Value) || Controller == nullptr)
	{
		return;
	}

	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	AddMovementInput(ForwardDirection, Value);
	FaceCameraYaw();
}

void AZorbaCharacter::MoveRight(float Value)
{
	if (FMath::IsNearlyZero(Value) || Controller == nullptr)
	{
		return;
	}

	const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	AddMovementInput(RightDirection, Value);
	FaceCameraYaw();
}

void AZorbaCharacter::Turn(float Value)
{
	AddControllerYawInput(Value);
}

void AZorbaCharacter::LookUp(float Value)
{
	AddControllerPitchInput(Value);
}

void AZorbaCharacter::TurnAtRate(float Value)
{
	const UWorld* World = GetWorld();
	const float DeltaSeconds = World ? World->GetDeltaSeconds() : 0.0f;
	AddControllerYawInput(Value * GamepadTurnRate * DeltaSeconds);
}

void AZorbaCharacter::LookUpAtRate(float Value)
{
	const UWorld* World = GetWorld();
	const float DeltaSeconds = World ? World->GetDeltaSeconds() : 0.0f;
	AddControllerPitchInput(Value * GamepadLookUpRate * DeltaSeconds);
}

void AZorbaCharacter::FaceCameraYaw()
{
	if (Controller == nullptr)
	{
		return;
	}

	SetActorRotation(FRotator(0.0f, Controller->GetControlRotation().Yaw, 0.0f));
}

void AZorbaCharacter::StartSprintInput()
{
	bSprintInputHeld = true;

	if (bHasMoveInput)
	{
		bSprintToggledOn = !bSprintToggledOn;
	}

	RefreshSprintState();
}

void AZorbaCharacter::StopSprintInput()
{
	bSprintInputHeld = false;

	if (!bHasMoveInput)
	{
		bSprintToggledOn = false;
	}

	RefreshSprintState();
}

void AZorbaCharacter::RefreshSprintState()
{
	SetSprinting(bHasMoveInput && bSprintToggledOn);
}

void AZorbaCharacter::SetSprinting(bool bNewIsSprinting)
{
	if (bIsSprinting == bNewIsSprinting)
	{
		return;
	}

	bIsSprinting = bNewIsSprinting;
	GetCharacterMovement()->MaxWalkSpeed = bIsSprinting ? SprintSpeed : WalkSpeed;
	UE_LOG(LogTemp, Log, TEXT("Sprint %s."), bIsSprinting ? TEXT("started") : TEXT("stopped"));
}

void AZorbaCharacter::StartAbilityLayer()
{
	bAbilityLayerHeld = true;
	UE_LOG(LogTemp, Log, TEXT("Ability layer started."));
}

void AZorbaCharacter::StopAbilityLayer()
{
	bAbilityLayerHeld = false;
	UE_LOG(LogTemp, Log, TEXT("Ability layer stopped."));
}

void AZorbaCharacter::RequestPrimaryAttack()
{
	UE_LOG(LogTemp, Log, TEXT("Primary attack requested."));
	OnPrimaryAttackRequested();
}

void AZorbaCharacter::RequestHeavyAttack()
{
	UE_LOG(LogTemp, Log, TEXT("Heavy attack requested."));
	OnHeavyAttackRequested();
}

void AZorbaCharacter::StartDefend()
{
	UE_LOG(LogTemp, Log, TEXT("Defend started."));
	OnDefendStarted();
}

void AZorbaCharacter::StopDefend()
{
	UE_LOG(LogTemp, Log, TEXT("Defend stopped."));
	OnDefendStopped();
}

void AZorbaCharacter::RequestDodge()
{
	if (TryRouteAbilityLayerFaceButton(1))
	{
		return;
	}

	FVector DodgeDirection = GetLastMovementInputVector();
	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = GetActorForwardVector();
	}

	LaunchCharacter(DodgeDirection.GetSafeNormal() * DodgeStrength, true, false);

	UE_LOG(LogTemp, Log, TEXT("Dodge requested."));
	OnDodgeRequested();
}

void AZorbaCharacter::RequestProfaneDash()
{
	if (TryRouteAbilityLayerFaceButton(2))
	{
		return;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	const float CooldownRemaining = ProfaneDashCooldown - (CurrentTime - LastProfaneDashTime);

	if (CooldownRemaining > 0.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("Profane dash denied. Cooldown remaining: %.2f"), CooldownRemaining);
		OnProfaneDashDenied();
		return;
	}

	FaceCameraYaw();
	LaunchCharacter(GetActorForwardVector() * ProfaneDashStrength, true, false);

	LastProfaneDashTime = CurrentTime;

	UE_LOG(LogTemp, Log, TEXT("Profane dash requested."));
	OnProfaneDashRequested();
}

void AZorbaCharacter::RequestContextAction()
{
	if (TryRouteAbilityLayerFaceButton(3))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Context action requested."));
	OnContextActionRequested();
}

void AZorbaCharacter::RequestClassAction()
{
	if (TryRouteAbilityLayerFaceButton(4))
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("Class action requested."));
	OnClassActionRequested();
}

void AZorbaCharacter::RequestAbilitySlot1()
{
	RequestAbilitySlot(1);
}

void AZorbaCharacter::RequestAbilitySlot2()
{
	RequestAbilitySlot(2);
}

void AZorbaCharacter::RequestAbilitySlot3()
{
	RequestAbilitySlot(3);
}

void AZorbaCharacter::RequestAbilitySlot4()
{
	RequestAbilitySlot(4);
}

void AZorbaCharacter::RequestAbilitySlot(int32 SlotIndex)
{
	UE_LOG(LogTemp, Log, TEXT("Ability slot %d requested."), SlotIndex);
	OnAbilitySlotRequested(SlotIndex);
}

bool AZorbaCharacter::TryRouteAbilityLayerFaceButton(int32 SlotIndex)
{
	if (!bAbilityLayerHeld)
	{
		return false;
	}

	RequestAbilitySlot(SlotIndex);
	return true;
}

void AZorbaCharacter::RequestRelic()
{
	UE_LOG(LogTemp, Log, TEXT("Relic requested."));
	OnRelicRequested();
}

void AZorbaCharacter::ShowObjective()
{
	UE_LOG(LogTemp, Log, TEXT("Show objective requested."));
}

void AZorbaCharacter::ResetCamera()
{
	if (Controller == nullptr)
	{
		return;
	}

	Controller->SetControlRotation(FRotator(-15.0f, GetActorRotation().Yaw, 0.0f));
}

void AZorbaCharacter::RequestPause()
{
	if (AZorbaPlayerController* ZorbaPlayerController = Cast<AZorbaPlayerController>(GetController()))
	{
		ZorbaPlayerController->TogglePause();
	}
}
