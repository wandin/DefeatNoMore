// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "WDNPlayerController.generated.h"


UCLASS()
class WDN_API AWDNPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	void UpdatePlayerHealth(float Health, float MaxHealth);

protected:
	virtual void BeginPlay() override;

private:
	
	UPROPERTY()
	class AWDNHUD* HUD;
};
