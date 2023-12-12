// Fill out your copyright notice in the Description page of Project Settings.


#include "WDNPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "WDN/Character/WdnCharacter.h"
#include "WDN/HUD/PlayerOverlayWidget.h"
#include "WDN/HUD/WDNHUD.h"


void AWDNPlayerController::BeginPlay()
{
	Super::BeginPlay();

	PlayerHUD = Cast<AWDNHUD>(GetHUD());

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	EnableInput(this);
	
	WDNCharacter = Cast<AWdnCharacter>(GetCharacter());
	if(WDNCharacter)
	{
		UpdatePlayerHealth(WDNCharacter->GetHealth(), WDNCharacter->GetMaxHealth());
	}
}

void AWDNPlayerController::UpdatePlayerHealth(float Health, float MaxHealth)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<AWDNHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD && PlayerHUD->PlayerOverlay && PlayerHUD->PlayerOverlay->HealthText &&
				PlayerHUD->PlayerOverlay->HealthBar;
	if(bHUDValid)
	{
		const float HealthPercentage = Health / MaxHealth;
		PlayerHUD->PlayerOverlay->HealthBar->SetPercent(HealthPercentage);

		const FString HealthText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Health));
		PlayerHUD->PlayerOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
}

void AWDNPlayerController::UpdatePlayerScore(float Score)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<AWDNHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD &&
		PlayerHUD->PlayerOverlay &&
		PlayerHUD->PlayerOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		PlayerHUD->PlayerOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
}

void AWDNPlayerController::UpdatePlayerDefeats(int32 Defeats)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<AWDNHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD &&
		PlayerHUD->PlayerOverlay &&
		PlayerHUD->PlayerOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		PlayerHUD->PlayerOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
}

void AWDNPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	WDNCharacter = Cast<AWdnCharacter>(InPawn);
	if(WDNCharacter)
	{
		UpdatePlayerHealth(WDNCharacter->GetHealth(), WDNCharacter->GetMaxHealth());
	}
}

void AWDNPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if(UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		//Jump
		EIC->BindAction(JumpAction, ETriggerEvent::Triggered, this, &AWDNPlayerController::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &AWDNPlayerController::CharacterStopJumping);
		//Move
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AWDNPlayerController::Move);
		// Walk (not running / no footstep sounds)
		EIC->BindAction(WalkAction, ETriggerEvent::Triggered, this, &AWDNPlayerController::WalkPressed);
		EIC->BindAction(WalkAction, ETriggerEvent::Completed, this, &AWDNPlayerController::WalkReleased);
		//Look
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &AWDNPlayerController::Look);
		//Crouch
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &AWDNPlayerController::CrouchPressed);
		// Aim - ZoomFOV
		EIC->BindAction(AimAction, ETriggerEvent::Triggered, this, &AWDNPlayerController::AimButtonPressed);
		EIC->BindAction(AimAction, ETriggerEvent::Completed, this, &AWDNPlayerController::AimButtonReleased);
		//Fire
		EIC->BindAction(FireAction, ETriggerEvent::Triggered, this, &AWDNPlayerController::FireButtonPressed);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &AWDNPlayerController::FireButtonReleased);
	}
}

void AWDNPlayerController::Move(const FInputActionValue& Value)
{
	// AddMovementInput takes a Vector
	const FVector2d MovementVector = Value.Get<FVector2d>();
	if(IsValid(WDNCharacter))
	{
		WDNCharacter->AddMovementInput(WDNCharacter->GetActorForwardVector(), MovementVector.Y);
		WDNCharacter->AddMovementInput(WDNCharacter->GetActorRightVector(), MovementVector.X);
	}
}

void AWDNPlayerController::Look(const FInputActionValue& Value)
{
	// AddControllerPitchInput takes a Vector
	const FVector2d LookAxisVector = Value.Get<FVector2d>();
	float Pitch = LookAxisVector.Y;
	if(IsValid(WDNCharacter))
	{
		APlayerController* PC = Cast<APlayerController>(this);
		PC->AddPitchInput(LookAxisVector.Y);
		WDNCharacter->AddControllerYawInput(LookAxisVector.X);
	}
}

void AWDNPlayerController::Jump()
{
	if(WDNCharacter)
	{
		if(WDNCharacter->bIsCrouched)
		{
			WDNCharacter->UnCrouch();
		}
		WDNCharacter->Jump();
	}
}

void AWDNPlayerController::CharacterStopJumping()
{
	if(WDNCharacter)
	{
		WDNCharacter->StopJumping();
	}
}

void AWDNPlayerController::CrouchPressed()
{
	if(WDNCharacter)
	{
		if(WDNCharacter->bIsCrouched)
		{
			WDNCharacter->UnCrouch();
		}
		WDNCharacter->Crouch();
	}
}

void AWDNPlayerController::WalkPressed()
{
	if(WDNCharacter)
	{
		if(IsLocalController()) // if not right, we`ll try IsLocalController()
		{
			ServerWalkPressed();
		}
		WDNCharacter->bWalking = true;
		WDNCharacter->GetCharacterMovement()->MaxWalkSpeed = 220.f;
	}
}

void AWDNPlayerController::WalkReleased()
{
	if(WDNCharacter)
	{
		if(IsLocalController())
		{
			ServerWalkReleased();
		}
		WDNCharacter->bWalking = false;
		WDNCharacter->GetCharacterMovement()->MaxWalkSpeed = 420.f;
	}
}

void AWDNPlayerController::ServerWalkPressed_Implementation()
{
	if(WDNCharacter)
	{
		WDNCharacter->bWalking = true;
		WDNCharacter->GetCharacterMovement()->MaxWalkSpeed = 220.f;
	}
}

void AWDNPlayerController::ServerWalkReleased_Implementation()
{
	if(WDNCharacter)
	{
		WDNCharacter->bWalking = false;
		WDNCharacter->GetCharacterMovement()->MaxWalkSpeed = 420.f;
	}
}

void AWDNPlayerController::AimButtonPressed()
{
	if(WDNCharacter)
	{
		WDNCharacter->AimButtonPressed();
	}
}

void AWDNPlayerController::AimButtonReleased()
{
	if(WDNCharacter)
	{
		WDNCharacter->AimButtonReleased();
	}
}

void AWDNPlayerController::FireButtonPressed()
{
	if(WDNCharacter)
	{
		WDNCharacter->FireButtonPressed();
	}
}

void AWDNPlayerController::FireButtonReleased()
{
	if(WDNCharacter)
	{
		WDNCharacter->FireButtonReleased();
	}
}