// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "BulletShell.generated.h"

class USoundCue;

UCLASS()
class WDN_API ABulletShell : public AActor
{
	GENERATED_BODY()
	
public:	
	ABulletShell();
	
protected:
	virtual void BeginPlay() override;

	/**
	 * @brief OnHit delegate function to play proper FX's when the bulletshell hits the ground or anything while falls
	 * @param HitComp - component that generate the hit event
	 * @param OtherActor - actor that was hit
	 * @param OtherComp  - other comp that was hit
	 * @param NormalImpulse  - type of impulse
	 * @param Hit - FHitResult...
	 */
	UFUNCTION()
	virtual void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);
private:

	// BulletShell mesh
	UPROPERTY(VisibleAnywhere)
	UStaticMeshComponent* BulletShellMesh;
	/**
	 * @brief  float used on AddImpulse and AddRelativeRotation to our shell when it's ejected, but before is applied a random value to it
	 */
	UPROPERTY(EditAnywhere)
	float ShellEjectionImpulse;

	// Sound Cue
	UPROPERTY(EditAnywhere)
	USoundCue* DroppingShellSound;
	// time until Bulletshell be destroyed
	float LifeSpan;

};