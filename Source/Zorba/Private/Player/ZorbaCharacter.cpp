// Copyright Epic Games, Inc. All Rights Reserved.

#include "Player/ZorbaCharacter.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/ZorbaPlayerController.h"
#include "Player/ZorbaPlayerState.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedPlayerInput.h"
#include "InputActionValue.h"
#include "UObject/ConstructorHelpers.h"
#include "Combat/ZorbaAttackDefinition.h"
#include "Combat/ZorbaCombatAttributeSet.h"
#include "Combat/ZorbaGameplayTags.h"
#include "Combat/ZorbaMeleeCombatComponent.h"
#include "DrawDebugHelpers.h"
#include "Enemy/ZorbaEnemyCharacter.h"
#include "Enemy/ZorbaEnemyCombatBrainComponent.h"
#include "Kismet/GameplayStatics.h"
#if !UE_BUILD_SHIPPING
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#endif

AZorbaCharacter::AZorbaCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;

	MeleeCombatComponent =
		CreateDefaultSubobject<UZorbaMeleeCombatComponent>(
			TEXT("MeleeCombatComponent"));

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

UAbilitySystemComponent* AZorbaCharacter::GetAbilitySystemComponent() const
{
	if (const AZorbaPlayerState* ZorbaPlayerState = GetPlayerState<AZorbaPlayerState>())
	{
		return ZorbaPlayerState->GetAbilitySystemComponent();
	}

	return nullptr;
}

float AZorbaCharacter::GetForbiddenTechniqueSlot1CooldownRemaining() const
{
	const UWorld* World = GetWorld();
	if (!World)
	{
		return 0.0f;
	}

	return FMath::Max(
		0.0f,
		ForbiddenTechniqueSlot1Cooldown
			- (World->GetTimeSeconds() - LastForbiddenTechniqueSlot1Time));
}

void AZorbaCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	InitializeAbilitySystem();
}

void AZorbaCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();
	InitializeAbilitySystem();
}

void AZorbaCharacter::InitializeAbilitySystem()
{
	if (AZorbaPlayerState* ZorbaPlayerState = GetPlayerState<AZorbaPlayerState>())
	{
		if (UAbilitySystemComponent* AbilitySystem = ZorbaPlayerState->GetAbilitySystemComponent())
		{
			AbilitySystem->InitAbilityActorInfo(ZorbaPlayerState, this);
		}
	}
}

void AZorbaCharacter::BeginPlay()
{
	Super::BeginPlay();

	DefaultPawnCollisionResponse = GetCapsuleComponent()->GetCollisionResponseToChannel(ECC_Pawn);
#if !UE_BUILD_SHIPPING
	ConfigureAdvancedCombatAutomation();
#endif
}

void AZorbaCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	RecoverCombatStamina(DeltaSeconds);
}

void AZorbaCharacter::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	if (bIsInDarkForm && OtherActor && OtherActor != this)
	{
		OnDarkFormPassedThroughActor(OtherActor);
	}
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

	if (DarkFormAction)
	{
		EnhancedInputComponent->BindAction(DarkFormAction, ETriggerEvent::Started, this, &AZorbaCharacter::RequestDarkForm);
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

	if (PauseAction)
	{
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &AZorbaCharacter::RequestPause);
	}
}

void AZorbaCharacter::Move(const FInputActionValue& Value)
{
	if (MeleeCombatComponent
		&& MeleeCombatComponent->IsAttackInProgress())
	{
		StopMoveInput();
		return;
	}

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
	if (MeleeCombatComponent
		&& MeleeCombatComponent->IsAttackInProgress())
	{
		return;
	}

	if (Controller == nullptr)
	{
		return;
	}

	SetActorRotation(FRotator(0.0f, Controller->GetControlRotation().Yaw, 0.0f));
}

