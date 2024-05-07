// Fill out your copyright notice in the Description page of Project Settings.

#include "ProjectileWeapon.h"

#include "Components/SkeletalMeshComponent.h"

#include "Engine/SkeletalMeshSocket.h"
#include "DefeatNoMore/Weapons/Projectiles/Projectile.h"

#include "Engine/World.h"

#include "GameFramework/Pawn.h"


void AProjectileWeapon::Fire(const FVector& HitTarget)
{
	Super::Fire(HitTarget);

	if(!HasAuthority()) return;
	
	// Instigator to our SpawnParams, an APawn casted from our weapon's owner
	APawn* InstigatorPawn = Cast<APawn>(GetOwner());
	
	// Skeletal Mesh - MWeapon Muzzle Flash socket
	const USkeletalMeshSocket* MuzzleFlashSocket = GetWeaponMesh()->GetSocketByName(FName("MuzzleFlash"));
	if(MuzzleFlashSocket)
	{
		// Socket transform
		FTransform SocketTransform = MuzzleFlashSocket->GetSocketTransform(GetWeaponMesh());
		// we'll get our projectile direction from this ToTarget vector
		const FVector ToTarget = HitTarget - SocketTransform.GetLocation(); // From MuzzleFlashSocket to hit location - doing it on TraceUnderCrosshairs function
		FRotator TargetRotation = ToTarget.Rotation(); // our Projectile Direction as mentioned prior
		if(ProjectileCLass && InstigatorPawn)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.Owner = GetOwner();
			SpawnParams.Instigator = InstigatorPawn;
			UWorld* World = GetWorld();
			if(World)
			{
				//Spawn Actor
				World->SpawnActor<AProjectile>(ProjectileCLass, SocketTransform.GetLocation(), TargetRotation, SpawnParams);
			}
		}
	}
}
