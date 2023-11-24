
#include "CombatComponent.h"

#include "Engine/SkeletalMeshSocket.h"

#include "WDN/Character/WdnCharacter.h"
#include "WDN/Weapons/Weapon.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WDN/PlayerController/WDNPlayerController.h"
#include "WDN/HUD/WDNHUD.h"
#include "TimerManager.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bAiming = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	if(WdnCharacter)
	{
		WdnCharacter->GetCharacterMovement()->MaxWalkSpeed = BaseWalkSpeed;

		if(WdnCharacter->GetFirstPersonCamera())
		{
			DefaultFOV = WdnCharacter->GetFirstPersonCamera()->FieldOfView;
			CurrentFOV = DefaultFOV;
		}
	}
}

void UCombatComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UCombatComponent, EquippedWeapon);
	DOREPLIFETIME(UCombatComponent, bAiming);
	DOREPLIFETIME(UCombatComponent, bFiring);
}

void UCombatComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	if(WdnCharacter && WdnCharacter->IsLocallyControlled())
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
	if(!IsValid(WdnCharacter) || !IsValid(WdnCharacter->Controller)) return;
	
	// Set Controller to WdnCharacter->Controller if Controller is null, otherwise Controller = Controller;
	Controller = Controller == nullptr ? Cast<AWDNPlayerController>(WdnCharacter->Controller) : Controller;
	if(Controller)
	{
		// Set HUD to Controller->HUD if HUD is null, otherwise HUD = HUD;
		HUD = HUD == nullptr ? Cast<AWDNHUD>(Controller->GetHUD()) : HUD;
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
			const FVector2d WalkSpeedRange(0.f, WdnCharacter->GetCharacterMovement()->MaxWalkSpeed);
			const FVector2d ClampedSpeed(0.f, 1.f);
			FVector Velocity = WdnCharacter->GetVelocity();
			Velocity.Z = 0.f;

			// we map the character's velocity between 0 and 1, so if character is in Idle = 0, half-speed 0.5, maxspeed = 1
			CrosshairVelocityFactor =  FMath::GetMappedRangeValueClamped(WalkSpeedRange, ClampedSpeed, Velocity.Size());

			// spread while falling/Jumping
			if(WdnCharacter->GetCharacterMovement()->IsFalling()) 
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, 1.15f, DeltaSeconds, 30.f);
			}
			// not in Air - default
			else 
			{
				CrosshairInAirFactor = FMath::FInterpTo(CrosshairInAirFactor, .8f, DeltaSeconds, 30.f);
			}

			// Shrink Crosshairs while Crouching
			if(WdnCharacter->GetCharacterMovement()->IsCrouching()) // shrink while crouching negative (-CrosshairShrinkSpreadFactor)
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
				if(WdnCharacter->GetCharacterMovement()->IsCrouching())
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
	if(WdnCharacter == nullptr || WeaponToEquip == nullptr) return;

	if(IsWeaponEquipped()) return; // returns if a weapon is already equipped
	
	EquippedWeapon = WeaponToEquip;
	EquippedWeapon->SetWeaponState(EWeaponState::EWS_Equipped);
	const USkeletalMeshSocket* HandSocket = WdnCharacter->GetMesh()->GetSocketByName(FName("RightHandSocket"));
	if(HandSocket)
	{
		HandSocket->AttachActor(EquippedWeapon, WdnCharacter->GetMesh());
	}
	EquippedWeapon->SetOwner(WdnCharacter);
	bIsWeaponEquipped = true;
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
		if(WdnCharacter)
		{
			// Extend the Start of our LineTrace - a bit in front of our character to not trace ourself
			float DistanceFromCharacterToLineTrace = (WdnCharacter->GetActorLocation() - Start).Size();
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
	if(WdnCharacter && WdnCharacter->GetFirstPersonCamera())
	{
		WdnCharacter->GetFirstPersonCamera()->SetFieldOfView(CurrentFOV);
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
	if(bCanFire)
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

void UCombatComponent::StartFireTimer()
{
	if (EquippedWeapon == nullptr || WdnCharacter == nullptr) return;
	WdnCharacter->GetWorldTimerManager().SetTimer(
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
	if(!IsValid(WdnCharacter)) return;
	if(WdnCharacter)
	{
		WdnCharacter->PlayFireMontage(bFiring);
		EquippedWeapon->Fire(TraceHitTarget);
		UE_LOG(LogTemp, Warning, TEXT("FIRING!"));
	}
}