FVector AZorbaCharacter::ResolveAttackDirection() const
{
	FVector ActorForward = GetActorForwardVector();
	ActorForward.Z = 0.0f;
	ActorForward = ActorForward.GetSafeNormal();

	if (ActorForward.IsNearlyZero())
	{
		ActorForward = FVector::ForwardVector;
	}

	if (!IsValid(PrimaryAttackDefinition))
	{
		return ActorForward;
	}

	if (PrimaryAttackDefinition->DirectionPolicy
		== EZorbaAttackDirectionPolicy::FacingOnly)
	{
		return ActorForward;
	}

	FVector2D MoveInput = FVector2D::ZeroVector;

	if (MoveAction)
	{
		if (const APlayerController* PlayerController =
			Cast<APlayerController>(Controller))
		{
			if (const UEnhancedPlayerInput* EnhancedPlayerInput =
				Cast<UEnhancedPlayerInput>(
					PlayerController->PlayerInput))
			{
				MoveInput =
					EnhancedPlayerInput
					->GetActionValue(MoveAction)
					.Get<FVector2D>();
			}
		}
	}

	// 현재 방향 입력이 없으면 이전 이동 방향이나 카메라를 사용하지 않는다.
	if (MoveInput.IsNearlyZero(0.1f))
	{
		return ActorForward;
	}

	const FVector Up = FVector::UpVector;

	FVector GroundForward = ActorForward;
	FVector GroundRight = GetActorRightVector().GetSafeNormal2D();

	if (FollowCamera)
	{
		const FVector CameraForward =
			FollowCamera->GetForwardVector();

		const FVector CameraRight =
			FollowCamera->GetRightVector();

		GroundForward =
			FVector::VectorPlaneProject(CameraForward, Up);

		GroundRight =
			FVector::VectorPlaneProject(CameraRight, Up);

		// 정수리 시점에서는 CameraForward의 수평 투영이 0에 가까워진다.
		if (GroundForward.SizeSquared()
			< FMath::Square(0.05f))
		{
			if (GroundRight.Normalize())
			{
				// 정수리에서 카메라를 살짝 내렸을 때와 같은 전방.
				GroundForward =
					FVector::CrossProduct(
						GroundRight,
						Up).GetSafeNormal();
			}
			else
			{
				GroundForward = ActorForward;
			}
		}
		else
		{
			GroundForward.Normalize();
		}
	}

	if (GroundForward.IsNearlyZero())
	{
		GroundForward = ActorForward;
	}

	GroundRight =
		FVector::CrossProduct(
			Up,
			GroundForward).GetSafeNormal();

	FVector AttackDirection =
		GroundForward * MoveInput.Y
		+ GroundRight * MoveInput.X;

	AttackDirection.Z = 0.0f;
	AttackDirection = AttackDirection.GetSafeNormal();

	return AttackDirection.IsNearlyZero()
		? ActorForward
		: AttackDirection;
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
	ApplyCurrentMovementSpeed();
	UE_LOG(LogTemp, Log, TEXT("Sprint %s."), bIsSprinting ? TEXT("started") : TEXT("stopped"));
}

void AZorbaCharacter::ApplyCurrentMovementSpeed()
{
	GetCharacterMovement()->MaxWalkSpeed = bIsInDarkForm ? DarkFormSpeed : (bIsSprinting ? SprintSpeed : WalkSpeed);
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
	if (bIsDefending)
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("Primary attack ignored while defending."));
		return;
	}

	const FVector AttackDirection =
		ResolveAttackDirection();
	if (TryStartOpportunityAttack(AttackDirection))
	{
		return;
	}

	StartAttackDefinition(PrimaryAttackDefinition, AttackDirection);
}

void AZorbaCharacter::RequestHeavyAttack()
{
	if (bIsDefending || !MeleeCombatComponent)
	{
		return;
	}

	const FVector AttackDirection = ResolveAttackDirection();
	if (MeleeCombatComponent->IsAttackInProgress())
	{
		UZorbaAttackDefinition* ActiveAttackDefinition =
			MeleeCombatComponent->GetActiveAttackDefinition();
		if (ActiveAttackDefinition != PrimaryAttackDefinition
			|| ActiveAttackDefinition->AttackKind
				!= EZorbaAttackKind::Standard
			|| !MeleeCombatComponent->IsHeavyBranchWindowOpen()
			|| !IsValid(DerivedHeavyAttackDefinition))
		{
			UE_LOG(
				LogTemp,
				Verbose,
				TEXT("Heavy attack ignored: derived-heavy window is closed."));
			return;
		}

		StartAttackDefinition(
			DerivedHeavyAttackDefinition,
			MeleeCombatComponent->GetCurrentAttackDirection(),
			nullptr,
			true);
		return;
	}

	StartAttackDefinition(HeavyAttackDefinition, AttackDirection);
}

bool AZorbaCharacter::TryStartOpportunityAttack(
	const FVector& AttackDirection)
{
	if (!MeleeCombatComponent
		|| MeleeCombatComponent->IsAttackInProgress()
		|| !IsValid(OpportunityAttackDefinition))
	{
		return false;
	}

	AActor* TargetActor = MeleeCombatComponent->FindBestEligibleTarget(
		OpportunityAttackDefinition,
		AttackDirection);
	if (!TargetActor)
	{
		return false;
	}

	FVector TargetDirection =
		(TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!StartAttackDefinition(
		OpportunityAttackDefinition,
		TargetDirection,
		TargetActor))
	{
		return false;
	}

	OnOpportunityAttackStarted(TargetActor);
	return true;
}

bool AZorbaCharacter::TryStartExecution(
	const FVector& AttackDirection)
{
	if (!MeleeCombatComponent
		|| MeleeCombatComponent->IsAttackInProgress()
		|| !IsValid(ExecutionAttackDefinition))
	{
		return false;
	}

	AActor* TargetActor = MeleeCombatComponent->FindBestEligibleTarget(
		ExecutionAttackDefinition,
		AttackDirection);
	if (!TargetActor)
	{
		return false;
	}

	FVector TargetDirection =
		(TargetActor->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!StartAttackDefinition(
		ExecutionAttackDefinition,
		TargetDirection,
		TargetActor))
	{
		return false;
	}

	OnExecutionStarted(TargetActor);
	return true;
}

void AZorbaCharacter::ApplyFodderParryExecutionBenefits()
{
	if (!HasAuthority()
		|| !MeleeCombatComponent
		|| !IsValid(ExecutionAttackDefinition))
	{
		return;
	}

	MeleeCombatComponent->ApplyInstantExecutionBenefits(
		ExecutionAttackDefinition);
}

