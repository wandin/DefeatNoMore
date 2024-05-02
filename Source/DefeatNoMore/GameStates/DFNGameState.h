// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "DFNGameState.generated.h"

class ADFNPlayerState;
/**
 * 
 */
UCLASS()
class DEFEATNOMORE_API ADFNGameState : public AGameState
{
	GENERATED_BODY()
	
public:

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	void UpdateTopScore(ADFNPlayerState* ScoringPlayer);
	
	UPROPERTY(Replicated)
	TArray<ADFNPlayerState*> TopScoringPlayers;

private:

	float TopScore = 0.f;
};
