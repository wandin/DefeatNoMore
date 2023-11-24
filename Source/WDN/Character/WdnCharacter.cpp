#include "WdnCharacter.h"

#include <Windows/DirectX/include/d3d12sdklayers.h>

#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "WDN/Components/CombatComponent.h"
#include "WDN/Weapons/Weapon.h"
#include "WDN/WDN.h"
#include "WDN/PlayerController/WDNPlayerController.h"

AWdnCharacter::AWdnCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArm->SetupAttachment(GetMesh(), FName("head"));
	const FVector SpringArmLocation(5.f, 18.f, 0.f);
	const FRotator SpringArmRotation(0.f, 90.f, -90.f);
	SpringArm->SetRelativeLocationAndRotation(SpringArmLocation, SpringArmRotation);
	SpringArm->SetRelativeLocationAndRotation(SpringArmLocation, SpringArmRotation);
	
	// First Person camera - attached to mesh(head socket), location and rotation set.
	FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->AttachToComponent(SpringArm, FAttachmentTransformRules::KeepRelativeTransform);
	FirstPersonCamera->bUsePawnControlRotation = true;
	
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
	GetCharacterMovement()->bOrientRotationToMovement = false;	
	
	NetUpdateFrequency = 128.f;
	MinNetUpdateFrequency = 64.f;
}

void AWdnCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AWdnCharacter, OverlappingWeapon);
	DOREPLIFETIME(AWdnCharacter, bWalking);
	DOREPLIFETIME(AWdnCharacter, bIsFiring);
	DOREPLIFETIME(AWdnCharacter, Health);
}

void AWdnCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if(IsValid(CombatComp))
	{
		CombatComp->WdnCharacter = this;
	}
}

void AWdnCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	WdnPlayerController = Cast<AWDNPlayerController>(Controller);
	if(WdnPlayerController)
	{
		if(UEnhancedInputLocalPlayerSubsystem* EIP  = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(WdnPlayerController->GetLocalPlayer()))
		{
			EIP->AddMappingContext(DefaultMappingContext, 0);
		}
		EnableInput(WdnPlayerController);
		
		WdnPlayerController->UpdatePlayerHealth(Health, MaxHealth);
	}
	if(HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AWdnCharacter::ReceiveDamage);
	}
	UpdateHUDHealth();
}

void AWdnCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// if(GetLocalRole() > ROLE_SimulatedProxy && IsLocallyControlled()) // same as IsLocallyControlled
	// {
	// 	AimOffset(DeltaSeconds);
	// }
	// else
	// {
	// 	// TimeSinceLastMovementReplication += DeltaSeconds;
	// 	// if(TimeSinceLastMovementReplication > 0.03f)
	// 	// {
	// 	// 	OnRep_ReplicatedMovement();
	// 	// }
	// 	TurnInPlace(DeltaSeconds);
	//
	// 	CalculateAO_Pitch();
	// }
	AimOffset(DeltaSeconds);
}

void AWdnCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if(UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(PlayerInputComponent))
	{
		//Jump
		EIC->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AWdnCharacter::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		//Move
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWdnCharacter::Move);
		// Walk (not running / no footstep sounds)
		EIC->BindAction(WalkAction, ETriggerEvent::Triggered, this, &AWdnCharacter::WalkPressed);
		EIC->BindAction(WalkAction, ETriggerEvent::Completed, this, &AWdnCharacter::WalkReleased);
		//Look
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWdnCharacter::Look);
		//Crouch
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AWdnCharacter::CrouchPressed);
		// Aim - ZoomFOV
		EIC->BindAction(AimAction, ETriggerEvent::Triggered, this, &AWdnCharacter::AimButtonPressed);
		EIC->BindAction(AimAction, ETriggerEvent::Completed, this, &AWdnCharacter::AimButtonReleased);
		//Fire
		EIC->BindAction(FireAction, ETriggerEvent::Triggered, this, &AWdnCharacter::FireButtonPressed);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AWdnCharacter::FireButtonReleased);
	}
}

void AWdnCharacter::Move(const FInputActionValue& Value)
{
	// AddMovementInput takes a Vector
	const FVector2d MovementVector = Value.Get<FVector2d>();
	if(IsValid(Controller))
	{
		AddMovementInput(GetActorForwardVector(), MovementVector.Y);
		AddMovementInput(GetActorRightVector(), MovementVector.X);
	}
}

void AWdnCharacter::Look(const FInputActionValue& Value)
{
	// AddControllerPitchInput takes a Vector
	const FVector2d LookAxisVector = Value.Get<FVector2d>();
	float Pitch = LookAxisVector.Y;
	if(IsValid(Controller))
	{
		APlayerController* PC = Cast<APlayerController>(GetController());
		PC->AddPitchInput(LookAxisVector.Y);
		AddControllerYawInput(LookAxisVector.X);
	}
}

void AWdnCharacter::Jump()
{
	if(bIsCrouched)
	{
		UnCrouch();
	}
	Super::Jump();
}

void AWdnCharacter::CrouchPressed()
{
	if(bIsCrouched)
	{
		UnCrouch();
	}
	Crouch();
}

void AWdnCharacter::WalkPressed()
{
	if(IsLocallyControlled())
	{
		ServerWalkPressed();
	}
	bWalking = true;
	GetCharacterMovement()->MaxWalkSpeed = 220.f;
}

