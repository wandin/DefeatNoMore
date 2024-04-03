
#include "CombatComponent.h"

#include "Engine/SkeletalMeshSocket.h"

#include "DefeatNoMore/Character/DFNCharacter.h"
#include "DefeatNoMore/Weapons/Weapon.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DefeatNoMore/PlayerController/DFNPlayerController.h"
#include "DefeatNoMore/HUD/DFNHUD.h"
#include "TimerManager.h"

#include "DefeatNoMore/Enums/WeaponTypes.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAiming = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if(DFNCharacter)
	{
		DFNCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if(DFNCharacter->GetFirstPersonCamera())
		{
			DefaultFOV = DFNCharacter->GetFirstPersonCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
		if(DFNCharacter->HasAuthority())
		{
			InitializeCarriedAmmo();
		}
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, bFiring);
	DOREPLIFETIME_CONDITION(UCombatComponent, CarriedAmmo, COND_OwnerOnly);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if(DFNCharacter && DFNCharacter->IsLocallyControlled())
	{
		FHitResult HitResult;
		TraceUnderCrosshairs(HitResult);
		HitTarget = HitResult.ImpactPoint;

		SetHUDCrosshairs(DeltaTime); // Crosshairs
		InterpFOV(DeltaTime); // CameraFOV - Aiming/Zooming. 
	}
}

void UCombatComponent::SetHUDCrosshairs(float DeltaSeconds)
{
	// return if not valid.
	if(!IsValid(DFNCharacter) || !IsValid(DFNCharacter->Controller)) return;
	
	// Set Controller to DFNCharacter->Controller if Controller is null, otherwise Controller = Controller;
	Controller = Controller == nullptr ? Cast<ADFNPlayerController>(DFNCharacter->Controller) : Controller;
	if(Controller)
	{
		// Set HUD to Controller->HUD if HUD is null, otherwise HUD = HUD;
		HUD = HUD == nullptr ? Cast<ADFNHUD>(Controller->GetHUD()) : HUD;
		if(HUD)
		{
			if(EquippedWeapon)
			{
				// set our FHUDPackage reference
				HUDPackage.CrosshairsCenter = EquippedWeapon->CrosshairsCenter;
				HUDPackage.CrosshairsRight = EquippedWeapon->CrosshairsRight;
				HUDPackage.CrosshairsLeft =  EquippedWeapon->CrosshairsLeft;
				HUDPackage.CrosshairsTop = EquippedWeapon->CrosshairsTop;
				HUDPackage.CrosshairsBottom = EquippedWeapon->CrosshairsBottom;
			}
			else
			{
				HUDPackage.CrosshairsCenter = nullptr;
				HUDPackage.CrosshairsRight = nullptr;
				HUDPackage.CrosshairsLeft =  nullptr;
				HUDPackage.CrosshairsTop = nullptr;
				HUDPackage.CrosshairsBottom = nullptr;
			}
			// Calculate our crosshairs spread
			const FVector2d WalkSpeedRange(0.f, DFNCharacter->GetCharacterMovement()->MaxWalkSpeed);
			const FVector2d ClampedSpeed(0.f, 1.f);
			FVector Velocity = DFNCharacter->GetVelocity();
			Velocity.Z = 0.f;

			// we map the character's velocity between 0 and 1, so if character is in Idle = 0, half-speed 0.5, maxspeed = 1
			CrosshairVelocityFactor =  FMath::GetMappedRangeValueClamped(WalkSpeedRange, ClampedSpeed, Velocity.Size());

			// spread while falling/Jumping
			if(DFNCharacter->GetCharacterMovement()->IsFalling()) 
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 1.15f, DeltaSeconds, 30.f);
			}
			// not in Air - default
			else 
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, .8f, DeltaSeconds, 30.f);
			}

			// Shrink Crosshairs while Crouching
			if(DFNCharacter->GetCharacterMovement()->IsCrouching()) // shrink while crouching negative (-CrosshairShrinkSpreadFactor)
			{
				CrosshairCrouchFactor = FMath::FInterpTo(CrosshairCrouchFactor, 1.05f, DeltaSeconds, 30.f);
			}
			// not crouching - default
			else
			{
				CrosshairCrouchFactor = FMath::FInterpTo(CrosshairCrouchFactor, .8f, DeltaSeconds, 30.f);
			}
			// Shrink while Aiming
			if(bAiming)
			{
				if(DFNCharacter->GetCharacterMovement()->IsCrouching())
				{
					CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, .85f, DeltaSeconds, 30.f);
				}
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, 1.20f, DeltaSeconds, 30.f);
			}
			// not Aiming - default
			else
			{
				CrosshairAimFactor = FMath::FInterpTo(CrosshairAimFactor, .8f, DeltaSeconds, 30.f);
			}
			//Shooting
			CrosshairShootingFactor = FMath::FInterpTo(CrosshairShootingFactor, 0.f, DeltaSeconds, 30.f);
						
			/*
			* here we set the CrosshairSpread float in HUDPackage struct equals to our CrosshairVelocityFactor calculated above
			* (+) our CrosshairFactor depending on character's action
			* this will be our factor on spreading the crosshair. (by character's velocity, jumping and crouching so far..)
			*/
			HUDPackage.CrosshairSpread = 1.13f + CrosshairVelocityFactor + CrosshairInAirFactor + CrosshairShootingFactor - CrosshairCrouchFactor - CrosshairAimFactor;
			HUD->SetHUDPackage(HUDPackage);
		}
	}
}