bool AZorbaCharacter::StartAttackDefinition(
	UZorbaAttackDefinition* AttackDefinition,
	const FVector& AttackDirection,
	AActor* LockedTarget,
	bool bTransitionFromActiveAttack)
{
	if (!IsValid(AttackDefinition) || !MeleeCombatComponent)
	{
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("Attack rejected: definition or melee component is missing."));
		return false;
	}

	const bool bStarted = bTransitionFromActiveAttack
		? MeleeCombatComponent->TransitionAttack(
			AttackDefinition,
			AttackDirection)
		: MeleeCombatComponent->BeginAttack(
			AttackDefinition,
			AttackDirection,
			LockedTarget);
	if (!bStarted)
	{
		return false;
	}

	StopMovementForAttack();
	CombatStaminaRecoveryDelayRemaining = CombatStaminaRecoveryDelay;
	UE_LOG(
		LogTemp,
		Log,
		TEXT("Player attack requested: %s Direction=%s"),
		*AttackDefinition->AttackId.ToString(),
		*AttackDirection.ToCompactString());
	return true;
}

void AZorbaCharacter::StopMovementForAttack()
{
	ConsumeMovementInputVector();
	GetCharacterMovement()->StopMovementImmediately();
	StopMoveInput();
}

void AZorbaCharacter::RecoverCombatStamina(float DeltaSeconds)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!AbilitySystem
		|| AbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead))
	{
		return;
	}

	if (bIsDefending
		|| bIsInDarkForm
		|| (MeleeCombatComponent
			&& MeleeCombatComponent->IsAttackInProgress()))
	{
		CombatStaminaRecoveryDelayRemaining = CombatStaminaRecoveryDelay;
		return;
	}

	CombatStaminaRecoveryDelayRemaining = FMath::Max(
		0.0f,
		CombatStaminaRecoveryDelayRemaining - DeltaSeconds);
	if (CombatStaminaRecoveryDelayRemaining > 0.0f)
	{
		return;
	}

	const float CurrentStamina = AbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute());
	const float MaxStamina = AbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetMaxCombatStaminaAttribute());
	if (CurrentStamina >= MaxStamina)
	{
		return;
	}

	AbilitySystem->ApplyModToAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute(),
		EGameplayModOp::Additive,
		FMath::Max(0.0f, CombatStaminaRecoveryPerSecond) * DeltaSeconds);
	if (AbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute()) > 0.0f)
	{
		AbilitySystem->RemoveLooseGameplayTag(
			ZorbaGameplayTags::State_StaminaDepleted);
	}
}

