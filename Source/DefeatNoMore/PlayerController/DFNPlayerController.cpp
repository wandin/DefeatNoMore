// Fill out your copyright notice in the Description page of Project Settings.

#include "DFNPlayerController.h"


#include "InterchangeResult.h"
#include "TimerManager.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DefeatNoMore/Character/DFNCharacter.h"
#include "DefeatNoMore/Components/CombatComponent.h"
#include "DefeatNoMore/GameModes/DFNGameMode.h"
#include "DefeatNoMore/GameStates/DFNGameState.h"
#include "DefeatNoMore/HUD/Announcement.h"
#include "DefeatNoMore/HUD/PlayerOverlayWidget.h"
#include "DefeatNoMore/HUD/DFNHUD.h"
#include "DefeatNoMore/PlayerState/DFNPlayerState.h"

#include "GameFramework/GameMode.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"


void ADFNPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	GameHUD = Cast<ADFNHUD>(GetHUD());

	ServerCheckMatchState();
}

void ADFNPlayerController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	SetHUDTime();
	CheckTimeSync(DeltaSeconds);
	PollInit();
}

void ADFNPlayerController::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADFNPlayerController, MatchState);
}

void ADFNPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
		
	DFNCharacter = Cast<ADFNCharacter>(InPawn);
	if(DFNCharacter)
	{
		UpdatePlayerHealth(DFNCharacter->GetHealth(), DFNCharacter->GetMaxHealth());
	}
}

void ADFNPlayerController::PollInit()
{
	if(PlayerOverlay == nullptr)
	{
		if(GameHUD && GameHUD->PlayerOverlay)
		{
			PlayerOverlay = GameHUD->PlayerOverlay;
			if(PlayerOverlay)
			{
				UpdatePlayerHealth(HUDHealth, HUDMaxHealth);
				UpdatePlayerScore(HUDScore);
				UpdatePlayerDefeats(HUDDefeats);
			}
		}
	}
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

void ADFNPlayerController::ServerCheckMatchState_Implementation()
{
	ADFNGameMode* GameMode = Cast<ADFNGameMode>(UGameplayStatics::GetGameMode(this));
	if(GameMode)
	{
		LevelStartTime = GameMode->LevelStartingTime;
		WarmupTime = GameMode->WarmupTime;
		MatchTime = GameMode->MatchTime;
		CooldownTime = GameMode->CooldownTime;
		MatchState = GameMode->GetMatchState();
		
		ClientJoinMidGame(MatchState, WarmupTime, MatchTime, CooldownTime, LevelStartTime);
	}
}

void ADFNPlayerController::ClientJoinMidGame_Implementation(FName StateOfMatch, float TimeWarmup, float TimeMatch, float TimeCoolDown, float StartingTime)
{
	LevelStartTime = StartingTime;
	WarmupTime = TimeWarmup;
	MatchTime = TimeMatch;
	CooldownTime = TimeCoolDown;
	MatchState = StateOfMatch;
	OnMatchStateSet(MatchState);

	if(GameHUD && MatchState == MatchState::WaitingToStart)
	{
		GameHUD->AddAnnouncementOverlay();
	}
}

void ADFNPlayerController::UpdatePlayerHealth(float Health, float MaxHealth)
{
	GameHUD = GameHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : GameHUD;

	bool bHUDValid = GameHUD && GameHUD->PlayerOverlay && GameHUD->PlayerOverlay->HealthText &&
				GameHUD->PlayerOverlay->HealthBar;
	if(bHUDValid)
	{
		const float HealthPercentage = Health / MaxHealth;
		GameHUD->PlayerOverlay->HealthBar->SetPercent(HealthPercentage);

		const FString HealthText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Health));
		GameHUD->PlayerOverlay->HealthText->SetText(FText::FromString(HealthText));
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
	GameHUD = GameHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : GameHUD;
	bool bHUDValid = GameHUD &&
		GameHUD->PlayerOverlay &&
		GameHUD->PlayerOverlay->ScoreAmount;
	if (bHUDValid)
	{
		FString ScoreText = FString::Printf(TEXT("%d"), FMath::FloorToInt(Score));
		GameHUD->PlayerOverlay->ScoreAmount->SetText(FText::FromString(ScoreText));
	}
	else
	{
		bInitializedCharacterOverlay = true;
		HUDScore = Score;
	}
}

