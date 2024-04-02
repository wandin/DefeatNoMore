#include "DFNCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "DefeatNoMore/PlayerState/DFNPlayerState.h"
#include "DefeatNoMore/Components/CombatComponent.h"
#include "DefeatNoMore/Weapons/Weapon.h"
#include "DefeatNoMore/DefeatNoMore.h"
#include "DefeatNoMore/GameModes/DFNGameMode.h"
#include "DefeatNoMore/PlayerController/DFNPlayerController.h"

ADFNCharacter::ADFNCharacter()
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

	DissolveTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeLineComponent"));
}

void ADFNCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ADFNCharacter, OverlappingWeapon);
	DOREPLIFETIME(ADFNCharacter, bWalking);
	DOREPLIFETIME(ADFNCharacter, bIsFiring);
	DOREPLIFETIME(ADFNCharacter, Health);
}

void ADFNCharacter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if(IsValid(CombatComp))
	{
		CombatComp->DFNCharacter = this;
	}
}

void ADFNCharacter::BeginPlay()
{
	Super::BeginPlay();

	UpdateHUDHealth();
	if(HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &ADFNCharacter::ReceiveDamage);
	}
}

void ADFNCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	/* I commented it out but don't recall the whole thing exactly -.-, will come back to this later
	 * better keep it, it relates to the character turninPlace animations, to be fixed
	 * it works better WITHOUT this extra logic around, but not perfect as I want.
	 * we must go with procedural animations - DFN
	 */
	
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
	PollInit();
}

void ADFNCharacter::Jump()
{
	if(bIsCrouched)
	{
		UnCrouch();
	}
	Super::Jump();
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
		InterpAO_Yaw = FMath::FInterpTo(InterpAO_Yaw, 0.f, DeltaSeconds, 4.f);
		AO_Yaw = InterpAO_Yaw;
		if (FMath::Abs(AO_Yaw) < 45.f)
		{
			TurningInPlace = ETurnInPlace::ETIP_NotTurning;
			StartingAimRotation = FRotator(0.f, GetBaseAimRotation().Yaw, 0.f);
		}
	}
}

void ADFNCharacter::SetOverlappingWeapon(AWeapon* Weapon)
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

AWeapon* ADFNCharacter::GetEquippedWeapon() const
{
	if(!IsValid(CombatComp)) return nullptr;
	return CombatComp->EquippedWeapon;
}

void ADFNCharacter::AimButtonPressed() const
{
	if(CombatComp)
	{
		CombatComp->SetAiming(true);
	}
}

void ADFNCharacter::AimButtonReleased()
{
	if(CombatComp)
	{
		CombatComp->SetAiming(false);
	}
}

bool ADFNCharacter::bIsAiming() const
{
	return (CombatComp && CombatComp->bAiming);	
}
void ADFNCharacter::FireButtonPressed() const
{
	if(CombatComp)
	{
		CombatComp->FireButtonPressed(true);
	}
}

void ADFNCharacter::FireButtonReleased() const
{
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

void ADFNCharacter::PlayReloadMontage()
{
	if (CombatComp == nullptr || CombatComp->EquippedWeapon == nullptr) return;

	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && ReloadMontage)
	{
		AnimInstance->Montage_Play(ReloadMontage);
		FName SectionName;

		switch (CombatComp->EquippedWeapon->GetWeaponType())
		{
		case EWeaponType::EWT_AssaultRifle:
			SectionName = FName("Rifle");
			break;
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
			DFNGameMode->PlayerEliminated(this, DFNPlayerController, AttackerController);
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
	if(DissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance = UMaterialInstanceDynamic::Create(DissolveMaterialInstance, this);
		GetMesh()->SetMaterial(0, DynamicDissolveMaterialInstance);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), 0.55f);
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Glow"), 200.f);
	}
	StartDissolve();
	GetCharacterMovement()->DisableMovement();
	GetCharacterMovement()->StopMovementImmediately();
	if(DFNPlayerController)
	{
		DisableInput(DFNPlayerController);
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

FVector ADFNCharacter::GetHitTarget() const
{
	if(CombatComp == nullptr) return FVector();
	return CombatComp->HitTarget;
}
