// Fill out your copyright notice in the Description page of Project Settings.


#include "DefeatNoMore/Weapons/Projectiles/DFNProjectileMovementComponent.h"

UProjectileMovementComponent::EHandleBlockingHitResult UDFNProjectileMovementComponent::HandleBlockingHit(
	const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining)
{
	Super::HandleBlockingHit(Hit, TimeTick, MoveDelta, SubTickTimeRemaining);

	return EHandleBlockingHitResult::AdvanceNextSubstep;
}

void UDFNProjectileMovementComponent::HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta)
{
	// Projectiles should not stop
	//Rockets should not stop, only explode when their CollisionBox detects a hit.
}