void ADFNPlayerController::UpdatePlayerDefeats(int32 Defeats)
{
	GameHUD = GameHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : GameHUD;
	bool bHUDValid = GameHUD &&
		GameHUD->PlayerOverlay &&
		GameHUD->PlayerOverlay->DefeatsAmount;
	if (bHUDValid)
	{
		FString DefeatsText = FString::Printf(TEXT("%d"), Defeats);
		GameHUD->PlayerOverlay->DefeatsAmount->SetText(FText::FromString(DefeatsText));
	}
	else
	{
		bInitializedCharacterOverlay = true;
		HUDDefeats = Defeats;
	}
}

void ADFNPlayerController::UpdateHUDWeaponAmmo(int32 Ammo)
{
	GameHUD = GameHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : GameHUD;
	bool bHUDValid = GameHUD && GameHUD->PlayerOverlay && GameHUD->PlayerOverlay->WeaponCurrentAmmo;
	if(bHUDValid)
	{
		FString AmmoText = FString::Printf(TEXT("%d"), Ammo);
		GameHUD->PlayerOverlay->WeaponCurrentAmmo->SetText(FText::FromString(AmmoText));
	}
}

void ADFNPlayerController::UpdateHUDCarriedAmmo(int32 CarriedAmmo)
{
	GameHUD = GameHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : GameHUD;
	bool bHUDValid = GameHUD &&
		GameHUD->PlayerOverlay &&
		GameHUD->PlayerOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		FString CarriedAmmoText = FString::Printf(TEXT("%d"), CarriedAmmo);
		GameHUD->PlayerOverlay->CarriedAmmoAmount->SetText(FText::FromString(CarriedAmmoText));
	}
}

void ADFNPlayerController::UpdateHUDWeaponImage(UTexture2D* WeaponImage)
{
	GameHUD = GameHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : GameHUD;
	bool bHUDValid = GameHUD &&
		GameHUD->PlayerOverlay &&
		GameHUD->PlayerOverlay->CarriedAmmoAmount;
	if (bHUDValid)
	{
		GameHUD->PlayerOverlay->WeaponImageField->SetBrushFromTexture(WeaponImage);
		GameHUD->PlayerOverlay->WeaponImageField->SetOpacity(1.0f);
	}
}

void ADFNPlayerController::SetHUDMatchCountDown(float MatchCountDownTime)
{
	GameHUD = GameHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : GameHUD;
	bool bHUDValid = GameHUD &&
		GameHUD->PlayerOverlay &&
		GameHUD->PlayerOverlay->MatchCountDownText;
	if (bHUDValid)
	{
		if(MatchCountDownTime <= 15.f)
		{
			GameHUD->PlayerOverlay->MatchCountDownText->SetColorAndOpacity(FColor::Orange);
		}
		if(MatchCountDownTime <= 10.f)
		{
			GameHUD->PlayerOverlay->MatchCountDownText->SetColorAndOpacity(FColor::Red);
		}
		if(MatchCountDownTime < 0.f)
		{
			// If Countdown is negative we don't display it and return;
			GameHUD->PlayerOverlay->MatchCountDownText->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(MatchCountDownTime / 60);
		int32 Seconds = MatchCountDownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		GameHUD->PlayerOverlay->MatchCountDownText->SetText(FText::FromString(CountdownText));
	}
}

void ADFNPlayerController::SetHUDWarmupCountDown(float WarmupCountDownTime)
{
	GameHUD = GameHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : GameHUD;
	bool bHUDValid = GameHUD &&
		GameHUD->AnnouncementOverlay &&
		GameHUD->AnnouncementOverlay->WarmupTime;
	if (bHUDValid)
	{
		if(WarmupCountDownTime < 0.f)
		{
			// If Countdown is negative we don't display it and return;
			GameHUD->AnnouncementOverlay->WarmupTime->SetText(FText());
			return;
		}
		int32 Minutes = FMath::FloorToInt(WarmupCountDownTime / 60);
		int32 Seconds = WarmupCountDownTime - Minutes * 60;
		FString CountdownText = FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds);
		GameHUD->AnnouncementOverlay->WarmupTime->SetText(FText::FromString(CountdownText));
	}
}

