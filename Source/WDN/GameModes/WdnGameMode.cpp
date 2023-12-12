// Fill out your copyright notice in the Description page of Project Settings.


#include "WdnGameMode.h"

#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "WDN/PlayerState/WDNPlayerState.h"
#include "WDN/Character/WdnCharacter.h"
#include "WDN/PlayerController/WDNPlayerController.h"

void AWdnGameMode::PlayerEliminated(AWdnCharacter* ElimmedCharacter, AWDNPlayerController* VictimController,
                                    AWDNPlayerController* AttackerController)
{
	if (AttackerController == nullptr || AttackerController->PlayerState == nullptr) return;
	if (VictimController == nullptr || VictimController->PlayerState == nullptr) return;
	AWDNPlayerState* AttackerPlayerState = AttackerController ? Cast<AWDNPlayerState>(AttackerController->PlayerState) : nullptr;
	AWDNPlayerState* VictimPlayerState = VictimController ? Cast<AWDNPlayerState>(VictimController->PlayerState) : nullptr;

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

void AWdnGameMode::RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController)
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