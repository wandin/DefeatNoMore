#include "DFNCharacter.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "TimerManager.h"

#include "Animation/AnimInstance.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "DefeatNoMore/PlayerState/DFNPlayerState.h"
#include "DefeatNoMore/Components/CombatComponent.h"
#include "DefeatNoMore/Weapons/Weapon.h"
#include "DefeatNoMore/DefeatNoMore.h"
#include "DefeatNoMore/GameModes/DFNGameMode.h"
#include "DefeatNoMore/PlayerController/DFNPlayerController.h"

#include "Engine/LocalPlayer.h"

#include "Materials/MaterialInstanceDynamic.h"

ADFNCharacter::ADFNCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArm->SetupAttachment(GetMesh());
	SpringArm->TargetArmLength = 600.f;
	SpringArm->bUsePawnControlRotation = true;
	
	// First Person camera - attached to mesh(head socket), location and rotation set.
	ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
	ThirdPersonCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	ThirdPersonCamera->bUsePawnControlRotation = false;
	
	//CombatComponent
	CombatComp = CreateDefaultSubobject<UCombatComponent>(TEXT("CombatComponent"));
	CombatComp->SetIsReplicated(true);
	
	//movement
	TurningInPlace = ETurnInPlace::ETIP_NotTurning;	
	GetCharacterMovement()->NavAgentProps.bCanCrouch = true;
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionObjectType(ECC_SkeletalMesh);
	GetMesh()->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	GetCharacterMovement()->bOrientRotationToMovement = true;
	
	NetUpdateFrequency = 128.f;
	MinNetUpdateFrequency = 64.f;

	DissolveTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeLineComponent"));
}

void ADFNCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFNCharacter, OverlappingWeapon);
	DOREPLIFETIME(ADFNCharacter, bWalking);
	DOREPLIFETIME(ADFNCharacter, bIsFiring);
	DOREPLIFETIME(ADFNCharacter, Health);
	DOREPLIFETIME(ADFNCharacter, bDisableGameplay); // disabling actions when needed
}

void ADFNCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if(IsValid(CombatComp))
	{
		CombatComp->DFNCharacter = this;
	}
}

void ADFNCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);
	
	if(ADFNPlayerController* PC = Cast<ADFNPlayerController>(GetController()))
	{
		/* Mapping Input*/
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Beginplay - Subsystem is valid!"));
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ADFNCharacter::BeginPlay()
{
	Super::BeginPlay();

	if(ADFNPlayerController* PC = Cast<ADFNPlayerController>(GetController()))
	{
		/* Mapping Input*/
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			UE_LOG(LogTemp, Warning, TEXT("Beginplay - Subsystem is valid!"));
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
	
	if(HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ADFNCharacter::ReceiveDamage);
	}
	UpdateHUDHealth();
}

void ADFNCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	RotateInPlace(DeltaSeconds);
	PollInit();
}

void ADFNCharacter::RotateInPlace(float DeltaSeconds)
{
	if(bDisableGameplay)
	{
		bUseControllerRotationYaw = false;
		TurningInPlace = ETurnInPlace::ETIP_NotTurning;
		return;
	}
	if(GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled())
	{
		AimOffset(DeltaSeconds);
	}
	else
	{
		TimeSinceLastMovementReplication += DeltaSeconds;
		if(TimeSinceLastMovementReplication> 0.05f)
		{
			OnRep_ReplicatedMovement();
		}
		CalculateAO_Pitch();
	}
}

void ADFNCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if(UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		//Jump
		EIC->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ADFNCharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ADFNCharacter::CharacterStopJumping);
		//Move
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADFNCharacter::Move);
		// Walk (not running / no footstep sounds)
		EIC->BindAction(WalkAction, ETriggerEvent::Triggered, this, &ADFNCharacter::WalkPressed);
		EIC->BindAction(WalkAction, ETriggerEvent::Completed, this, &ADFNCharacter::WalkReleased);
		//Look
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADFNCharacter::Look);
		//Crouch
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ADFNCharacter::CrouchPressed);
		//Equip
		EIC->BindAction(EquipAction, ETriggerEvent::Triggered, this, &ADFNCharacter::EquipButtonPressed);
		// Aim - ZoomFOV
		EIC->BindAction(AimAction, ETriggerEvent::Triggered, this, &ADFNCharacter::AimButtonPressed);
		EIC->BindAction(AimAction, ETriggerEvent::Completed, this, &ADFNCharacter::AimButtonReleased);
		//Fire
		EIC->BindAction(FireAction, ETriggerEvent::Started, this, &ADFNCharacter::FireButtonPressed);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &ADFNCharacter::FireButtonReleased);
		//Reload
		EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &ADFNCharacter::ReloadButtonPressed);
	}
}

