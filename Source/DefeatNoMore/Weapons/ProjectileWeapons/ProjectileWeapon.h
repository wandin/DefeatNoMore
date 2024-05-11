// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DefeatNoMore/Weapons/Weapon.h"
#include "ProjectileWeapon.generated.h"

/**
 * 
 */
UCLASS()
class DEFEATNOMORE_API AProjectileWeapon : public AWeapon
{
	GENERATED_BODY()

public:

	virtual void Fire(const FVector& HitTarget) override;

private:

	// to be populated with our different projectiles
	UPROPERTY(EditAnywhere)
	TSubclassOf<class AProjectile> ProjectileCLass;	
};
