// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "DFNPlayerController.generated.h"


class ADFNCharacter;
class UInputMappingContext;
class UInputAction;

UCLASS()
class DEFEATNOMORE_API ADFNPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void SetupInputComponent() override;

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdatePlayerHealth(float Health, float MaxHealth);
	void UpdatePlayerScore(float Score);
	void UpdatePlayerDefeats(int32 Defeats);
	void UpdateHUDWeaponAmmo(int32 Ammo);
	void UpdateHUDCarriedAmmo(int32 CarriedAmmo);
	void UpdateHUDWeaponImage(UTexture2D* WeaponImage);
	void SetHUDMatchCountDown(float CountDownTime);

	virtual void OnPossess(APawn* InPawn) override;
	virtual void Tick(float DeltaSeconds) override;
	virtual float GetServerTime(); //synced with Server World clock

	virtual void ReceivedPlayer() override; // sync with Server Clock as soon as possible.

	void OnMatchStateSet(FName State);
	
protected:
	virtual void BeginPlay() override;

	UPROPERTY()
	ADFNCharacter* DFNCharacter;
	
	// Input calls
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Jump();
	void CharacterStopJumping(); // we`ll call the character`s stopJump() method;
	void CrouchPressed();
	void WalkPressed();
	void WalkReleased();
	void AimButtonPressed();
	void AimButtonReleased();
	void FireButtonPressed();
	void FireButtonReleased();
	void ReloadButtonPressed();
	
	// calls ServerWalkPressed, which set variables that are already replicated on CharacterMovementComponent.
	UFUNCTION(Server, Unreliable)
	void ServerWalkPressed();
	UFUNCTION(Server, Unreliable)
	void ServerWalkReleased();


	void SetHUDTime();

	// Sync time between client and Server
	/*
	 * Requests the current server time, passing in the client`s time when the request was sent
	 * @param TimeofClientRequest 
	 */
	UFUNCTION(Server, Reliable)
	void ServerRequestServerTime(float TimeofClientRequest);

	
	/**
	 * Reports the current Server time to the Client and the time when the server received the Client Request
	 * @param TimeofCLientRequest 
	 * @param TimeServerReceivedClientRequest 
	 */
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float TimeofCLientRequest, float TimeServerReceivedClientRequest);

	// Difference between client and server time.
	float ClientServerDelta = 0.f;

	UPROPERTY(EditAnywhere, Category = "Time")
	float TimeSyncFrequency = 5.f;

	float TimeSyncRunningTime = 0.f;

	void CheckTimeSync(float DeltaSeconds);

	void PollInit();

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
	
	UPROPERTY()
	class ADFNHUD* PlayerHUD;
	
	float MatchTime = 120.f;
	uint32 CountDownInt;

	UPROPERTY(ReplicatedUsing = OnRep_MatchState)
	FName MatchState;

	UFUNCTION()
	void OnRep_MatchState();

	UPROPERTY()
	class UPlayerOverlayWidget* PlayerOverlay;

	bool bInitializedCharacterOverlay = false;

	float HUDHealth;
	float HUDMaxHealth;
	float HUDScore;
	int32 HUDDefeats;
};