#if !UE_BUILD_SHIPPING
void AZorbaCharacter::ConfigureAdvancedCombatAutomation()
{
	FString TestMode;
	if (!FParse::Value(
		FCommandLine::Get(),
		TEXT("ZorbaCombatTest="),
		TestMode))
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	auto FindTestEnemy = [this]() -> AZorbaEnemyCharacter*
	{
		TArray<AActor*> Enemies;
		UGameplayStatics::GetAllActorsOfClass(
			this,
			AZorbaEnemyCharacter::StaticClass(),
			Enemies);
		Enemies.Sort(
			[this](const AActor& Left, const AActor& Right)
			{
				return FVector::DistSquared(
					GetActorLocation(),
					Left.GetActorLocation())
					< FVector::DistSquared(
						GetActorLocation(),
					Right.GetActorLocation());
			});
		AZorbaEnemyCharacter* FallbackEnemy = nullptr;
		for (AActor* EnemyActor : Enemies)
		{
			AZorbaEnemyCharacter* Enemy =
				Cast<AZorbaEnemyCharacter>(EnemyActor);
			if (!Enemy || Enemy->IsDead())
			{
				continue;
			}
			if (!Enemy->IsFodder())
			{
				return Enemy;
			}
			if (!FallbackEnemy)
			{
				FallbackEnemy = Enemy;
			}
		}
		return FallbackEnemy;
	};

	if (TestMode.Equals(TEXT("Heavy"), ESearchCase::IgnoreCase))
	{
		FTimerHandle TestTimer;
		World->GetTimerManager().SetTimer(
			TestTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this]() { RequestHeavyAttack(); }),
			0.25f,
			false);
	}
	else if (TestMode.Equals(
		TEXT("DerivedHeavy"),
		ESearchCase::IgnoreCase))
	{
		FTimerHandle LightTimer;
		World->GetTimerManager().SetTimer(
			LightTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this]() { RequestPrimaryAttack(); }),
			0.2f,
			false);
		FTimerHandle HeavyTimer;
		World->GetTimerManager().SetTimer(
			HeavyTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this]() { RequestHeavyAttack(); }),
			1.15f,
			false);
	}
	else if (TestMode.Equals(
		TEXT("HeavyRepeat"),
		ESearchCase::IgnoreCase))
	{
		FTimerHandle FirstHeavyTimer;
		World->GetTimerManager().SetTimer(
			FirstHeavyTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this]() { RequestHeavyAttack(); }),
			0.2f,
			false);
		FTimerHandle ReentryTimer;
		World->GetTimerManager().SetTimer(
			ReentryTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this]() { RequestHeavyAttack(); }),
			1.35f,
			false);
		FTimerHandle CompletedRepeatTimer;
		World->GetTimerManager().SetTimer(
			CompletedRepeatTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this]() { RequestHeavyAttack(); }),
			3.45f,
			false);
	}
	else if (TestMode.Equals(
		TEXT("Opportunity"),
		ESearchCase::IgnoreCase))
	{
		FTimerHandle TestTimer;
		World->GetTimerManager().SetTimer(
			TestTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this, FindTestEnemy]()
				{
					AZorbaEnemyCharacter* Enemy = FindTestEnemy();
					if (!Enemy || !Enemy->GetAbilitySystemComponent())
					{
						return;
					}
					Enemy->GetAbilitySystemComponent()->ApplyModToAttribute(
						UZorbaCombatAttributeSet::GetCombatStaminaAttribute(),
						EGameplayModOp::Additive,
						-Enemy->GetCombatAttributes()->GetCombatStamina());
					FHitResult HitResult;
					HitResult.ImpactPoint = Enemy->GetActorLocation();
					Enemy->HandleMeleeHit(this, HitResult);
					RequestPrimaryAttack();
				}),
			0.25f,
			false);
	}
	else if (TestMode.Equals(
		TEXT("Execution"),
		ESearchCase::IgnoreCase))
	{
		FTimerHandle TestTimer;
		World->GetTimerManager().SetTimer(
			TestTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this, FindTestEnemy]()
				{
					AZorbaEnemyCharacter* Enemy = FindTestEnemy();
					UAbilitySystemComponent* PlayerAbilitySystem =
						GetAbilitySystemComponent();
					if (!Enemy
						|| !Enemy->GetAbilitySystemComponent()
						|| !PlayerAbilitySystem)
					{
						return;
					}
					Enemy->GetAbilitySystemComponent()->ApplyModToAttribute(
						UZorbaCombatAttributeSet::GetHealthAttribute(),
						EGameplayModOp::Additive,
						-80.0f);
					PlayerAbilitySystem->ApplyModToAttribute(
						UZorbaCombatAttributeSet::GetHealthAttribute(),
						EGameplayModOp::Additive,
						-60.0f);
					PlayerAbilitySystem->ApplyModToAttribute(
						UZorbaCombatAttributeSet::GetCombatStaminaAttribute(),
						EGameplayModOp::Additive,
						-75.0f);
					RequestContextAction();
				}),
			0.25f,
			false);
	}
	else if (TestMode.Equals(
		TEXT("RearDoctrine"),
		ESearchCase::IgnoreCase)
		|| TestMode.Equals(
			TEXT("FrontDoctrineBaseline"),
			ESearchCase::IgnoreCase))
	{
		const bool bRearAttack = TestMode.Equals(
			TEXT("RearDoctrine"),
			ESearchCase::IgnoreCase);
		FTimerHandle DoctrineTimer;
		World->GetTimerManager().SetTimer(
			DoctrineTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this, FindTestEnemy, bRearAttack]()
				{
					AZorbaEnemyCharacter* Enemy = FindTestEnemy();
					if (!Enemy || !PrimaryAttackDefinition)
					{
						UE_LOG(
							LogTemp,
							Error,
							TEXT("Sacred doctrine automation missing enemy or primary attack."));
						return;
					}

					if (UZorbaEnemyCombatBrainComponent* EnemyBrain =
						Enemy->FindComponentByClass<UZorbaEnemyCombatBrainComponent>())
					{
						EnemyBrain->StopBrain();
					}
					if (UZorbaMeleeCombatComponent* EnemyCombat =
						Enemy->FindComponentByClass<UZorbaMeleeCombatComponent>())
					{
						EnemyCombat->InterruptAttack(0.0f);
					}
					Enemy->StopDefend();

					if (UAbilitySystemComponent* EnemyAbilitySystem =
						Enemy->GetAbilitySystemComponent())
					{
						EnemyAbilitySystem->SetNumericAttributeBase(
							UZorbaCombatAttributeSet::GetHealthAttribute(),
							EnemyAbilitySystem->GetNumericAttribute(
								UZorbaCombatAttributeSet::GetMaxHealthAttribute()));
						EnemyAbilitySystem->SetNumericAttributeBase(
							UZorbaCombatAttributeSet::GetCombatStaminaAttribute(),
							EnemyAbilitySystem->GetNumericAttribute(
								UZorbaCombatAttributeSet::GetMaxCombatStaminaAttribute()));
					}

					const FVector EnemyForward = FVector::ForwardVector;
					Enemy->SetActorRotation(EnemyForward.Rotation());
					const FVector EnemyLocation = Enemy->GetActorLocation();
					const FVector TestLocation = EnemyLocation
						+ EnemyForward * (bRearAttack ? -150.0f : 150.0f);
					SetActorLocation(
						TestLocation,
						false,
						nullptr,
						ETeleportType::TeleportPhysics);
					const FVector AttackDirection =
						(EnemyLocation - TestLocation).GetSafeNormal2D();
					SetActorRotation(AttackDirection.Rotation());
					GetCharacterMovement()->StopMovementImmediately();
					Enemy->StartDefend(5.0f);

					const bool bAttackStarted = StartAttackDefinition(
						PrimaryAttackDefinition,
						AttackDirection,
						Enemy);
					UE_LOG(
						LogTemp,
						Display,
						TEXT("Sacred doctrine automation prepared: Rear=%s AttackStarted=%s Enemy=%s"),
						bRearAttack ? TEXT("true") : TEXT("false"),
						bAttackStarted ? TEXT("true") : TEXT("false"),
						*GetNameSafe(Enemy));
				}),
			0.25f,
			false);
	}
	else if (TestMode.Equals(
		TEXT("ForbiddenTechnique"),
		ESearchCase::IgnoreCase))
	{
		FTimerHandle ForbiddenTechniqueTimer;
		World->GetTimerManager().SetTimer(
			ForbiddenTechniqueTimer,
			FTimerDelegate::CreateWeakLambda(
				this,
				[this, FindTestEnemy]()
				{
					AZorbaEnemyCharacter* Enemy = FindTestEnemy();
					if (!Enemy
						|| !Enemy->GetAbilitySystemComponent()
						|| !Enemy->GetCombatAttributes())
					{
						UE_LOG(
							LogTemp,
							Error,
							TEXT("Forbidden technique automation missing an eligible enemy."));
						return;
					}

					RequestAbilitySlot(1);
					UE_LOG(
						LogTemp,
						Display,
						TEXT("Forbidden technique automation invalid target: Cooldown=%.2f"),
						GetForbiddenTechniqueSlot1CooldownRemaining());

					if (UZorbaEnemyCombatBrainComponent* EnemyBrain =
						Enemy->FindComponentByClass<UZorbaEnemyCombatBrainComponent>())
					{
						EnemyBrain->StopBrain();
					}
					Enemy->StopDefend();
					const FVector EnemyLocation = Enemy->GetActorLocation();
					const FVector TestLocation = EnemyLocation - FVector::ForwardVector * 400.0f;
					SetActorLocation(
						TestLocation,
						false,
						nullptr,
						ETeleportType::TeleportPhysics);
					SetActorRotation((EnemyLocation - TestLocation).Rotation());
					GetCharacterMovement()->StopMovementImmediately();

					UAbilitySystemComponent* EnemyAbilitySystem =
						Enemy->GetAbilitySystemComponent();
					const float MaxHealth = EnemyAbilitySystem->GetNumericAttribute(
						UZorbaCombatAttributeSet::GetMaxHealthAttribute());
					EnemyAbilitySystem->SetNumericAttributeBase(
						UZorbaCombatAttributeSet::GetHealthAttribute(),
						MaxHealth * 0.4f);
					FHitResult PatternTriggerHit;
					PatternTriggerHit.ImpactPoint = EnemyLocation;
					Enemy->HandleMeleeHit(this, PatternTriggerHit);

					const bool bWasEnraged = Enemy->IsEnraged();
					RequestAbilitySlot(1);
					UE_LOG(
						LogTemp,
						Display,
						TEXT("Forbidden technique automation activation: Enemy=%s WasEnraged=%s IsEnraged=%s IsStunned=%s Cooldown=%.2f"),
						*GetNameSafe(Enemy),
						bWasEnraged ? TEXT("true") : TEXT("false"),
						Enemy->IsEnraged() ? TEXT("true") : TEXT("false"),
						Enemy->IsStunned() ? TEXT("true") : TEXT("false"),
						GetForbiddenTechniqueSlot1CooldownRemaining());

					// A successful cast owns the cooldown even though the first target is no longer enraged.
					RequestAbilitySlot(1);
					TWeakObjectPtr<AZorbaEnemyCharacter> WeakEnemy = Enemy;
					FTimerHandle RecoveryTimer;
					GetWorldTimerManager().SetTimer(
						RecoveryTimer,
						FTimerDelegate::CreateWeakLambda(
							this,
							[this, WeakEnemy]()
							{
								const AZorbaEnemyCharacter* RecoveredEnemy = WeakEnemy.Get();
								UE_LOG(
									LogTemp,
									Display,
									TEXT("Forbidden technique automation recovery: Enemy=%s IsEnraged=%s IsStunned=%s Cooldown=%.2f"),
									*GetNameSafe(RecoveredEnemy),
									RecoveredEnemy && RecoveredEnemy->IsEnraged() ? TEXT("true") : TEXT("false"),
									RecoveredEnemy && RecoveredEnemy->IsStunned() ? TEXT("true") : TEXT("false"),
									GetForbiddenTechniqueSlot1CooldownRemaining());
							}),
						ForbiddenTechniqueSlot1StunDuration + 0.35f,
						false);
				}),
			0.25f,
			false);
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Advanced combat automation armed: Mode=%s"),
		*TestMode);
}
#endif

