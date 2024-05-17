// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "DFNProjectileMovementComponent.h"

#include "DefeatNoMore/Weapons/Projectiles/Projectile.h"
#include "RocketProjectile.generated.h"

/**
 * 
 */
UCLASS()
class DEFEATNOMORE_API ARocketProjectile : public AProjectile
{
	GENERATED_BODY()

public:

	ARocketProjectile();
	
protected:
	
	/**
	 * @brief - Projectile Hit Event
	 * @param HitComp - Component that causes the hit (our projectile)
	 * @param OtherActor -the actor that was hit
	 * @param OtherComp - the other comp that was hit
	 * @param NormalImpulse - the impulse, kinda perpendicular
	 * @param Hit - FHitResult with the storaged infos
	 */
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit) override;



	/* Rocket Damage Radius properties */
	UPROPERTY(EditDefaultsOnly)
	float InnerRadius = 200.f;
	UPROPERTY(EditDefaultsOnly)
	float OuterRadius = 500.f;
	UPROPERTY(EditDefaultsOnly)
	float DamageFallOff = 1.f;

	UPROPERTY(VisibleAnywhere)
	UDFNProjectileMovementComponent* RocketProjectileMovementComponent;
};
