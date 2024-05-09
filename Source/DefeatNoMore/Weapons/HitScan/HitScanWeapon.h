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
};