void ADFNCharacter::Move(const FInputActionValue& Value)
{
	if(bDisableGameplay) return;
	
	// AddMovementInput takes a Vector
	const FVector2d MovementVector = Value.Get<FVector2d>();
	AddMovementInput(GetActorForwardVector(), MovementVector.Y);
	AddMovementInput(GetActorRightVector(), MovementVector.X);
}

void ADFNCharacter::Look(const FInputActionValue& Value)
{
	// AddControllerPitchInput takes a Vector
	const FVector2d LookAxisVector = Value.Get<FVector2d>();

	APlayerController* PC = Cast<APlayerController>(GetController());
	if(PC)
	{
		PC->AddPitchInput(LookAxisVector.Y);
	}
	AddControllerYawInput(LookAxisVector.X);
}

void ADFNCharacter::Jump()
{
	if(bDisableGameplay) return;
	if(bIsCrouched)
	{
		UnCrouch();
	}
	Super::Jump();
}

void ADFNCharacter::CharacterStopJumping()
{
	StopJumping();
}

void ADFNCharacter::CrouchPressed()
{
	if(bDisableGameplay) return;
	if(bIsCrouched)
	{
		UnCrouch();
	}
	Crouch();
}

void ADFNCharacter::WalkPressed()
{
	if(IsLocallyControlled())
	{
		ServerWalkPressed();
	}
	bWalking = true;
	GetCharacterMovement()->MaxWalkSpeed = 220.f;
}

void ADFNCharacter::WalkReleased()
{
	if(IsLocallyControlled())
	{
		ServerWalkReleased();
	}
	bWalking = false;
	GetCharacterMovement()->MaxWalkSpeed = 420.f;
}

void ADFNCharacter::EquipButtonPressed()
{
	if (bDisableGameplay) return;
	if (CombatComp)
	{
		if (HasAuthority())
		{
			CombatComp->EquipWeapon(OverlappingWeapon);
		}
		else
		{
			ServerEquipButtonPressed();
		}
	}
}

void ADFNCharacter::ServerEquipButtonPressed_Implementation()
{
	if(CombatComp)
	{
		CombatComp->EquipWeapon(OverlappingWeapon);
	}
}

void ADFNCharacter::ServerWalkPressed_Implementation()
{
	bWalking = true;
	GetCharacterMovement()->MaxWalkSpeed = 220.f;
}

void ADFNCharacter::ServerWalkReleased_Implementation()
{
	bWalking = false;
	GetCharacterMovement()->MaxWalkSpeed = 420.f;
}

void ADFNCharacter::ReloadButtonPressed()
{
	if(bDisableGameplay) return;
	if(GetCombatComponent())
	{
		GetCombatComponent()->Reload();
	}
}

float ADFNCharacter::CalculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void ADFNCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
}

void ADFNCharacter::AimOffset(float DeltaSeconds)
{
	if (CombatComp && CombatComp->EquippedWeapon == nullptr) return;
	const float Speed = CalculateSpeed();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	//	Yaw
	if(Speed > 0.f && !bIsInAir)
	{
		bRotateRootBone = true;
		const FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(CurrentAimRotation, StartingAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurnInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
		bUseControllerRotationYaw = true;
		TurnInPlace(DeltaSeconds);
	}
	if (Speed > 0.f || bIsInAir) // running, or jumping
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		bUseControllerRotationYaw = true;
		TurningInPlace = ETurnInPlace::ETIP_NotTurning;
	}
	TurnInPlace(DeltaSeconds);
	// Pitch
	CalculateAO_Pitch();
}

void ADFNCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if(AO_Pitch > 90.f && !IsLocallyControlled())
	{
		const FVector2d InRange(270.f, 360.f);
		const FVector2d OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void ADFNCharacter::SimProxiesTurn()
{
	if(CombatComp == nullptr || CombatComp->EquippedWeapon == nullptr) return;
	
	const float Speed = CalculateSpeed();
	if(Speed > 0.f)
	{
		bRotateRootBone = false;
		AO_Yaw = 0.f;
		TurningInPlace = ETurnInPlace::ETIP_NotTurning;
		return;
	}
	// Calculate difference rotation last frame
	ProxyRotationLastFrame = ProxyRotation;
	ProxyRotation = GetActorRotation();
	Proxy_Yaw = UKismetMathLibrary::NormalizedDeltaRotator(ProxyRotation, ProxyRotationLastFrame).Yaw;

	if(FMath::Abs(Proxy_Yaw) > TurnThreshHold)
	{
		if(Proxy_Yaw > TurnThreshHold)
		{
			TurningInPlace = ETurnInPlace::ETIP_Right;
		}
		else if (Proxy_Yaw < -TurnThreshHold)
		{
			TurningInPlace = ETurnInPlace::ETIP_Left;
		}
		else
		{
			TurningInPlace = ETurnInPlace::ETIP_NotTurning;
		}
		return;
	}
	TurningInPlace = ETurnInPlace::ETIP_NotTurning;

}

void ADFNCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.0f;
}

void ADFNCharacter::EquipWeaponOnCombatComponent()
{
	if(IsValid(CombatComp))
	{
		if(!HasAuthority())
		{
			ServerEquipWeaponOnCombatComponent();
		}
		CombatComp->EquipWeapon(OverlappingWeapon);
	}
}

void ADFNCharacter::ServerEquipWeaponOnCombatComponent()
{
	if(IsValid(CombatComp))
	{
		CombatComp->EquipWeapon(OverlappingWeapon);
	}
}

void ADFNCharacter::TurnInPlace(float DeltaSeconds)
{
	if (AO_Yaw > 90.f)
	{
		TurningInPlace = ETurnInPlace::ETIP_Right;
	}
	else if (AO_Yaw < -90.f)
	{
		TurningInPlace = ETurnInPlace::ETIP_Left;
	}
	if (TurningInPlace != ETurnInPlace::ETIP_NotTurning)
	{
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaSeconds, 3.f);
		AO_Yaw = InterpAO_Yaw;
		if (AO_Yaw < 15.f)
		{
			TurningInPlace = ETurnInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ADFNCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	OverlappingWeapon = Weapon;

	if(!CombatComp || CombatComp->EquippedWeapon) return; // if CombatComp not valid, or we already have a EquippedWeapon return

	// Otherwise we Equip a weapon just by simply overlapping it.
	if(IsLocallyControlled() && HasAuthority())
	{
		if(OverlappingWeapon)
		{
			// Equip Weapon
			EquipWeaponOnCombatComponent();
		}
	}
	if(OverlappingWeapon)
	{
		// Equip Weapon
		ServerEquipWeaponOnCombatComponent();
	}
}

AWeapon* ADFNCharacter::GetEquippedWeapon() const
{
	if(!IsValid(CombatComp)) return nullptr;
	return CombatComp->EquippedWeapon;
}

void ADFNCharacter::AimButtonPressed()
{
	if(bDisableGameplay) return;
	if(CombatComp)
	{
		CombatComp->SetAiming(true);
	}
}

void ADFNCharacter::AimButtonReleased()
{
	if(bDisableGameplay) return;
	if(CombatComp)
	{
		CombatComp->SetAiming(false);
	}
}

bool ADFNCharacter::bIsAiming() const
{
	return (CombatComp && CombatComp->bAiming);	
}
void ADFNCharacter::FireButtonPressed()
{
	if(bDisableGameplay) return;
	if(CombatComp)
	{
		CombatComp->FireButtonPressed(true);
	}
}

void ADFNCharacter::FireButtonReleased()
{
	if(bDisableGameplay) return;
	if(CombatComp)
	{
		CombatComp->FireButtonPressed(false);
	}
}

void ADFNCharacter::PlayFireMontage(bool bFiring)
{
	if(!IsValid(CombatComp) || !IsValid(CombatComp->EquippedWeapon)) return;

	bIsFiring = bFiring;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && FireWeaponMontage && bIsFiring)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
	}
}

