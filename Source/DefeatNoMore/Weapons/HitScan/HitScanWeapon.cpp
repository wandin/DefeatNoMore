// Fill out your copyright notice in the Description page of Project Settings.


#include "DefeatNoMore/Weapons/HitScan/HitScanWeapon.h"

#include "Components/SkeletalMeshComponent.h"

#include "DefeatNoMore/Character/DFNCharacter.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Engine/World.h"

#include "GameFramework/DamageType.h"

#include "Kismet/GameplayStatics.h"

#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"


void AHitScanWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if(OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(("MuzzleFlash"));
	if(MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());

		FVector Start = SocketTransform.GetLocation();
		FVector End = Start + (HitTarget - Start) * 10;

		FHitResult FireHit;
		UWorld* World = GetWorld();
		if (World)
		{
			World->LineTraceSingleByChannel(FireHit, Start, End, ECollisionChannel::ECC_Visibility);

			FVector BeamEnd = End;
			if(FireHit.bBlockingHit)
			{
				BeamEnd = FireHit.ImpactPoint;
				ADFNCharacter* DFNCharacter = Cast<ADFNCharacter>(FireHit.GetActor());
				if(DFNCharacter && HasAuthority() && InstigatorController)
				{
					UGameplayStatics::ApplyDamage(DFNCharacter, Damage, InstigatorController, this, UDamageType::StaticClass());
				}
				if(ImpactParticles)
				{
					UGameplayStatics::SpawnEmitterAtLocation(World, ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
				}
			}
			if(BeamParticles)
			{
				UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(World, BeamParticles, SocketTransform);
				if(Beam)
				{
					Beam->SetVectorParameter(FName("Target"), BeamEnd);
				}
			}
		}
	}
}
