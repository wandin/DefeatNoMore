// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "DFNProjectileMovementComponent.generated.h"

/**
 * 
 */
UCLASS()
class DEFEATNOMORE_API UDFNProjectileMovementComponent : public UProjectileMovementComponent
{
	GENERATED_BODY()

protected:

	virtual EHandleBlockingHitResult HandleBlockingHit(const FHitResult& Hit, float TimeTick, const FVector& MoveDelta, float& SubTickTimeRemaining) override;

	virtual void HandleImpact(const FHitResult& Hit, float TimeSlice, const FVector& MoveDelta) override;
};
