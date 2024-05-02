#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"

#include "GameFramework/Character.h"
#include "Camera/CameraActor.h"
#include "Components/TimelineComponent.h"

#include "DefeatNoMore/Enums/CombatState.h"

#include "GameFramework/SpringArmComponent.h"
#include "DefeatNoMore/Enums/TurnInPlace.h"
#include "DefeatNoMore/Interfaces/InterfaceCrosshairInteraction.h"
#include "DFNCharacter.generated.h"

class AWeapon;
class UCombatComponent;
class UAnimMontage;

class UInputMappingContext;
class UInputAction;

UCLASS()
class DEFEATNOMORE_API ADFNCharacter : public ACharacter, public IInterfaceCrosshairInteraction
{
	GENERATED_BODY()	


public:
	ADFNCharacter();
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void PostInitializeComponents() override;
	virtual void PossessedBy(AController* NewController) override;

	
	void PlayFireMontage(bool bFiring);
	void PlayReloadMontage() const;

	virtual void OnRep_ReplicatedMovement() override;
	
	// Eliminations 
	void Elimination();
	UFUNCTION(NetMulticast, Reliable)
	void MulticastElimination();
	void PlayEliminationMontage();


	/* Weapon Equipping */
	void SetOverlappingWeapon(AWeapon* Weapon);
	AWeapon* GetEquippedWeapon() const;
	/* -- */
	
	UPROPERTY(Replicated)
	bool bWalking;
	
	UPROPERTY(Replicated)
	bool bDisableGameplay = false;

protected:
	virtual void BeginPlay() override;
	
	// Input calls
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	virtual void Jump() override;
	void CharacterStopJumping(); // we`ll call the character`s stopJump() method;
	void CrouchPressed();
	void WalkPressed();
	void WalkReleased();
	void AimButtonPressed();	// call AimButtonPressed on CombatComponent, responsible for handling all combat actions 
	void AimButtonReleased();
	void FireButtonPressed();	// calll FireButtonPressed on CombatComponent, responsible for handling all combat actions 
	void FireButtonReleased();
	void ReloadButtonPressed();

	// calls ServerWalkPressed, which set variables that are already replicated on CharacterMovementComponent.
	UFUNCTION(Server, Unreliable)
	void ServerWalkPressed();
	UFUNCTION(Server, Unreliable)
	void ServerWalkReleased();


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
	
	void PlayHitReactMontage() const;
	
	UFUNCTION()
	void ReceiveDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType, class AController* InstigatorController, AActor* DamageCauser);
	void UpdateHUDHealth();
	// Poll for any relelvant classes and initialize our HUD
	void PollInit();
	void RotateInPlace(float DeltaSeconds);
	
private:

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
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Input, meta = (AllowPrivateAccess = "true"))
	UInputAction* ReloadAction;
	/* ------------ END INPUT ---------------*/
	

	
	
	// SpringArm(cameraBoom) and Camera
	UPROPERTY(EditDefaultsOnly, Category = SpringArm)
	USpringArmComponent* SpringArm;
	UPROPERTY(EditDefaultsOnly, Category = Camera)
	UCameraComponent* ThirdPersonCamera;
	
	// CombatComponent, handles all of ours combat actions
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "true"))
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
	
	/*
	 * Animations Montages
	 */
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* FireWeaponMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* ReloadMontage;
	
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* HitReactMontage;
	
	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* DeathMontage;

	UPROPERTY(EditAnywhere, Category = Combat)
	UAnimMontage* EquipWeaponMontage;

	bool bRotateRootBone;
	float TurnThreshHold = 0.1f;
	FRotator ProxyRotationLastFrame;
	FRotator ProxyRotation;
	float Proxy_Yaw;
	float TimeSinceLastMovementReplication;

	float CalculateSpeed() const;
	

	UFUNCTION()
	void OnRep_Health();
	
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
	class ADFNPlayerController* DFNPlayerController;
	
	UPROPERTY()
	class ADFNPlayerState* DFNPlayerState;

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
	FORCEINLINE UCameraComponent* GetThirdPersonCamera() const { return ThirdPersonCamera; }
	FORCEINLINE UCombatComponent* GetCombatComponent() const { return CombatComp; }

	FORCEINLINE bool GetDisableGameplay() { return bDisableGameplay; }
	
	ECombatState GetCombatState() const;
	
	FVector GetHitTarget() const;
};
