// Fill out your copyright notice in the Description page of Project Settings.


#include "DFNGameMode.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "DefeatNoMore/PlayerState/DFNPlayerState.h"
#include "DefeatNoMore/Character/DFNCharacter.h"
#include "DefeatNoMore/PlayerController/DFNPlayerController.h"

void ADFNGameMode::PlayerEliminated(ADFNCharacter* ElimmedCharacter, ADFNPlayerController* VictimController,
                                    ADFNPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	ADFNPlayerState* AttackerPlayerState = AttackerController ? Cast<ADFNPlayerState>(AttackerController->PlayerState) : nullptr;
	ADFNPlayerState* VictimPlayerState = VictimController ? Cast<ADFNPlayerState>(VictimController->PlayerState) : nullptr;

	if (AttackerPlayerState && AttackerPlayerState != VictimPlayerState)
	{
		AttackerPlayerState->AddToScore(1.f);
	}
	if (VictimPlayerState)
	{
		VictimPlayerState->AddToDefeats(1);
	}
	
	if(ElimmedCharacter)
	{
		ElimmedCharacter->Elimination();
	}
}

void ADFNGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
{
	if(ElimmedCharacter)
	{
		ElimmedCharacter->Destroy();
		ElimmedCharacter->Reset();
	}
	if(ElimmedController)
	{
		TArray<AActor*> PlayerStarts;
		UGameplayStatics::GetAllActorsOfClass(this, APlayerStart::StaticClass(), PlayerStarts);
		int32 Selection = FMath::RandRange(0, PlayerStarts.Num() - 1);
		RestartPlayerAtPlayerStart(ElimmedController, PlayerStarts[Selection]);
	}
}