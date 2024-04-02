// Fill out your copyright notice in the Description page of Project Settings.


#include "DFNPlayerState.h"

#include "Net/UnrealNetwork.h"
#include "DefeatNoMore/Character/DFNCharacter.h"
#include "DefeatNoMore/PlayerController/DFNPlayerController.h"

void ADFNPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ADFNPlayerState, Defeats);
}

void ADFNPlayerState::AddToScore(float ScoreAmount)
{
	SetScore(GetScore() + ScoreAmount); // Score variable is outdated on PlayerState, GetScore instead.
	Character = Character == nullptr ? Cast<ADFNCharacter>(GetPawn()) : Character;
	if(Character)
	{
		Controller = Controller == nullptr ? Cast<ADFNPlayerController>(Character->Controller) : Controller;
		if(Controller)
		{
			Controller->UpdatePlayerScore(GetScore());
		}
	}
}

void ADFNPlayerState::OnRep_Score()
{
	Super::OnRep_Score();
	
	Character = Character == nullptr ? Cast<ADFNCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ADFNPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->UpdatePlayerScore(GetScore());
		}
	}
}

void ADFNPlayerState::AddToDefeats(int32 DefeatsAmount)
{
	Defeats += DefeatsAmount;
	Character = Character == nullptr ? Cast<ADFNCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ADFNPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->UpdatePlayerDefeats(Defeats);
		}
	}
}

void ADFNPlayerState::OnRep_Defeats()
{
	Character = Character == nullptr ? Cast<ADFNCharacter>(GetPawn()) : Character;
	if (Character)
	{
		Controller = Controller == nullptr ? Cast<ADFNPlayerController>(Character->Controller) : Controller;
		if (Controller)
		{
			Controller->UpdatePlayerDefeats(Defeats);
		}
	}
}