void AZorbaCharacter::StartDefend()
{
	if (bIsDefending
		|| (MeleeCombatComponent
			&& MeleeCombatComponent->IsAttackInProgress()))
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!AbilitySystem
		|| AbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead)
		|| AbilitySystem->GetNumericAttribute(
			UZorbaCombatAttributeSet::GetCombatStaminaAttribute()) <= 0.0f)
	{
		UE_LOG(
			LogTemp,
			Verbose,
			TEXT("Defend rejected: combat stamina is empty or the player is dead."));
		return;
	}

	bIsDefending = true;
	AbilitySystem->AddLooseGameplayTag(
		ZorbaGameplayTags::State_Defending);
	AbilitySystem->AddLooseGameplayTag(
		ZorbaGameplayTags::State_ParryWindow);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			ParryWindowTimerHandle,
			this,
			&AZorbaCharacter::CloseParryWindow,
			ParryWindowDuration,
			false);
	}

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Defend started. ParryWindow=%.2fs"),
		ParryWindowDuration);
	OnDefendStarted();
}

void AZorbaCharacter::StopDefend()
{
	if (!bIsDefending)
	{
		return;
	}

	bIsDefending = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ParryWindowTimerHandle);
	}

	if (UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponent())
	{
		AbilitySystem->RemoveLooseGameplayTag(
			ZorbaGameplayTags::State_ParryWindow);
		AbilitySystem->RemoveLooseGameplayTag(
			ZorbaGameplayTags::State_Defending);
	}

	UE_LOG(LogTemp, Log, TEXT("Defend stopped."));
	OnDefendStopped();
}

