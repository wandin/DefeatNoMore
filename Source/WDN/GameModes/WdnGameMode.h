// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "WdnGameMode.generated.h"

/**
 * 
 */
UCLASS()
class WDN_API AWdnGameMode : public AGameMode
{
	GENERATED_BODY()

public:

	virtual void PlayerEliminated(class AWdnCharacter* ElimmedCharacter, class AWDNPlayerController* VictimController, AWDNPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);
};
