// Fill out your copyright notice in the Description page of Project Settings.


#include "Shotgun.h"

#include "Components/SkeletalMeshComponent.h"

#include "DefeatNoMore/Character/DFNCharacter.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Engine/World.h"

#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"

#include "Kismet/GameplayStatics.h"

#include "Sound/SoundCue.h"

void AShotgun::Fire(const FVector& HitTarget)
{
	AWeapon::Fire(HitTarget);
	
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if(OwnerPawn == nullptr) return;
	AController* InstigatorController = OwnerPawn->GetController();
	
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(("MuzzleFlash"));
	if(MuzzleFlashSocket)
	{
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		FVector Start = SocketTransform.GetLocation();
		uint32 Hits = 0;

		TMap<ADFNCharacter*, uint32> HitMap;
		
		for (uint32 i = 0; i < ShotgunSpreadCount; i++)
		{
			FHitResult FireHit;
			WeaponTraceHit(Start, HitTarget, FireHit);

			ADFNCharacter* DFNCharacter = Cast<ADFNCharacter>(FireHit.GetActor());
			if(DFNCharacter && HasAuthority() && InstigatorController)
			{
				if(HitMap.Contains(DFNCharacter))
				{
					HitMap[DFNCharacter]++;
				}
				else
				{
					HitMap.Emplace(DFNCharacter, 1);
				}
			}
			
			if(ShotgunImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ShotgunImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
			}
			if(ShotgunImpactSound)
			{
				UGameplayStatics::PlaySoundAtLocation(this, ShotgunImpactSound, FireHit.ImpactPoint);
			}
		}

		for(auto HitPair : HitMap)
		{
			if(HitPair.Key && HasAuthority() && InstigatorController)
			{
				UGameplayStatics::ApplyDamage(HitPair.Key, ShotgunDamage * HitPair.Value, InstigatorController, this, UDamageType::StaticClass());
			}
		}
	}
}