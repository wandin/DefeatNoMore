// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Projectile.generated.h"


class UBoxComponent;
class UProjectileMovementComponent;
class UParticleSystem;
class UParticleSystemComponent;
class USoundCue;

UCLASS()
class DEFEATNOMORE_API AProjectile : public AActor
{
	GENERATED_BODY()
	
public:	
	AProjectile();
	virtual void Tick(float DeltaTime) override;

	virtual void Destroyed() override; 

protected:
	virtual void BeginPlay() override;

	/**
	 * @brief - Projectile Hit Event
	 * @param HitComp - Component that causes the hit (our projectile)
	 * @param OtherActor -the actor that was hit
	 * @param OtherComp - the other comp that was hit
	 * @param NormalImpulse - the impulse, kinda perpendicular
	 * @param Hit - FHitResult with the storaged infos
	 */
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	UPROPERTY(EditAnywhere)
	float Damage = 30.f;
private:

	// Projectile CollisionBox and ProjectileMovementComponent, set in blueprints.
	UPROPERTY(EditAnywhere)
	UBoxComponent* CollisionBox;
	UPROPERTY(VisibleAnywhere)
	UProjectileMovementComponent* ProjectileMovementComponent;
	// Tracer and component, to be spawned
	UPROPERTY(EditAnywhere)
	UParticleSystem* Tracer;
	UPROPERTY()
	UParticleSystemComponent* TracerComponent;
	// impact FX's to be spawned when projectile hits something.
	UPROPERTY(EditAnywhere)
	UParticleSystem* ImpactParticles;
	UPROPERTY(EditAnywhere)
	USoundCue* ImpactSound;
};