void UCombatComponent::EquipWeapon(AWeapon* WeaponToEquip)
{
	if(DFNCharacter == nullptr || WeaponToEquip == nullptr) return;

	if(IsWeaponEquipped()) return; // returns if a weapon is already equipped

	if(EquippedWeapon)
	{
		EquippedWeapon->DropWeapon(); // drop weapon before equipping a new one!
	}
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = DFNCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if(HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, DFNCharacter->GetMesh());
	}
	EquippedWeapon->SetOwner(DFNCharacter);
	EquippedWeapon->SetHUDAmmo();
	bIsWeaponEquipped = true;

	// replicated
	if(CarriedAmmoMap.Contains(EquippedWeapon->GetWeaponType()))
	{
		CarriedAmmo = CarriedAmmoMap[EquippedWeapon->GetWeaponType()];
	}
	
	Controller = Controller == nullptr ? Cast<ADFNPlayerController>(DFNCharacter->Controller) : Controller;
	if(Controller)
	{
		Controller->UpdateHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::Reload()
{
	//play reload animation
	if(CarriedAmmo > 0)
	{
		ServerReload();
	}
}

void UCombatComponent::ServerReload_Implementation()
{
	if(DFNCharacter == nullptr) return;
	DFNCharacter->PlayReloadMontage();
}

void UCombatComponent::OnRep_EquippedWeapon()
{
	if (EquippedWeapon && DFNCharacter)
	{
		EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
		const USkeletalMeshSocket* HandSocket = DFNCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
		if (HandSocket)
		{
			HandSocket->AttachActor(EquippedWeapon, DFNCharacter->GetMesh());
		}
		DFNCharacter->GetCharacterMovement()->bOrientRotationToMovement = false;
		DFNCharacter->bUseControllerRotationYaw = true;
	}
}

bool UCombatComponent::IsWeaponEquipped() const
{
	return bIsWeaponEquipped;
}

void UCombatComponent::TraceUnderCrosshairs(FHitResult& TraceHitResult)
{
	FVector2d ViewPortSize;
	if(GEngine && GEngine->GameViewport)
	{
		// Our crosshair location is centered in our viewport
		GEngine->GameViewport->GetViewportSize(ViewPortSize);
	}
	const FVector2d CrosshairLocation(ViewPortSize.X / 2.f, ViewPortSize.Y / 2.f);
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;
	const bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(
		UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);
	
	if(bScreenToWorld)
	{
		FVector Start = CrosshairWorldPosition; // Line Trace Start
		if(DFNCharacter)
		{
			// Extend the Start of our LineTrace - a bit in front of our character to not trace ourself
			float DistanceFromCharacterToLineTrace = (DFNCharacter->GetActorLocation() - Start).Size();
			Start += CrosshairWorldDirection * (DistanceFromCharacterToLineTrace - 30.f);
		}
		
		// Line trace end (80,000 units).
		const FVector End = Start + CrosshairWorldDirection * TRACE_LENGTH; 

		// our line trace
		GetWorld()->LineTraceSingleByChannel(TraceHitResult, Start, End, ECC_Visibility);

		// Check if actor is valid and if the actor has our UInterfaceCrosshairInteraction implemented on its class
		if(IsValid(TraceHitResult.GetActor()) && TraceHitResult.GetActor()->Implements<UInterfaceCrosshairInteraction>())
		{
			//Sets our crosshair's color
			HUDPackage.CrosshairColor = FLinearColor::Red;
		}
		else
		{
			HUDPackage.CrosshairColor = FLinearColor::White;
		}
		if(!TraceHitResult.bBlockingHit)
		{
			// if no hits in the line trace, we trace hitresult to end point of our linetrace and we store it in HitTarget.
			TraceHitResult.ImpactPoint = End; 
		}
	}
}

void UCombatComponent::SetAiming(bool bIsAiming)
{
	bAiming = bIsAiming;
	ServerSetAiming(bIsAiming);
}

void UCombatComponent::ServerSetAiming_Implementation(bool bIsAiming)
{
	bAiming = bIsAiming;
}

void UCombatComponent::InterpFOV(float DeltaSeconds)
{
	if(!IsValid(EquippedWeapon)) return;

	if(bAiming)
	{
		// zoom
		CurrentFOV = FMath::FInterpTo(CurrentFOV, EquippedWeapon->GetZoomedFOV(), DeltaSeconds, EquippedWeapon->GetZoomInterpSpeed());
	}
	else
	{
		// cancel zoom (not aiming)
		CurrentFOV = FMath::FInterpTo(CurrentFOV, DefaultFOV, DeltaSeconds, ZoomInterpSpeed);
	}
		//Sets the camera FOV
	if(DFNCharacter && DFNCharacter->GetFirstPersonCamera())
	{
		DFNCharacter->GetFirstPersonCamera()->SetFieldOfView(CurrentFOV);
	}
}

void UCombatComponent::FireButtonPressed(bool bPressed)
{
	bFireButtonPressed = bPressed;
   
	if (bFireButtonPressed && EquippedWeapon)
	{
		Fire();
	}
}

void UCombatComponent::Fire()
{
	if(CanFire())
	{
		ServerFire(HitTarget);
		if(EquippedWeapon)
		{
			bCanFire = false;
			CrosshairShootingFactor = .75f;
		}
		StartFireTimer();
	}
}

bool UCombatComponent::CanFire() const
{
	if(EquippedWeapon == nullptr) return false;
	return !EquippedWeapon->IsEmpty() || !bCanFire;
}

void UCombatComponent::OnRep_CarriedAmmo()
{
	Controller = Controller == nullptr ? Cast<ADFNPlayerController>(DFNCharacter->Controller) : Controller;
	if(Controller)
	{
		Controller->UpdateHUDCarriedAmmo(CarriedAmmo);
	}
}

void UCombatComponent::InitializeCarriedAmmo()
{
	//Initializing Ammo for each weapon type
	CarriedAmmoMap.Emplace(EWeaponType::EWT_AssaultRifle, StartingAmmoAR);
}	

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || DFNCharacter == nullptr) return;
	DFNCharacter->GetWorldTimerManager().SetTimer(
		FireTimer,this,&UCombatComponent::FireTimerFinished,EquippedWeapon->FireDelay);
}

void UCombatComponent::FireTimerFinished()
{
	if (EquippedWeapon == nullptr) return;
	bCanFire = true;
	if (bFireButtonPressed && EquippedWeapon->bIsAutomatic)
	{
		Fire();
	}
}

void UCombatComponent::ServerFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	MulticastFire(TraceHitTarget);
}

void UCombatComponent::MulticastFire_Implementation(const FVector_NetQuantize& TraceHitTarget)
{
	if(!IsValid(EquippedWeapon)) return;
	if(!IsValid(DFNCharacter)) return;
	if(DFNCharacter)
	{
		DFNCharacter->PlayFireMontage(bFiring);
		EquippedWeapon->Fire(TraceHitTarget);
	}
}