// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "WDN/HUD/WDNHUD.h"
#include "CombatComponent.generated.h"

#define TRACE_LENGTH 30000.f // define our trace length

class AWdnCharacter;
class AWDNPlayerController;
class AWDNHUD;
class AWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class WDN_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	friend class AWdnCharacter;

	void EquipWeapon(AWeapon* WeaponToEquip);

protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);
	
	// FireButtonPressed calls ServerFire in order to fire weapons
	void FireButtonPressed(bool bPressed);
	
	/**
	 * @brief Server Fire calls MulticastFire, so it's replicated to all clients. From server to client
	 * as calling Multicast from client doesn't work, we need to call multicast from the server
	 * so on FireButtonPressed we call ServerFire, which calls MulticasFire, all clients receive the fire infos
	 * @param TraceHitTarget 
	 */
	UFUNCTION(Server, Reliable)
	void ServerFire(const FVector_NetQuantize& TraceHitTarget);
	UFUNCTION(NetMulticast, Reliable)
	void MulticastFire(const FVector_NetQuantize& TraceHitTarget);

	/**
	 * @brief Trace a line in the center of screen, our crosshair location is centered in our viewport
	 * @param TraceHitResult 
	 */
	void TraceUnderCrosshairs(FHitResult& TraceHitResult);

	void SetHUDCrosshairs(float DeltaSeconds);

	UFUNCTION()
	void OnRep_EquippedWeapon();
	
private:

	UPROPERTY()
	AWdnCharacter* WdnCharacter;
	UPROPERTY()
	AWDNPlayerController* Controller;
	UPROPERTY()
	AWDNHUD* HUD;
	
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

	bool bIsWeaponEquipped;
	bool IsWeaponEquipped() const;

	UPROPERTY(EditAnywhere)
	float BaseWalkSpeed;
	
	UPROPERTY(Replicated)
	bool bAiming;
	
	bool bFireButtonPressed;
	UPROPERTY(Replicated) 
	bool bFiring;

	/*
	 * HUD and Crosshairs
	 */
	float CrosshairVelocityFactor;
	float CrosshairInAirFactor;
	float CrosshairCrouchFactor;
	float CrosshairAimFactor;
	float CrosshairShootingFactor;
	float CrosshairSpreadShrinkFactor;

	FVector HitTarget;
	
	FHUDPackage HUDPackage;

	/*
	 * Aiming and FOV
	 */

	// FOV when not aiming, ours camera's fov
	float DefaultFOV;
	float CurrentFOV;
	
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomedFOV = 30.f;
	UPROPERTY(EditAnywhere, Category = Combat)
	float ZoomInterpSpeed = 20.f;
	
	void InterpFOV(float DeltaSeconds);

	/*
	 * Automatic fire
	 */
	FTimerHandle FireTimer;
	
	bool bCanFire = true;
	void StartFireTimer();
	void FireTimerFinished();
	void Fire();
};