void AWdnCharacter::WalkReleased()
{
	if(IsLocallyControlled())
	{
		ServerWalkReleased();
	}
	bWalking = false;
	GetCharacterMovement()->MaxWalkSpeed = 420.f;
}

void AWdnCharacter::ServerWalkPressed_Implementation()
{
	bWalking = true;
	GetCharacterMovement()->MaxWalkSpeed = 220.f;
}

void AWdnCharacter::ServerWalkReleased_Implementation()
{
	bWalking = false;
	GetCharacterMovement()->MaxWalkSpeed = 420.f;
}

float AWdnCharacter::CalculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void AWdnCharacter::OnRep_Health()
{
}

void AWdnCharacter::AimOffset(float DeltaSeconds)
{
	if (CombatComp && CombatComp->EquippedWeapon == nullptr) return;
	const float Speed = CalculateSpeed();
	const bool bIsInAir = GetCharacterMovement()->IsFalling();
	
	//	Yaw
	if(Speed == 0.f && !bIsInAir)
	{
		bRotateRootBone = true;
		
		const FRotator CurrentAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		const FRotator DeltaAimRotation = UKismetMathLibrary::NormalizedDeltaRotator(StartingAimRotation, CurrentAimRotation);
		AO_Yaw = DeltaAimRotation.Yaw;
		if (TurningInPlace == ETurnInPlace::ETIP_NotTurning)
		{
			InterpAO_Yaw = AO_Yaw;
		}
	}
	if (Speed > 0.f || bIsInAir) // running, or jumping
	{
		bRotateRootBone = false;
		StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		AO_Yaw = 0.f;
		TurningInPlace = ETurnInPlace::ETIP_NotTurning;
	}

	TurnInPlace(DeltaSeconds);
	// Pitch
	CalculateAO_Pitch();
}

void AWdnCharacter::CalculateAO_Pitch()
{
	AO_Pitch = GetBaseAimRotation().Pitch;
	if(AO_Pitch > 90.f && !IsLocallyControlled())
	{
		const FVector2d InRange(270.f, 360.f);
		const FVector2d OutRange(-90.f, 0.f);
		AO_Pitch = FMath::GetMappedRangeValueClamped(InRange, OutRange, AO_Pitch);
	}
}

void AWdnCharacter::SimProxiesTurn()
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

void AWdnCharacter::OnRep_ReplicatedMovement()
{
	Super::OnRep_ReplicatedMovement();
	SimProxiesTurn();
	TimeSinceLastMovementReplication = 0.0f;
}

void AWdnCharacter::EquipWeaponOnCombatComponent()
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

void AWdnCharacter::ServerEquipWeaponOnCombatComponent()
{
	if(IsValid(CombatComp))
	{
		CombatComp->EquipWeapon(OverlappingWeapon);
	}
}

void AWdnCharacter::TurnInPlace(float DeltaSeconds)
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
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaSeconds, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 45.f)
		{
			TurningInPlace = ETurnInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void AWdnCharacter::SetOverlappingWeapon(AWeapon* Weapon)
{
	OverlappingWeapon = Weapon;
	if(IsLocallyControlled() && HasAuthority())
	{
		if(OverlappingWeapon && CombatComp)
		{
			// Equip Weapon
			EquipWeaponOnCombatComponent();
		}
	}
	if(OverlappingWeapon && CombatComp)
	{
		// Equip Weapon
		ServerEquipWeaponOnCombatComponent();
	}
}

AWeapon* AWdnCharacter::GetEquippedWeapon()
{
	if(!IsValid(CombatComp)) return nullptr;
	return CombatComp->EquippedWeapon;
}

void AWdnCharacter::AimButtonPressed()
{
	if(CombatComp)
	{
		CombatComp->SetAiming(true);
	}
}

void AWdnCharacter::AimButtonReleased()
{
	if(CombatComp)
	{
		CombatComp->SetAiming(false);
	}
}

bool AWdnCharacter::bIsAiming()
{
	return (CombatComp && CombatComp->bAiming);	
}
void AWdnCharacter::FireButtonPressed()
{
	if(CombatComp)
	{
		CombatComp->FireButtonPressed(true);
	}
}

void AWdnCharacter::FireButtonReleased()
{
	if(CombatComp)
	{
		CombatComp->FireButtonPressed(false);
	}
}

void AWdnCharacter::PlayFireMontage(bool bFiring)
{
	if(!IsValid(CombatComp) || !IsValid(CombatComp->EquippedWeapon)) return;

	bIsFiring = bFiring;
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && FireWeaponMontage && bIsFiring)
	{
		AnimInstance->Montage_Play(FireWeaponMontage);
	}
}

void AWdnCharacter::PlayHitReactMontage()
{
	if(!IsValid(CombatComp) || !IsValid(CombatComp->EquippedWeapon)) return;
	
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && HitReactMontage)
	{
		AnimInstance->Montage_Play(HitReactMontage);
	}
}

void AWdnCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();
}

void AWdnCharacter::UpdateHUDHealth()
{
	WdnPlayerController = WdnPlayerController == nullptr ? Cast<AWDNPlayerController>(Controller) : WdnPlayerController;
	if(WdnPlayerController)
	{
		WdnPlayerController->UpdatePlayerHealth(Health, MaxHealth);
	}
}

FVector AWdnCharacter::GetHitTarget() const
{
	if(CombatComp == nullptr) return FVector();
	return CombatComp->HitTarget;
}
