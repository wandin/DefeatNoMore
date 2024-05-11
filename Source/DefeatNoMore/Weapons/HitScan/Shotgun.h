// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DefeatNoMore/Weapons/HitScan/HitScanWeapon.h"
#include "Shotgun.generated.h"

/**
 * 
 */
UCLASS()
class DEFEATNOMORE_API AShotgun : public AHitScanWeapon
{
	GENERATED_BODY()


	
public:

	virtual void Fire(const FVector& HitTarget) override;



	
private:

	UPROPERTY(EditAnywhere)
	uint32 ShotgunSpreadCount = 8;
	
	UPROPERTY(EditAnywhere)
	float ShotgunDamage = 15.f;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ShotgunImpactParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ShotgunBeamParticles;

	UPROPERTY(EditAnywhere)
	UParticleSystem* ShotgunMuzzleFlash;

	UPROPERTY(EditAnywhere)
	USoundCue* ShotgunFireSound;

	UPROPERTY(EditAnywhere)
	USoundCue* ShotgunImpactSound;
};
