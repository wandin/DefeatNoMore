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

	ADFNGameMode();
	virtual void Tick(float DeltaSeconds) override;
	virtual void PlayerEliminated(class ADFNCharacter* ElimmedCharacter, class ADFNPlayerController* VictimController, ADFNPlayerController* AttackerController);
	virtual void RequestRespawn(ACharacter* ElimmedCharacter, AController* ElimmedController);



	UPROPERTY(EditDefaultsOnly)
	float WarmupTime = 5.0f;

	float LevelStartingTime = 0.f;

protected:
	virtual void BeginPlay() override;
	virtual void OnMatchStateSet() override;

private:
	UPROPERTY(EditDefaultsOnly)
	float CountDownTime = 0.f;
};
