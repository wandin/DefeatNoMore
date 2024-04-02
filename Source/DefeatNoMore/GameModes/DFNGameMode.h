// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "DFNGameMode.generated.h"

/**
 * 
 */
UCLASS()
class DEFEATNOMORE_API ADFNGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	virtual void PlayerEliminated(class ADFNCharacter* ElimmedCharacter, class ADFNPlayerController* VictimController, ADFNPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
};
