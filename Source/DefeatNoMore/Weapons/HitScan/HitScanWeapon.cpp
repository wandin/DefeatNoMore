// Fill out your copyright notice in the Description page of Project Settings.


#include "DefeatNoMore/Weapons/HitScan/HitScanWeapon.h"

#include "DrawDebugHelpers.h"

#include "Components/SkeletalMeshComponent.h"

#include "DefeatNoMore/Character/DFNCharacter.h"

#include "Engine/SkeletalMeshSocket.h"
#include "Engine/World.h"

#include "GameFramework/DamageType.h"

#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"

#include "Sound/SoundCue.h"


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
				if(ImpactSound)
				{
					UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, FireHit.ImpactPoint);
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
		if(MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(World, MuzzleFlash, SocketTransform);
		}
		if(FireSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
		}
	}
}

FVector AHitScanWeapon::TraceSpread(const FVector& TraceStart, const FVector& HitTarget)
{
	FVector ToTargetNormalized = (HitTarget - TraceStart).GetSafeNormal();
	FVector SphereCenter = TraceStart + ToTargetNormalized * DistanceToSphere;
	FVector RandVector = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SphereRadius);
	FVector EndLocation = SphereCenter + RandVector;
	FVector ToEndLocation = EndLocation - TraceStart;
	DrawDebugSphere(GetWorld(), SphereCenter, SphereRadius, 12, FColor::Red, true);

	DrawDebugSphere(GetWorld(), EndLocation, 5.f, 12, FColor::Blue, true);
	DrawDebugLine(GetWorld(), TraceStart, TraceStart + ToEndLocation * TRACE_LENGTH / ToEndLocation.Size(), FColor::Green, true);
	return FVector(TraceStart + ToEndLocation + TRACE_LENGTH / ToEndLocation.Size());
}
