// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "DFNPlayerState.generated.h"

/**
 * 
 */
UCLASS()
class DEFEATNOMORE_API ADFNPlayerState : public APlayerState
{
	GENERATED_BODY()


public:
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void OnRep_Score() override;

	UFUNCTION()
	virtual void OnRep_Defeats();

	void AddToScore(float ScoreAmount);
	void AddToDefeats(int32 DefeatsAmount);

private:

	UPROPERTY()
	class ADFNCharacter* Character;
	
	UPROPERTY()
	class ADFNPlayerController* Controller;

	UPROPERTY(ReplicatedUsing = OnRep_Defeats)
	int32 Defeats;
};