// Fill out your copyright notice in the Description page of Project Settings.

#include "DFNPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DefeatNoMore/Character/DFNCharacter.h"
#include "DefeatNoMore/Components/CombatComponent.h"
#include "DefeatNoMore/HUD/PlayerOverlayWidget.h"
#include "DefeatNoMore/HUD/DFNHUD.h"

#include "GameFramework/GameMode.h"

#include "Net/UnrealNetwork.h"


void ADFNPlayerController::BeginPlay()
{
	Super::BeginPlay();

	PlayerHUD = Cast<ADFNHUD>(GetHUD());

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	DFNCharacter = Cast<ADFNCharacter>(GetCharacter());
	if(DFNCharacter)
	{
		UpdatePlayerHealth(DFNCharacter->GetHealth(), DFNCharacter->GetMaxHealth());
		DFNCharacter->EnableInput(this);
	}
}

void ADFNPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();
	CheckTimeSync(DeltaSeconds);
	PollInit();
}

void ADFNPlayerController::CheckTimeSync(float DeltaSeconds)
{
	TimeSyncRunningTime += ClientServerDelta;
	if(IsLocalController() && TimeSyncRunningTime > TimeSyncFrequency)
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
		TimeSyncRunningTime = 0.f;
	}
}

void ADFNPlayerController::PollInit()
{
	if(PlayerOverlay == nullptr)
	{
		if(PlayerHUD && PlayerHUD->PlayerOverlay)
		{
			PlayerOverlay = PlayerHUD->PlayerOverlay;
			if(PlayerOverlay)
			{
				UpdatePlayerHealth(HUDHealth, HUDMaxHealth);
				UpdatePlayerScore(HUDScore);
				UpdatePlayerDefeats(HUDDefeats);
			}
		}
	}
}

void ADFNPlayerController::UpdatePlayerHealth(float Health, float MaxHealth)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : PlayerHUD;

	bool bHUDValid = PlayerHUD && PlayerHUD->PlayerOverlay && PlayerHUD->PlayerOverlay->HealthText &&
				PlayerHUD->PlayerOverlay->HealthBar;
	if(bHUDValid)
	{
		const float HealthPercentage = Health / MaxHealth;
		PlayerHUD->PlayerOverlay->HealthBar->SetPercent(HealthPercentage);

		const FString HealthText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Health));
		PlayerHUD->PlayerOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		bInitializedCharacterOverlay = true;
		HUDHealth = Health;
		HUDMaxHealth = MaxHealth;
	}
}

void ADFNPlayerController::UpdatePlayerScore(float Score)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD &&
		PlayerHUD->PlayerOverlay &&
		PlayerHUD->PlayerOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		PlayerHUD->PlayerOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializedCharacterOverlay = true;
		HUDScore = Score;
	}
}

void ADFNPlayerController::UpdatePlayerDefeats(int32 Defeats)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD &&
		PlayerHUD->PlayerOverlay &&
		PlayerHUD->PlayerOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		PlayerHUD->PlayerOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializedCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void ADFNPlayerController::UpdateHUDWeaponAmmo(int32 Ammo)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD && PlayerHUD->PlayerOverlay && PlayerHUD->PlayerOverlay->WeaponCurrentAmmo;
	if(bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		PlayerHUD->PlayerOverlay->WeaponCurrentAmmo->SetText(FText::FromString(AmmoText));
	}
}

void ADFNPlayerController::UpdateHUDCarriedAmmo(int32 CarriedAmmo)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD &&
		PlayerHUD->PlayerOverlay &&
		PlayerHUD->PlayerOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString CarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		PlayerHUD->PlayerOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoText));
	}
}

void ADFNPlayerController::UpdateHUDWeaponImage(UTexture2D* WeaponImage)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD &&
		PlayerHUD->PlayerOverlay &&
		PlayerHUD->PlayerOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		PlayerHUD->PlayerOverlay->WeaponImageField->SetBrushFromTexture(WeaponImage);
		PlayerHUD->PlayerOverlay->WeaponImageField->SetOpacity(1.0f);
	}
}

void ADFNPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(DefaultMappingContext, 0);
	}
	EnableInput(this);

	
	DFNCharacter = Cast<ADFNCharacter>(InPawn);
	if(DFNCharacter)
	{
		UpdatePlayerHealth(DFNCharacter->GetHealth(), DFNCharacter->GetMaxHealth());
		DFNCharacter->EnableInput(this);
	}
}

void ADFNPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	if(UEnhancedInputComponent* EIC = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		//Jump
		EIC->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ADFNPlayerController::Jump);
		EIC->BindAction(JumpAction, ETriggerEvent::Completed, this, &ADFNPlayerController::CharacterStopJumping);
		//Move
		EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ADFNPlayerController::Move);
		// Walk (not running / no footstep sounds)
		EIC->BindAction(WalkAction, ETriggerEvent::Triggered, this, &ADFNPlayerController::WalkPressed);
		EIC->BindAction(WalkAction, ETriggerEvent::Completed, this, &ADFNPlayerController::WalkReleased);
		//Look
		EIC->BindAction(LookAction, ETriggerEvent::Triggered, this, &ADFNPlayerController::Look);
		//Crouch
		EIC->BindAction(CrouchAction, ETriggerEvent::Started, this, &ADFNPlayerController::CrouchPressed);
		// Aim - ZoomFOV
		EIC->BindAction(AimAction, ETriggerEvent::Triggered, this, &ADFNPlayerController::AimButtonPressed);
		EIC->BindAction(AimAction, ETriggerEvent::Completed, this, &ADFNPlayerController::AimButtonReleased);
		//Fire
		EIC->BindAction(FireAction, ETriggerEvent::Started, this, &ADFNPlayerController::FireButtonPressed);
		EIC->BindAction(FireAction, ETriggerEvent::Completed, this, &ADFNPlayerController::FireButtonReleased);

		EIC->BindAction(ReloadAction, ETriggerEvent::Started, this, &ADFNPlayerController::ReloadButtonPressed);

	}
}

void ADFNPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADFNPlayerController, MatchState);
}

void ADFNPlayerController::Move(const FInputActionValue& Value)
{
	// AddMovementInput takes a Vector
	const FVector2d MovementVector = Value.Get<FVector2d>();
	if(IsValid(DFNCharacter))
	{
		DFNCharacter->AddMovementInput(DFNCharacter->GetActorForwardVector(), MovementVector.Y);
		DFNCharacter->AddMovementInput(DFNCharacter->GetActorRightVector(), MovementVector.X);
	}
}

void ADFNPlayerController::Look(const FInputActionValue& Value)
{
	// AddControllerPitchInput takes a Vector
	const FVector2d LookAxisVector = Value.Get<FVector2d>();
	float Pitch = LookAxisVector.Y;
	if(IsValid(DFNCharacter))
	{
		APlayerController* PC = Cast<APlayerController>(this);
		PC->AddPitchInput(LookAxisVector.Y);
		DFNCharacter->AddControllerYawInput(LookAxisVector.X);
	}
}

void ADFNPlayerController::Jump()
{
	if(DFNCharacter)
	{
		if(DFNCharacter->bIsCrouched)
		{
			DFNCharacter->UnCrouch();
		}
		DFNCharacter->Jump();
	}
}

void ADFNPlayerController::CharacterStopJumping()
{
	if(DFNCharacter)
	{
		DFNCharacter->StopJumping();
	}
}

void ADFNPlayerController::CrouchPressed()
{
	if(DFNCharacter)
	{
		if(DFNCharacter->bIsCrouched)
		{
			DFNCharacter->UnCrouch();
		}
		DFNCharacter->Crouch();
	}
}

void ADFNPlayerController::WalkPressed()
{
	if(DFNCharacter)
	{
		if(IsLocalController()) // if not right, we`ll try IsLocalController()
		{
			ServerWalkPressed();
		}
		DFNCharacter->bWalking = true;
		DFNCharacter->GetCharacterMovement()->MaxWalkSpeed = 220.f;
	}
}

void ADFNPlayerController::WalkReleased()
{
	if(DFNCharacter)
	{
		if(IsLocalController())
		{
			ServerWalkReleased();
		}
		DFNCharacter->bWalking = false;
		DFNCharacter->GetCharacterMovement()->MaxWalkSpeed = 420.f;
	}
}

