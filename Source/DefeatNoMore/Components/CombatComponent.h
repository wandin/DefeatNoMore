// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DefeatNoMore/HUD/DFNHUD.h"
#include "DefeatNoMore/Enums/WeaponTypes.h"
#include "DefeatNoMore/Enums/CombatState.h"


#include "CombatComponent.generated.h"

class ADFNCharacter;
class ADFNPlayerController;
class ADFNHUD;
class AWeapon;

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class DEFEATNOMORE_API UCombatComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UCombatComponent();
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	friend class ADFNCharacter;

	void EquipWeapon(AWeapon* WeaponToEquip);

	void Reload();
	UFUNCTION(BlueprintCallable)
	void FinishReloading();
	
	// FireButtonPressed calls ServerFire in order to fire weapons
	void FireButtonPressed(bool bPressed);
	
protected:
	virtual void BeginPlay() override;

	void SetAiming(bool bIsAiming);

	UFUNCTION(Server, Reliable)
	void ServerSetAiming(bool bIsAiming);
	
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

	UFUNCTION(Server, Reliable)
	void ServerReload();

	void HandleReload();

	int32 AmountToReload();
	
	UFUNCTION()
	void OnRep_EquippedWeapon();
	
private:

	UPROPERTY()
	ADFNCharacter* DFNCharacter;
	UPROPERTY()
	ADFNPlayerController* Controller;
	UPROPERTY()
	ADFNHUD* HUD;
	
	UPROPERTY(ReplicatedUsing = OnRep_EquippedWeapon)
	AWeapon* EquippedWeapon;

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

	bool CanFire() const;

	UPROPERTY(ReplicatedUsing = OnRep_CarriedAmmo)
	int32 CarriedAmmo;
	
	UFUNCTION()
	void OnRep_CarriedAmmo();

	TMap<EWeaponType, int32> CarriedAmmoMap;

	UPROPERTY(EditAnywhere)
	int32 StartingPistolAmmo = 0;
	
	UPROPERTY(EditAnywhere)
	int32 StartingSMGAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingShotGunAmmo = 0;
	
	UPROPERTY(EditAnywhere)
	int32 StartingAssaultRifleAmmo = 0;

	UPROPERTY(EditAnywhere)
	int32 StartingRocketAmmo = 0;
		
	void InitializeCarriedAmmo();

	UPROPERTY(ReplicatedUsing = OnRep_CombatState)
	ECombatState CombatState = ECombatState::ECS_Unoccupied;

	UFUNCTION()
	void OnRep_CombatState();

	void UpdateAmmoValues();
};
