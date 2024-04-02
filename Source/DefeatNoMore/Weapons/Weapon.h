// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "GameFramework/Actor.h"
#include "DefeatNoMore/Enums/WeaponTypes.h"
#include "Weapon.generated.h"

class ADFNPlayerController;
class ADFNCharacter;
class USphereComponent;
class UAnimationAsset;

class UTexture2D;

/**
 * @brief Weapon states,(initial state is used during development only), game is supposed to have equipped and dropped only.
 */
UENUM(BlueprintType)
enum class EWeaponState : uint8
{
	EWS_Initial		UMETA(DisplayName = "InitialState"),
	EWS_Equipped	UMETA(DisplayName = "Equipped"),
	EWS_Dropped		UMETA(DisplayName = "Dropped"),

	EWS_MAX			UMETA(DisplayName = "Default")
};

UCLASS()
class DEFEATNOMORE_API AWeapon : public AActor
{
	GENERATED_BODY()
	
public:	
	AWeapon();
	virtual void Tick(float DeltaTime) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// we are overriding Fire in ProjectileWeapon
	virtual void Fire(const FVector& HitTarget);

	void DropWeapon();
	virtual void OnRep_Owner() override;

	void SetHUDAmmo();
	
protected:
	virtual void BeginPlay() override;
	
	UFUNCTION()
	virtual void OnSphereOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
								UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
								const FHitResult& SweepResult);
	UFUNCTION()
	void OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
							UPrimitiveComponent* OtherComp,	int32 OtherBodyIndex);
	

private:
	//Mesh and Collision Sphere
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USkeletalMeshComponent* WeaponMesh;
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	USphereComponent* AreaSphere;
	// WeaponState to be replicated using OnRep_WeaponState
	UPROPERTY(ReplicatedUsing = OnRep_WeaponState, VisibleAnywhere, Category = "Weapon Properties")
	EWeaponState WeaponState;


	UPROPERTY(EditAnywhere)
	float ZoomedFOV = 65.f; // Zoom FOV while aiming
	UPROPERTY(EditAnywhere)
	float ZoomInterpSpeed = 20.f; // ZoomInterSpeed to interp from/to our DefaultFOV
	
	UFUNCTION()
	void OnRep_WeaponState();
	// Weapon's fire animation
	UPROPERTY(EditAnywhere, Category = "Weapon Properties")
	UAnimationAsset* FireAnimation;
	// Subclass of bulletshell actor to be ejected when firing a projectile.
	UPROPERTY(EditAnywhere)
	TSubclassOf<class ABulletShell> BulletShellClass;


	/*** Ammo */
	UPROPERTY(EditAnywhere, ReplicatedUsing = OnRep_Ammo)
	int32 Ammo;
	UFUNCTION()
	void OnRep_Ammo();
	void SpendAmmoRound();
	UPROPERTY(EditAnywhere)
	int32 MagCapacity;
	/*** end_Ammo */

	
	UPROPERTY()
	ADFNCharacter* OwnerCharacter;
	UPROPERTY()
	ADFNPlayerController* OwnerController;

	UPROPERTY(EditAnywhere)
	EWeaponType WeaponType;
public:
	/*
	 * Textures for the weapon crosshairs
	 */
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsCenter;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsRight;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsLeft;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsTop;
	UPROPERTY(EditAnywhere, Category = Crosshairs)
	UTexture2D* CrosshairsBottom;


	UPROPERTY(EditAnywhere, Category = Combat)
	bool bIsAutomatic = true;
	UPROPERTY(EditAnywhere, Category = Combat)
	float FireDelay = 0.085f;
	// set the current weapon state.
	void SetWeaponState(EWeaponState State);
	
	FORCEINLINE USphereComponent* GetAreaSphere() const { return AreaSphere; }
	FORCEINLINE USkeletalMeshComponent* GetWeaponMesh() const { return WeaponMesh; }
	FORCEINLINE float GetZoomedFOV() const { return ZoomedFOV; }
	FORCEINLINE float GetZoomInterpSpeed() const { return ZoomInterpSpeed; }
	FORCEINLINE EWeaponType GetWeaponType() const { return WeaponType; }
	
	bool IsEmpty() const;
};