void ADFNCharacter::PlayReloadMontage() const
{
	if (CombatComp == nullptr || CombatComp->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (CombatComp->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_Pistol:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_SMG:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
		case EWeaponType::EWT_RocketLauncher:
			SectionName = FName("Rifle");
			break;
		default: ;
		}
		AnimInstance->Montage_JumpToSection(SectionName);
	}
}

void ADFNCharacter::PlayHitReactMontage() const
{
	if(!IsValid(CombatComp) || !IsValid(CombatComp->EquippedWeapon)) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HitReactMontage)
	{
		if(!IsLocallyControlled())
		{
			AnimInstance->Montage_Play(HitReactMontage);
		}
	}
}

void ADFNCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();

	if(Health == 0.f)
	{
		ADFNGameMode* DFNGameMode = GetWorld()->GetAuthGameMode<ADFNGameMode>();
		if(DFNGameMode)
		{
			DFNPlayerController = DFNPlayerController == nullptr ? Cast<ADFNPlayerController>(Controller) : DFNPlayerController;
			ADFNPlayerController* AttackerController = Cast<ADFNPlayerController>(InstigatorController);
			DFNGameMode->PlayerEliminated(Cast<ADFNCharacter>(DamagedActor), DFNPlayerController, AttackerController);
		}
	}
}

void ADFNCharacter::Elimination()
{
	if(CombatComp && CombatComp->EquippedWeapon)
	{
		CombatComp->EquippedWeapon->DropWeapon();
	}
	if(HasAuthority())
	{
		MulticastElimination();
	}
	MulticastElimination();
	GetWorldTimerManager().SetTimer(EliminationTimer, this, &ADFNCharacter::EliminationTimerFinished, EliminationDelay);
}

void ADFNCharacter::MulticastElimination_Implementation()
{
	if(DFNPlayerController)
	{
		DFNPlayerController->UpdateHUDWeaponAmmo(0); // reset ammo!
	}
	bEliminated = true;
	PlayEliminationMontage();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if(DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();
	bDisableGameplay = true;
	if(CombatComp)
	{
		CombatComp->FireButtonPressed(false);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADFNCharacter::PlayEliminationMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
	}
}

void ADFNCharacter::EliminationTimerFinished()
{
	ADFNGameMode* DFNGameMode = GetWorld()->GetAuthGameMode<ADFNGameMode>();
	if(DFNGameMode)
	{
		DFNGameMode->RequestRespawn(this, Controller);
	}
}

void ADFNCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if(DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void ADFNCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &ADFNCharacter::UpdateDissolveMaterial);
	if(DissolveCurve && DissolveTimeLine)
	{
		DissolveTimeLine->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeLine->Play();
	}
}

void ADFNCharacter::UpdateHUDHealth()
{
	DFNPlayerController = DFNPlayerController == nullptr ? Cast<ADFNPlayerController>(Controller) : DFNPlayerController;
	if(DFNPlayerController)
	{
		DFNPlayerController->UpdatePlayerHealth(Health, MaxHealth);
	}
}

void ADFNCharacter::PollInit()
{
	if (DFNPlayerState == nullptr)
	{
		DFNPlayerState = GetPlayerState<ADFNPlayerState>();
		if (DFNPlayerState)
		{
			DFNPlayerState->AddToScore(0.f);
			DFNPlayerState->AddToDefeats(0);
		}
	}
}

ECombatState ADFNCharacter::GetCombatState() const
{
	if (CombatComp == nullptr) return ECombatState::ECS_MAX;
	return CombatComp->CombatState;
}

FVector ADFNCharacter::GetHitTarget() const
{
	if(CombatComp == nullptr) return FVector();
	return CombatComp->HitTarget;
}
