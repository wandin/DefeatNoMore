// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "Blueprint/UserWidget.h"

#include "GameFramework/HUD.h"
#include "DFNHUD.generated.h"

class UPlayerOverlayWidget;
class UAnnouncement;
class USniperScope;
class UTexture2D;

/*
 * Struct with textures sfor our crosshairs types.
 */
USTRUCT(BlueprintType)
struct FHUDPackage
{
	GENERATED_BODY()
public:
	UTexture2D* CrosshairsCenter;
	UTexture2D* CrosshairsRight;
	UTexture2D* CrosshairsLeft;
	UTexture2D* CrosshairsTop;
	UTexture2D* CrosshairsBottom;
	float CrosshairSpread;

	FLinearColor CrosshairColor;
};

/**
 *  DFNHUD CLASS STARTS here
 */
UCLASS()
class DEFEATNOMORE_API ADFNHUD : public AHUD
{
	GENERATED_BODY()
public:
	virtual void DrawHUD() override;
	virtual void BeginPlay() override;

	/* Player Overlay - with basic player Infos, such as Health, Ammo, Score, Death */
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UUserWidget> PlayerOverlayClass;
	UPROPERTY()
	UPlayerOverlayWidget* PlayerOverlay;
	void AddPlayerOverlay();
	/* End of Player Overlay */

	/* Annoucement Overlay - with info about WarmupTime*/
	UPROPERTY(EditAnywhere, Category = "Player Stats")
	TSubclassOf<UUserWidget> AnnoucementOverlayClass;
	UPROPERTY()
	UAnnouncement* AnnouncementOverlay;
	void AddAnnouncementOverlay();
	/* End od Annoucement Overlay */

private:
	FHUDPackage HUDPackage;

	void DrawCrosshair(UTexture2D* Texture, FVector2d ViewPortCenter, FVector2d Spread, FLinearColor CrosshairColor);

	UPROPERTY(EditAnywhere)
	float CrosshairSpreadMax = 15.f;

public:
	FORCEINLINE void SetHUDPackage(const FHUDPackage& Package) { HUDPackage = Package; }
};
