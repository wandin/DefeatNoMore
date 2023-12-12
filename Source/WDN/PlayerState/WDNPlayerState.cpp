// Fill out your copyright notice in the Description page of Project Settings.


#include "WDNPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "WDN/Character/WdnCharacter.h"
#include "WDN/PlayerController/WDNPlayerController.h"

void AWDNPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AWDNPlayerState, Defeats);
}

void AWDNPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount); // Score variable is outdated on PlayerState, GetScore instead.
	Character = Character == nullptr ? Cast<AWdnCharacter>(GetPawn()) : Character;
	if(Character && Character->Controller)
	{
		Controller = Controller == nullptr ? Cast<AWDNPlayerController>(Character->Controller) : Controller;
		if(Controller)
		{
			Controller->UpdatePlayerScore(GetScore());
		}
	}
}

void AWDNPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	
	Character = Character == nullptr ? Cast<AWdnCharacter>(GetPawn()) : Character;
	if (Character && Character->Controller)
	{
		Controller = Controller == nullptr ? Cast<AWDNPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->UpdatePlayerScore(GetScore());
		}
	}
}

void AWDNPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	Character = Character == nullptr ? Cast<AWdnCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AWDNPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->UpdatePlayerDefeats(Defeats);
		}
	}
}

void AWDNPlayerState::OnRep_Defeats()
{
	Character = Character == nullptr ? Cast<AWdnCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<AWDNPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->UpdatePlayerDefeats(Defeats);
		}
	}
}