void ADFNPlayerController::SetHUDTime()
{
	float TimeLeft = 0.f;
	if(MatchState == MatchState::WaitingToStart) // Timer for Warmup
		TimeLeft = WarmupTime - GetServerTime() + LevelStartTime;
	else if (MatchState == MatchState::InProgress)	// Timer for Inprogress
		TimeLeft = WarmupTime + MatchTime - GetServerTime() + LevelStartTime;
	else if (MatchState == MatchState::Cooldown) // Timer for CoolDown when match ends
		TimeLeft = CooldownTime + WarmupTime + MatchTime - GetServerTime() + LevelStartTime; 
	
	uint32 SecondsLeft = FMath::CeilToInt(TimeLeft);

	if(HasAuthority())
	{
		DFNGameMode = DFNGameMode == nullptr ? Cast<ADFNGameMode>(UGameplayStatics::GetGameMode(this)) : DFNGameMode;
		if(DFNGameMode)
		{
			SecondsLeft = FMath::CeilToInt(DFNGameMode->GetCountdownTime() + LevelStartTime);
		}
	}
	
	if(CountDownInt != SecondsLeft)
	{
		if(MatchState == MatchState::WaitingToStart || MatchState == MatchState::Cooldown)
		{
			SetHUDWarmupCountDown(TimeLeft);
		}
		if(MatchState == MatchState::InProgress)
		{
			SetHUDMatchCountDown(TimeLeft);
		}
	}
	CountDownInt = SecondsLeft;
}

void ADFNPlayerController::ServerRequestServerTime_Implementation(float TimeofClientRequest)
{
	float ServerTimeofReceipt = GetWorld()->GetTimeSeconds();

	ClientReportServerTime(TimeofClientRequest, ServerTimeofReceipt);
}

void ADFNPlayerController::ClientReportServerTime_Implementation(float TimeofCLientRequest,	float TimeServerReceivedClientRequest)
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
	return GetWorld()->GetTimeSeconds() + ClientServerDelta;
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
		HandleMatchHasStarted();
	}
	else if(MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ADFNPlayerController::OnRep_MatchState()
{
	if(MatchState == MatchState::InProgress)
	{
		HandleMatchHasStarted();
	}
	else if(MatchState == MatchState::Cooldown)
	{
		HandleCooldown();
	}
}

void ADFNPlayerController::HandleMatchHasStarted()
{
	GameHUD = GameHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : GameHUD;
	if(GameHUD)
	{
		if(!GameHUD->PlayerOverlay)
		{
			GameHUD->AddPlayerOverlay();
		}
		if(GameHUD->AnnouncementOverlay)
		{
			GameHUD->AnnouncementOverlay->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}

void ADFNPlayerController::HandleCooldown()
{
	GameHUD = GameHUD == nullptr ? Cast<ADFNHUD>(GetHUD()) : GameHUD;
	if(GameHUD)
	{
		GameHUD->PlayerOverlay->RemoveFromParent();
		bool bHUDValid = GameHUD->AnnouncementOverlay && GameHUD->AnnouncementOverlay->AnnouncementText && GameHUD->AnnouncementOverlay->InfoText;
		if(bHUDValid)
		{
			GameHUD->AnnouncementOverlay->SetVisibility(ESlateVisibility::Visible);
			FString AnnouncementText("New Match starts in: ");
			GameHUD->AnnouncementOverlay->InfoText->SetText(FText::FromString(AnnouncementText));

			ADFNGameState* DFNGameState = Cast<ADFNGameState>(UGameplayStatics::GetGameState(this));
			ADFNPlayerState* DFNPlayerState = GetPlayerState<ADFNPlayerState>();
			if(DFNGameState)
			{
				TArray<ADFNPlayerState*> TopScoringPlayers = DFNGameState->TopScoringPlayers;
				FString InfoTextString;
				if(TopScoringPlayers.Num() == 0)
				{
					InfoTextString = FString("There is no Winner.");
				}
				else if(TopScoringPlayers.Num() == 1 && TopScoringPlayers[0] == DFNPlayerState)
				{
					InfoTextString = FString("You are the Winner!");
				}
				else if(TopScoringPlayers.Num() == 1)
				{
					InfoTextString = FString::Printf(TEXT("WINNER \n%s"), *TopScoringPlayers[0]->GetPlayerName());
				}
				else if(TopScoringPlayers.Num() > 1)
				{
					InfoTextString = FString("Tied Game!\n");
					for (auto TiedPlayer : TopScoringPlayers)
					{
						InfoTextString.Append(FString::Printf(TEXT("%s\n"), *TiedPlayer->GetPlayerName()));
					}
				}
				GameHUD->AnnouncementOverlay->InfoText->SetText(FText::FromString(InfoTextString));
			}
		}
	}
	DFNCharacter = Cast<ADFNCharacter>(GetPawn());
	if(DFNCharacter && DFNCharacter->GetCombatComponent())
	{
		DFNCharacter->bDisableGameplay = true;
		DFNCharacter->GetCombatComponent()->FireButtonPressed(false);
	}
}