void AZorbaCharacter::CloseParryWindow()
{
	if (UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponent())
	{
		AbilitySystem->RemoveLooseGameplayTag(
			ZorbaGameplayTags::State_ParryWindow);
	}

	UE_LOG(LogTemp, Verbose, TEXT("Parry window closed; block remains active."));
}

#if !UE_BUILD_SHIPPING
void AZorbaCharacter::ConfigureDefenseForAutomation(
	bool bKeepParryWindowOpen)
{
	StartDefend();
	if (!bIsDefending)
	{
		UE_LOG(
			LogTemp,
			Error,
			TEXT("Defense automation could not enter the defending state."));
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ParryWindowTimerHandle);
	}

	if (!bKeepParryWindowOpen)
	{
		CloseParryWindow();
	}

	UE_LOG(
		LogTemp,
		Display,
		TEXT("Defense automation configured: Mode=%s"),
		bKeepParryWindowOpen ? TEXT("Parry") : TEXT("Block"));
}
#endif

EZorbaMeleeDefenseResult AZorbaCharacter::ResolveIncomingMeleeHit(
	AActor* SourceActor,
	const FHitResult& HitResult,
	const UZorbaAttackDefinition* IncomingAttack,
	float& InOutHealthDamage,
	float& InOutStaminaDamage,
	float& OutParryStaminaDamage)
{
	OutParryStaminaDamage = 0.0f;
	const UAbilitySystemComponent* AbilitySystem =
		GetAbilitySystemComponent();
	if (!bIsDefending
		|| !IsValid(SourceActor)
		|| !AbilitySystem
		|| !AbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Defending))
	{
		return EZorbaMeleeDefenseResult::None;
	}

	FVector ToSource = SourceActor->GetActorLocation() - GetActorLocation();
	ToSource.Z = 0.0f;
	ToSource = ToSource.GetSafeNormal();
	const FVector FacingDirection = GetActorForwardVector().GetSafeNormal2D();
	const float MinimumDefenseDot =
		FMath::Cos(FMath::DegreesToRadians(DefenseHalfAngleDegrees));
	if (ToSource.IsNearlyZero()
		|| FVector::DotProduct(FacingDirection, ToSource)
			< MinimumDefenseDot)
	{
		return EZorbaMeleeDefenseResult::None;
	}

	const EZorbaDefenseInteraction DefenseInteraction = IncomingAttack
		? IncomingAttack->DefenseInteraction
		: EZorbaDefenseInteraction::Standard;
	if (DefenseInteraction == EZorbaDefenseInteraction::DodgeOnly)
	{
		return EZorbaMeleeDefenseResult::None;
	}

	if (AbilitySystem->HasMatchingGameplayTag(
		ZorbaGameplayTags::State_ParryWindow))
	{
		InOutHealthDamage = 0.0f;
		InOutStaminaDamage = 0.0f;
		OutParryStaminaDamage = FMath::Max(0.0f, ParryStaminaDamage);
		return EZorbaMeleeDefenseResult::Parried;
	}

	if (DefenseInteraction == EZorbaDefenseInteraction::GuardBreak)
	{
		StopDefend();
		OnGuardBroken(SourceActor);
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

void AZorbaCharacter::HandleMeleeHit(
	AActor* SourceActor,
	const FHitResult& HitResult,
	EZorbaMeleeDefenseResult DefenseResult)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!AbilitySystem)
	{
		return;
	}

	const float RemainingHealth = AbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetHealthAttribute());
	const float RemainingStamina = AbilitySystem->GetNumericAttribute(
		UZorbaCombatAttributeSet::GetCombatStaminaAttribute());
	CombatStaminaRecoveryDelayRemaining = CombatStaminaRecoveryDelay;

	if (RemainingStamina <= 0.0f)
	{
		AbilitySystem->AddLooseGameplayTag(
			ZorbaGameplayTags::State_StaminaDepleted);
		StopDefend();
	}

	if (RemainingHealth <= 0.0f)
	{
		const bool bWasAlreadyDead = AbilitySystem->HasMatchingGameplayTag(
			ZorbaGameplayTags::State_Dead);
		if (!bWasAlreadyDead)
		{
			AbilitySystem->AddLooseGameplayTag(
				ZorbaGameplayTags::State_Dead);
			GetCharacterMovement()->DisableMovement();
			StopDefend();
			if (HasAuthority())
			{
				OnPlayerDied.Broadcast(this, SourceActor);
			}
		}
	}

	if (GetWorld())
	{
		const FColor DebugColor =
			DefenseResult == EZorbaMeleeDefenseResult::Parried
				? FColor::Green
				: DefenseResult == EZorbaMeleeDefenseResult::Blocked
					? FColor::Blue
					: DefenseResult == EZorbaMeleeDefenseResult::GuardBroken
						? FColor::Orange
						: FColor::Red;
		DrawDebugPoint(
			GetWorld(),
			HitResult.ImpactPoint,
			24.0f,
			DebugColor,
			false,
			1.0f,
			0);
	}

	OnMeleeHitReceived(
		SourceActor,
		HitResult,
		DefenseResult,
		RemainingHealth,
		RemainingStamina);

	UE_LOG(
		LogTemp,
		Log,
		TEXT("Player melee result: Defense=%s Health=%.1f Stamina=%.1f"),
		DefenseResult == EZorbaMeleeDefenseResult::Parried
			? TEXT("Parried")
			: DefenseResult == EZorbaMeleeDefenseResult::Blocked
				? TEXT("Blocked")
				: DefenseResult == EZorbaMeleeDefenseResult::GuardBroken
					? TEXT("GuardBroken")
					: TEXT("None"),
		RemainingHealth,
		RemainingStamina);
}

