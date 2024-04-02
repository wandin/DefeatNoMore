// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "LobbyGameMode.generated.h"

/**
 * 
 */
UCLASS()
class DEFEATNOMORE_API ALobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	
	/**
	 * @brief perform actions on player's postLogin,
	 * currently just traveling from lobby to our GameMap if NumberOfPlayers = 4 for development and test reasons
	 * * @param NewPlayer 
	 */
	virtual void PostLogin(APlayerController* NewPlayer) override;
	
};
