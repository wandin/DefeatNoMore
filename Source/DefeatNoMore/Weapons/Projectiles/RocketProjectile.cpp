// Fill out your copyright notice in the Description page of Project Settings.


#include "RocketProjectile.h"

#include "Components/StaticMeshComponent.h"

#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"

#include "Kismet/GameplayStatics.h"

ARocketProjectile::ARocketProjectile()
{
	RocketMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RocketMesh"));
	RocketMesh->SetupAttachment(RootComponent);
	RocketMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ARocketProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
                              FVector NormalImpulse, const FHitResult& Hit)
{

	APawn* FiringPawn =  GetInstigator();
	if(FiringPawn)
	{
		AController* FiringController = FiringPawn->GetController();
		if(FiringController)
		{
			UGameplayStatics::ApplyRadialDamageWithFalloff(
				this,
				Damage,
				10.f,
				GetActorLocation(),
				InnerRadius,
				OuterRadius,
				DamageFallOff,
				UDamageType::StaticClass(), // <-- damage type
				TArray<AActor*>(),
				this,
				FiringController); // <-- instigatorController
		}
	}
	Super::OnHit(HitComp, OtherActor, OtherComp, NormalImpulse, Hit);
}