void ADFNPlayerController::ServerWalkPressed_Implementation()
{
	if(DFNCharacter)
	{
		DFNCharacter->bWalking = true;
		DFNCharacter->GetCharacterMovement()->MaxWalkSpeed = 220.f;
	}
}

void ADFNPlayerController::ServerWalkReleased_Implementation()
{
	if(DFNCharacter)
	{
		DFNCharacter->bWalking = false;
		DFNCharacter->GetCharacterMovement()->MaxWalkSpeed = 420.f;
	}
}

void ADFNPlayerController::AimButtonPressed()
{
	if(DFNCharacter)
	{
		DFNCharacter->AimButtonPressed();
	}
}

void ADFNPlayerController::AimButtonReleased()
{
	if(DFNCharacter)
	{
		DFNCharacter->AimButtonReleased();
	}
}

void ADFNPlayerController::FireButtonPressed()
{
	if(DFNCharacter)
	{
		DFNCharacter->FireButtonPressed();
	}
}

void ADFNPlayerController::FireButtonReleased()
{
	if(DFNCharacter)
	{
		DFNCharacter->FireButtonReleased();
	}
}

void ADFNPlayerController::ReloadButtonPressed()
{
	if(DFNCharacter && DFNCharacter->GetCombatComponent())
	{
		DFNCharacter->GetCombatComponent()->Reload();
	}
}

void ADFNPlayerController::SetHUDMatchCountDown(float CountDownTime)
{
	PlayerHUD = PlayerHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : PlayerHUD;
	bool bHUDValid = PlayerHUD &&
		PlayerHUD->PlayerOverlay &&
		PlayerHUD->PlayerOverlay->MatchCountDownText;
	if (bHUDValid)
	{
		int32 Minutes = FMath::FloorToInt(CountDownTime / 60);
		int32 Seconds = CountDownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		PlayerHUD->PlayerOverlay->MatchCountDownText->SetText(FText::FromString(CountdownText));
	}
}

void ADFNPlayerController::SetHUDTime()
{
	uint32 SecondsLeft = FMath::CeilToInt(MatchTime - GetServerTime());

	if(CountDownInt != SecondsLeft)
	{
		SetHUDMatchCountDown(MatchTime - GetServerTime());
	}
	CountDownInt = SecondsLeft;
}

void ADFNPlayerController::ServerRequestServerTime_Implementation(float TimeofClientRequest)
{
	float ServerTimeofReceipt = GetWorld()->GetTimeSeconds();

	ClientReportServerTime(TimeofClientRequest, ServerTimeofReceipt);
}

void ADFNPlayerController::ClientReportServerTime_Implementation(float TimeofCLientRequest,
	float TimeServerReceivedClientRequest)
{
	float RoundTripTime = GetWorld()->GetTimeSeconds() - TimeofCLientRequest;
	float CurrentServerTime = TimeServerReceivedClientRequest + (0.5f * RoundTripTime);
	ClientServerDelta = CurrentServerTime - GetWorld()->GetTimeSeconds();
}

float ADFNPlayerController::GetServerTime()
{
	if(HasAuthority())
	{
		return GetWorld()->GetTimeSeconds();
	}
	else
	{
		return GetWorld()->GetTimeSeconds() + ClientServerDelta;
	}
}

void ADFNPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if(IsLocalController())
	{
		ServerRequestServerTime(GetWorld()->GetTimeSeconds());
	}
}

void ADFNPlayerController::OnMatchStateSet(FName State)
{
	MatchState = State;

	if(MatchState == MatchState::InProgress)
	{
		PlayerHUD = PlayerHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : PlayerHUD;
		if(PlayerHUD)
		{
			PlayerHUD->AddPlayerOverlay();
		}
	}
}

void ADFNPlayerController::OnRep_MatchState()
{
	if(MatchState == MatchState::InProgress)
	{
		PlayerHUD = PlayerHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : PlayerHUD;
		if(PlayerHUD)
		{
			PlayerHUD->AddPlayerOverlay();
		}
	}
}