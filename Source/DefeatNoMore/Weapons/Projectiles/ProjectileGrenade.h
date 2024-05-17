// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DefeatNoMore/Weapons/Projectiles/Projectile.h"
#include "ProjectileGrenade.generated.h"

/**
 * 
 */
UCLASS()
class DEFEATNOMORE_API AProjectileGrenade : public AProjectile
{
	GENERATED_BODY()

public:

	AProjectileGrenade();
	virtual void Destroyed() override;
protected:

	virtual void BeginPlay() override;

	UFUNCTION()
	void OnBounce(const FHitResult& ImpactResult, const FVector& ImpactVelocity);

	/* Rocket Damage Radius properties */
	UPROPERTY(EditDefaultsOnly)
	float InnerRadius = 200.f;
	UPROPERTY(EditDefaultsOnly)
	float OuterRadius = 500.f;
	UPROPERTY(EditDefaultsOnly)
	float DamageFallOff = 1.f;

private:

	UPROPERTY(EditAnywhere)
	USoundCue* BounceSound;
};
