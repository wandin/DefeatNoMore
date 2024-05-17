// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketProjectile.h"

#include "DFNProjectileMovementComponent.h"

#include "Components/StaticMeshComponent.h"

#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "Kismet/GameplayStatics.h"

ARocketProjectile::ARocketProjectile()
{
	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	ProjectileMesh->SetupAttachment(RootComponent);
	ProjectileMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	RocketProjectileMovementComponent = CreateDefaultSubobject<UDFNProjectileMovementComponent>(TEXT("RocketMovementComponent"));
	RocketProjectileMovementComponent->bRotationFollowsVelocity = true;
	RocketProjectileMovementComponent->SetIsReplicated(true);
}

void ARocketProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{
	ExplodeDamage(InnerRadius, OuterRadius, DamageFallOff);
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
