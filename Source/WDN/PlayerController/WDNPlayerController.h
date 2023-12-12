// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputActionValue.h"
#include "WDNPlayerController.generated.h"


class AWdnCharacter;
class UInputMappingContext;
class UInputAction;

UCLASS()
class WDN_API AWDNPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	virtual void SetupInputComponent() override;


	void UpdatePlayerHealth(float Health, float MaxHealth);
	void UpdatePlayerScore(float Score);
	void UpdatePlayerDefeats(int32 Defeats);

	virtual void OnPossess(APawn* InPawn) override;
	

protected:
	virtual void BeginPlay() override;

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
	
	// calls ServerWalkPressed, which set variables that are already replicated on CharacterMovementComponent.
	UFUNCTION(Server, Unreliable)
	void ServerWalkPressed();
	UFUNCTION(Server, Unreliable)
	void ServerWalkReleased();

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
	/* ------------ END INPUT ---------------*/
	
	UPROPERTY()
	class AWDNHUD* PlayerHUD;

protected:

	UPROPERTY()
	AWdnCharacter* WDNCharacter;
	
};
