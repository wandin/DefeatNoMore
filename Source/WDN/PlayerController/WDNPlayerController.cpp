// Fill out your copyright notice in the Description page of Project Settings.


#include "WDNPlayerController.h"

#include "EngineUtils.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "WDN/HUD/PlayerOverlayWidget.h"
#include "WDN/HUD/WDNHUD.h"


void AWDNPlayerController::BeginPlay()
{
	Super::BeginPlay();

	HUD = Cast<AWDNHUD>(GetHUD());
}

void AWDNPlayerController::UpdatePlayerHealth(float Health, float MaxHealth)
{
	HUD = HUD == nullptr ? Cast<AWDNHUD>(GetHUD()) : HUD;

	bool bHUDValid = HUD && HUD->PlayerOverlay && HUD->PlayerOverlay->HealthText &&
				HUD->PlayerOverlay->HealthBar;
	if(bHUDValid)
	{
		const float HealthPercentage = Health / MaxHealth;
		HUD->PlayerOverlay->HealthBar->SetPercent(HealthPercentage);

		const FString HealthText = FString::Printf(TEXT("%d"), FMath::CeilToInt(Health));
		HUD->PlayerOverlay->HealthText->SetText(FText::FromString(HealthText));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("NOT VALID HUD"));
	}
}