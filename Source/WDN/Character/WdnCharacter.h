#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Character.h"
#include "Camera/CameraActor.h"
#include "GameFramework/SpringArmComponent.h"
#include "WDN/Enums/TurnInPlace.h"
#include "WDN/Interfaces/InterfaceCrosshairInteraction.h"
#include "WdnCharacter.generated.h"

class UInputMappingContext;
class UInputAction;
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
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PostInitializeComponents() override;

	void PlayFireMontage(bool bFiring);

	virtual void OnRep_ReplicatedMovement() override;
protected:
	virtual void BeginPlay() override;

	// Input calls
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	virtual void Jump() override;
	void CrouchPressed();
	void WalkPressed();
	void WalkReleased();
	void AimButtonPressed();
	void AimButtonReleased();
	
	// calls ServerWalkPressed, which set variables that are already replicated on CharacterMovementComponent.
	UFUNCTION(Server, Unreliable)
	void ServerWalkPressed();
	UFUNCTION(Server, Unreliable)
	void ServerWalkReleased();

	void CalculateAO_Pitch();
	// set our character aimoffset in order to play proper aimoffset animations(moving the upperbody) and turninplace animations
	void AimOffset(float DeltaSeconds);
	void SimProxiesTurn();

	// calll FireButtonPressed on CombatComponent, responsible for handling all combat actions 
	void FireButtonPressed();
	void FireButtonReleased();

	void PlayHitReactMontage();

	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	void UpdateHUDHealth();
	
private:

	// SpringArm(cameraBoom) and Camera
	UPROPERTY(EditDefaultsOnly, Category = SpringArm)
	USpringArmComponent* SpringArm;
	UPROPERTY(EditDefaultsOnly, Category = Camera)
	UCameraComponent* FirstPersonCamera;
	// CombatComponent, handles all of ours combat actions
	UPROPERTY(VisibleAnywhere, Category = Combat)
	UCombatComponent* CombatComp;

	/*------------------INPUT------------------*/
	/* MappingContext */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputMappingContext* DefaultMappingContext;
	/** Actions */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* JumpAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* MoveAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Input, meta=(AllowPrivateAccess = "true"))
	UInputAction* WalkAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* LookAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* CrouchAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* AimAction;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* FireAction;
	/* ------------ END INPUT ---------------*/

	UPROPERTY(Replicated)
	bool bWalking;

	// Equip weapons, call ServerEquipWeapons on out CombatComponent, to be replicated
	void EquipWeaponOnCombatComponent();
	void ServerEquipWeaponOnCombatComponent();

	// AimOffSets and Aim rotator
	float AO_Yaw;
	float InterpAO_Yaw;
	float AO_Pitch;
	FRotator StartingAimRotation;
	
	ETurnInPlace TurningInPlace;
	/**
	 * @brief handles our character aimoffset angles, setting TurnInPLace states in order to play TurnInPLace animations proprerly
	 * @param DeltaSeconds 
	 */
	void TurnInPlace(float DeltaSeconds);

	// variable set when firing
	UPROPERTY(Replicated)
	bool bIsFiring;
	// fireweaponmontage to be played, accordingly to the weapon
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;

	bool bRotateRootBone;
	float TurnThreshHold = 0.1f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float Proxy_Yaw;
	float TimeSinceLastMovementReplication;

	float CalculateSpeed() const;
	

	/* Player Health */
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, ReplicatedUsing = OnRep_Health, Category = "Player Stats")
	float Health = 100.f;

	UFUNCTION()
	void OnRep_Health();

	UPROPERTY()
	class AWDNPlayerController* WdnPlayerController;
	
protected:
	// variable set when crouching
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Movement)
	bool bCrouched;
	// Weapon being overllaped bu our character
	UPROPERTY(Replicated)
	AWeapon* OverlappingWeapon;



public:
	
	void SetOverlappingWeapon(AWeapon* Weapon);
	AWeapon* GetEquippedWeapon();

	bool bIsAiming();
	
	FORCEINLINE float GetAO_Yaw() const { return AO_Yaw; }
	FORCEINLINE float GetAO_Pitch() const { return AO_Pitch; }
	FORCEINLINE bool GetIsFiring() const { return bIsFiring; }
	FORCEINLINE ETurnInPlace GetTurnInPlace() const { return TurningInPlace; }
	FORCEINLINE bool GetIsWalking() const { return bWalking; }
	FVector GetHitTarget() const;
	FORCEINLINE UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }
	FORCEINLINE bool ShouldRotateRootBone() const { return bRotateRootBone; }
};
