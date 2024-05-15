// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DefeatNoMore/Weapons/Weapon.h"
#include "HitScanWeapon.generated.h"

/**
 * 
 */
UCLASS()
class DEFEATNOMORE_API AHitScanWeapon : public AWeapon
{
	GENERATED_BODY()


public:

	virtual void Fire(const FVector& HitTarget) override;

private:

	UPROPERTY(EditAnywhere)
	float Damage = 15.f;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* BeamParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* MuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundCue* FireSound;

	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;


	/* Trace end With Spread */

	UPROPERTY(EditAnywhere, Category = "Weapon Spread")
	float ShotDistance = 800.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Spread")
	float SpreadRadius = 75.f;

	UPROPERTY(EditAnywhere, Category = "Weapon Spread")
	bool bUseSpread = false;
	

protected:

	FVector TraceSpread(const FVector& TraceStart, const FVector& HitTarget);

	void WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit);
};