void AZorbaCharacter::RequestDodge()
{
	if (TryRouteAbilityLayerFaceButton(1))
	{
		return;
	}

	FVector DodgeDirection = FVector::ZeroVector;
	bool bReadCurrentMoveAction = false;

	if (MoveAction)
	{
		if (const APlayerController* PlayerController = Cast<APlayerController>(Controller))
		{
			if (const UEnhancedPlayerInput* EnhancedPlayerInput = Cast<UEnhancedPlayerInput>(PlayerController->PlayerInput))
			{
				bReadCurrentMoveAction = true;

				const FVector2D MoveInput = EnhancedPlayerInput->GetActionValue(MoveAction).Get<FVector2D>();
				if (!MoveInput.IsNearlyZero())
				{
					const FRotator YawRotation(0.0f, Controller->GetControlRotation().Yaw, 0.0f);
					const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
					const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
					DodgeDirection = ForwardDirection * MoveInput.Y + RightDirection * MoveInput.X;
				}
			}
		}
	}

	if (!bReadCurrentMoveAction)
	{
		DodgeDirection = GetPendingMovementInputVector();
		if (DodgeDirection.IsNearlyZero())
		{
			DodgeDirection = GetLastMovementInputVector();
		}
	}

	if (DodgeDirection.IsNearlyZero())
	{
		DodgeDirection = GetActorForwardVector();
	}

	DodgeDirection.Z = 0.0f;
	DodgeDirection = DodgeDirection.GetSafeNormal();
	LaunchCharacter(DodgeDirection * DodgeStrength, true, false);

	UE_LOG(LogTemp, Log, TEXT("Dodge requested. Direction=(%.2f, %.2f, %.2f)"), DodgeDirection.X, DodgeDirection.Y, DodgeDirection.Z);
	OnDodgeRequested();
}

void AZorbaCharacter::RequestDarkForm()
{
	if (TryRouteAbilityLayerFaceButton(2))
	{
		return;
	}

	const UWorld* World = GetWorld();
	const float CurrentTime = World ? World->GetTimeSeconds() : 0.0f;
	const float CooldownRemaining = DarkFormCooldown - (CurrentTime - LastDarkFormTime);

	if (bIsInDarkForm || CooldownRemaining > 0.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("Dark form denied. Cooldown remaining: %.2f"), FMath::Max(0.0f, CooldownRemaining));
		OnDarkFormDenied();
		return;
	}

	FaceCameraYaw();

	bIsInDarkForm = true;
	LastDarkFormTime = CurrentTime;
	ApplyCurrentMovementSpeed();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	if (World)
	{
		World->GetTimerManager().SetTimer(DarkFormTimerHandle, this, &AZorbaCharacter::FinishDarkForm, DarkFormDuration, false);
	}

	UE_LOG(LogTemp, Log, TEXT("Dark form started."));
	OnDarkFormStarted();
}

void AZorbaCharacter::FinishDarkForm()
{
	if (!bIsInDarkForm)
	{
		return;
	}

	bIsInDarkForm = false;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, DefaultPawnCollisionResponse);
	ApplyCurrentMovementSpeed();

	UE_LOG(LogTemp, Log, TEXT("Dark form ended."));
	OnDarkFormEnded();
}

void AZorbaCharacter::RequestContextAction()
{
	if (TryRouteAbilityLayerFaceButton(3))
	{
		return;
	}

	if (!bIsDefending
		&& TryStartExecution(ResolveAttackDirection()))
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
	if (SlotIndex == 1)
	{
		TryUseForbiddenTechniqueSlot1();
		return;
	}
}

