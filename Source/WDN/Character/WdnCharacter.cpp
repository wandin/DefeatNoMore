#include "WdnCharacter.h"

#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Net/UnrealNetwork.h"
#include "WDN/PlayerState/WDNPlayerState.h"
#include "WDN/Components/CombatComponent.h"
#include "WDN/Weapons/Weapon.h"
#include "WDN/WDN.h"
#include "WDN/GameModes/WdnGameMode.h"
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

	DissolveTimeLine = CreateDefaultSubobject<UTimelineComponent>(TEXT("DissolveTimeLineComponent"));
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
		
	UpdateHUDHealth();
	if(HasAuthority())
	{
		OnTakeAnyDamage.AddDynamic(this, &AWdnCharacter::ReceiveDamage);
	}
}

void AWdnCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	/* I commented it out but don't recall the whole thing exactly -.-, will come back to this later
	 * better keep it, it relates to the character turninPlace animations, to be fixed
	 * it works better WITHOUT this extra logic around, but not perfect as I want.
	 * we must go with procedural animations - WdN
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

void AWdnCharacter::Jump()
{
	if(bIsCrouched)
	{
		UnCrouch();
	}
	Super::Jump();
}

float AWdnCharacter::CalculateSpeed() const
{
	FVector Velocity = GetVelocity();
	Velocity.Z = 0.f;
	return Velocity.Size();
}

void AWdnCharacter::OnRep_Health()
{
	UpdateHUDHealth();
	PlayHitReactMontage();
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

bool AWdnCharacter::bIsAiming() const
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
		if(!IsLocallyControlled())
		{
			AnimInstance->Montage_Play(HitReactMontage);
		}
	}
}

void AWdnCharacter::ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
	AController* InstigatorController, AActor* DamageCauser)
{
	Health = FMath::Clamp(Health - Damage, 0.f, MaxHealth);
	UpdateHUDHealth();
	PlayHitReactMontage();

	if(Health == 0.f)
	{
		AWdnGameMode* WdnGameMode = GetWorld()->GetAuthGameMode<AWdnGameMode>();
		if(WdnGameMode)
		{
			WdnPlayerController = WdnPlayerController == nullptr ? Cast<AWDNPlayerController>(Controller) : WdnPlayerController;
			AWDNPlayerController* AttackerController = Cast<AWDNPlayerController>(InstigatorController);
			WdnGameMode->PlayerEliminated(this, WdnPlayerController, AttackerController);
		}
	}
}

void AWdnCharacter::Elimination()
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
	GetWorldTimerManager().SetTimer(EliminationTimer, this, &AWdnCharacter::EliminationTimerFinished, EliminationDelay);
}

void AWdnCharacter::MulticastElimination_Implementation()
{
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
	if(WdnPlayerController)
	{
		DisableInput(WdnPlayerController);
	}
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AWdnCharacter::PlayEliminationMontage()
{
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if(AnimInstance && DeathMontage)
	{
		AnimInstance->Montage_Play(DeathMontage);
	}
}

void AWdnCharacter::EliminationTimerFinished()
{
	AWdnGameMode* WdnGameMode = GetWorld()->GetAuthGameMode<AWdnGameMode>();
	if(WdnGameMode)
	{
		WdnGameMode->RequestRespawn(this, Controller);
	}
}

void AWdnCharacter::UpdateDissolveMaterial(float DissolveValue)
{
	if(DynamicDissolveMaterialInstance)
	{
		DynamicDissolveMaterialInstance->SetScalarParameterValue(TEXT("Dissolve"), DissolveValue);
	}
}

void AWdnCharacter::StartDissolve()
{
	DissolveTrack.BindDynamic(this, &AWdnCharacter::UpdateDissolveMaterial);
	if(DissolveCurve && DissolveTimeLine)
	{
		DissolveTimeLine->AddInterpFloat(DissolveCurve, DissolveTrack);
		DissolveTimeLine->Play();
	}
}

void AWdnCharacter::UpdateHUDHealth()
{
	WdnPlayerController = WdnPlayerController == nullptr ? Cast<AWDNPlayerController>(Controller) : WdnPlayerController;
	if(WdnPlayerController)
	{
		WdnPlayerController->UpdatePlayerHealth(Health, MaxHealth);
	}
}

void AWdnCharacter::PollInit()
{
	if (WDNPlayerState == nullptr)
	{
		WDNPlayerState = GetPlayerState<AWDNPlayerState>();
		if (WDNPlayerState)
		{
			WDNPlayerState->AddToScore(0.f);
			WDNPlayerState->AddToDefeats(0);
		}
	}
}

FVector AWdnCharacter::GetHitTarget() const
{
	if(CombatComp == nullptr) return FVector();
	return CombatComp->HitTarget;
}
