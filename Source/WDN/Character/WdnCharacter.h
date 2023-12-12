#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Camera/CameraActor.h"
#include "Components/TimelineComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "WDN/Enums/TurnInPlace.h"
#include "WDN/Interfaces/InterfaceCrosshairInteraction.h"
#include "WdnCharacter.generated.h"

class UInputMappingContext;
class AWeapon;
class UCombatComponent;
class UAnimMontage;

UCLASS()
class WDN_API AWdnCharacter : public ACharacter, public IInterfaceCrosshairInteraction
{
	GENERATED_BODY()	


public:
	AWdnCharacter();
	virtual void Tick(float DeltaSeconds) override;
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void OnRep_ReplicatedMovement() override;
	void PlayFireMontage(bool bFiring);

	// Eliminations 
	void Elimination();
	UFUNCTION(Server, Reliable)
	void ServerElimination();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElimination();
	void PlayEliminationMontage();

	virtual void Jump() override;

	void SetOverlappingWeapon(AWeapon* Weapon);
	AWeapon* GetEquippedWeapon();
	
	UPROPERTY(Replicated)
	bool bWalking;
	
	// call AimButtonPressed on CombatComponent, responsible for handling all combat actions 
	void AimButtonPressed();
	void AimButtonReleased();
	// calll FireButtonPressed on CombatComponent, responsible for handling all combat actions 
	void FireButtonPressed();
	void FireButtonReleased();

protected:
	virtual void BeginPlay() override;

	// Weapon being overlapped bu our character
	UPROPERTY(Replicated)
	AWeapon* OverlappingWeapon;

	// variable set when crouching
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	bool bCrouched;
	
	void CalculateAO_Pitch();
	// set our character aimoffset in order to play proper aimoffset animations(moving the upperbody) and turninplace animations
	void AimOffset(float DeltaSeconds);
	void SimProxiesTurn();
	
	void PlayHitReactMontage();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	void UpdateHUDHealth();
	// Poll for any relelvant classes and initialize our HUD
	void PollInit();
	
private:
		
	// SpringArm(cameraBoom) and Camera
	UPROPERTY(EditDefaultsOnly, Category = SpringArm)
	USpringArmComponent* SpringArm;
	UPROPERTY(EditDefaultsOnly, Category = Camera)
	UCameraComponent* FirstPersonCamera;
	// CombatComponent, handles all of ours combat actions
	UPROPERTY(VisibleAnywhere, Category = Combat)
	UCombatComponent* CombatComp;
		
	// Equip weapons, call ServerEquipWeapons on out CombatComponent, to be replicated
	void EquipWeaponOnCombatComponent();
	void ServerEquipWeaponOnCombatComponent();

	// AimOffSets and Aim rotator
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;

	/* Player Health */
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Health, Category = "Player Stats")
	float Health = 100.f;
	
	bool bIsAiming() const;
	
	ETurnInPlace TurningInPlace;
	/**
	 * @brief handles our character aim offset angles, setting TurnInPLace states in order to play TurnInPLace animations proprerly
	 * @param DeltaSeconds 
	 */
	void TurnInPlace(float DeltaSeconds);

	// variable set when firing
	UPROPERTY(Replicated)
	bool bIsFiring;
	// fire weapon montage to be played, accordingly to the weapon
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* DeathMontage;

	bool bRotateRootBone;
	float TurnThreshHold = 0.1f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float Proxy_Yaw;
	float TimeSinceLastMovementReplication;

	float CalculateSpeed() const;
	

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY()
	class AWDNPlayerController* WdnPlayerController;

	bool bEliminated = false;

	FTimerHandle EliminationTimer;

	UPROPERTY(EditDefaultsOnly)
	float EliminationDelay = 1.5f;

	void EliminationTimerFinished();

	/* Dissolve Effect */
	UPROPERTY(VisibleAnywhere)
	UTimelineComponent* DissolveTimeLine;
	FOnTimelineFloat DissolveTrack;

	UPROPERTY(EditAnywhere)
	UCurveFloat* DissolveCurve;


	UFUNCTION()
	void UpdateDissolveMaterial(float DissolveValue);
	void StartDissolve();

	UPROPERTY(VisibleAnywhere, Category = Elim)
	UMaterialInstanceDynamic* DynamicDissolveMaterialInstance;

	UPROPERTY(EditAnywhere, Category = Elim)
	UMaterialInstance* DissolveMaterialInstance;

	UPROPERTY()
	class AWDNPlayerState* WDNPlayerState;

public:
	FORCEINLINE float GetMaxHealth() const { return MaxHealth; }
	FORCEINLINE float GetHealth() const { return Health; }
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
	FORCEINLINE	bool IsEliminated() const { return bEliminated;	}
	FORCEINLINE bool GetIsWalking() const { return bWalking; }
	FORCEINLINE bool GetIsFiring() const { return bIsFiring; }

	FORCEINLINE ETurnInPlace GetTurnInPlace() const { return TurningInPlace; }
	FORCEINLINE UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	FVector GetHitTarget() const;
};