bool AZorbaCharacter::TryUseForbiddenTechniqueSlot1()
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystemComponent();
	if (!HasAuthority()
		|| !AbilitySystem
		|| bIsDefending
		|| bIsInDarkForm
		|| (MeleeCombatComponent && MeleeCombatComponent->IsAttackInProgress())
		|| AbilitySystem->HasMatchingGameplayTag(ZorbaGameplayTags::State_Dead)
		|| AbilitySystem->HasMatchingGameplayTag(ZorbaGameplayTags::State_Stunned))
	{
		OnForbiddenTechniqueSlot1Denied(
			EZorbaForbiddenTechniqueFailure::InvalidState,
			GetForbiddenTechniqueSlot1CooldownRemaining());
		UE_LOG(LogTemp, Log, TEXT("Forbidden technique slot 1 denied: InvalidState"));
		return false;
	}

	const float CooldownRemaining =
		GetForbiddenTechniqueSlot1CooldownRemaining();
	if (CooldownRemaining > 0.0f)
	{
		OnForbiddenTechniqueSlot1Denied(
			EZorbaForbiddenTechniqueFailure::Cooldown,
			CooldownRemaining);
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Forbidden technique slot 1 denied: Cooldown=%.2f"),
			CooldownRemaining);
		return false;
	}

	FVector AimDirection = ResolveAttackDirection().GetSafeNormal2D();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = GetActorForwardVector().GetSafeNormal2D();
	}
	AZorbaEnemyCharacter* Target =
		FindForbiddenTechniqueSlot1Target(AimDirection);
	if (!Target)
	{
		OnForbiddenTechniqueSlot1Denied(
			EZorbaForbiddenTechniqueFailure::NoEnragedTarget,
			0.0f);
		UE_LOG(
			LogTemp,
			Log,
			TEXT("Forbidden technique slot 1 denied: NoEnragedTarget Range=%.1f HalfAngle=%.1f"),
			ForbiddenTechniqueSlot1Range,
			ForbiddenTechniqueSlot1HalfAngleDegrees);
		return false;
	}

	if (!Target->BreakEnrageWithForbiddenTechnique(
		this,
		ForbiddenTechniqueSlot1StunDuration))
	{
		OnForbiddenTechniqueSlot1Denied(
			EZorbaForbiddenTechniqueFailure::NoEnragedTarget,
			0.0f);
		return false;
	}

	LastForbiddenTechniqueSlot1Time = GetWorld()
		? GetWorld()->GetTimeSeconds()
		: 0.0f;
	OnForbiddenTechniqueSlot1Started(Target);
#if !UE_BUILD_SHIPPING
	DrawDebugDirectionalArrow(
		GetWorld(),
		GetActorLocation() + FVector(0.0f, 0.0f, 80.0f),
		Target->GetActorLocation() + FVector(0.0f, 0.0f, 80.0f),
		45.0f,
		FColor(160, 64, 255),
		false,
		1.5f,
		0,
		4.0f);
#endif
	UE_LOG(
		LogTemp,
		Display,
		TEXT("Forbidden technique slot 1 activated: Target=%s Stun=%.2f Cooldown=%.2f"),
		*GetNameSafe(Target),
		ForbiddenTechniqueSlot1StunDuration,
		ForbiddenTechniqueSlot1Cooldown);
	return true;
}

AZorbaEnemyCharacter* AZorbaCharacter::FindForbiddenTechniqueSlot1Target(
	const FVector& AimDirection) const
{
	TArray<AActor*> EnemyActors;
	UGameplayStatics::GetAllActorsOfClass(
		this,
		AZorbaEnemyCharacter::StaticClass(),
		EnemyActors);

	const float SafeRange = FMath::Max(0.0f, ForbiddenTechniqueSlot1Range);
	const float MinimumAimDot = FMath::Cos(FMath::DegreesToRadians(
		FMath::Clamp(ForbiddenTechniqueSlot1HalfAngleDegrees, 0.0f, 180.0f)));
	const FVector SafeAimDirection = AimDirection.GetSafeNormal2D();
	AZorbaEnemyCharacter* BestTarget = nullptr;
	float BestScore = -BIG_NUMBER;
	for (AActor* EnemyActor : EnemyActors)
	{
		AZorbaEnemyCharacter* Enemy = Cast<AZorbaEnemyCharacter>(EnemyActor);
		if (!Enemy
			|| Enemy->IsDead()
			|| !Enemy->IsEnraged()
			|| Enemy->GetGenericTeamId() == GetGenericTeamId())
		{
			continue;
		}

		FVector ToEnemy = Enemy->GetActorLocation() - GetActorLocation();
		ToEnemy.Z = 0.0f;
		const float Distance = ToEnemy.Size();
		if (Distance > SafeRange)
		{
			continue;
		}

		const FVector DirectionToEnemy = Distance > UE_KINDA_SMALL_NUMBER
			? ToEnemy / Distance
			: SafeAimDirection;
		const float AimDot = FVector::DotProduct(
			SafeAimDirection,
			DirectionToEnemy);
		if (AimDot < MinimumAimDot)
		{
			continue;
		}

		const float DistanceScore = SafeRange > UE_KINDA_SMALL_NUMBER
			? 1.0f - Distance / SafeRange
			: 1.0f;
		const float Score = AimDot * 2.0f + DistanceScore;
		if (Score > BestScore)
		{
			BestScore = Score;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
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

void AZorbaCharacter::RequestPause()
{
	if (AZorbaPlayerController* ZorbaPlayerController = Cast<AZorbaPlayerController>(GetController()))
	{
		ZorbaPlayerController->TogglePause();
	}
}
