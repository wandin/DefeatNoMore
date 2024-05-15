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
		
		FHitResult FireHit;
		WeaponTraceHit(Start, HitTarget, FireHit);
		
		ADFNCharacter* DFNCharacter = Cast<ADFNCharacter>(FireHit.GetActor());
		if(DFNCharacter && HasAuthority() && InstigatorController)
		{
			UGameplayStatics::ApplyDamage(DFNCharacter, Damage, InstigatorController, this, UDamageType::StaticClass());
		}
		if(ImpactParticles)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, FireHit.ImpactPoint, FireHit.ImpactNormal.Rotation());
		}
		if(ImpactSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, FireHit.ImpactPoint);
		}
		if(MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
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
	FVector SphereCenter = TraceStart + ToTargetNormalized * ShotDistance;
	FVector RandVector = UKismetMathLibrary::RandomUnitVector() * FMath::FRandRange(0.f, SpreadRadius);
	FVector EndLocation = SphereCenter + RandVector;
	FVector ToEndLocation = EndLocation - TraceStart;
	return FVector(TraceStart + ToEndLocation + TRACE_LENGTH / ToEndLocation.Size());
}

void AHitScanWeapon::WeaponTraceHit(const FVector& TraceStart, const FVector& HitTarget, FHitResult& OutHit)
{
	UWorld* World = GetWorld();

	FVector TraceEnd = bUseSpread ? TraceSpread(TraceStart, HitTarget) : TraceStart + (HitTarget - TraceStart) * 10;
	if(World)
	{
		World->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);
		FVector BeamEnd = TraceEnd;

		if(OutHit.bBlockingHit)
		{
			BeamEnd = OutHit.ImpactPoint;
		}
		if(BeamParticles)
		{
			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(World, BeamParticles, TraceStart, FRotator::ZeroRotator, true);
			if(Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}
		}
		
	}
}