// Fill out your copyright notice in the Description page of Project Settings.


#include "WDNHUD.h"

#include "PlayerOverlayWidget.h"
#include "Blueprint/UserWidget.h"

void AWDNHUD::DrawHUD()
{
	Super::DrawHUD();
	
	FVector2d ViewPortSize;
	if(GEngine)
	{
		// GetViewportSize, X and Y divided by 2, stores the center in ViewportCenter
		GEngine->GameViewport->GetViewportSize(ViewPortSize);
		const FVector2d ViewportCenter(ViewPortSize.X / 2.f, ViewPortSize.Y / 2.f);
		
		float CrosshairSpreadScaled = CrosshairSpreadMax * HUDPackage.CrosshairSpread;
		
		// draw our chrosshairs.
		if(HUDPackage.CrosshairsCenter)
		{
			const FVector2d Spread(0.f, 0.f); // center crosshair (dot) doesn't spread.
			DrawCrosshair(HUDPackage.CrosshairsCenter, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if(HUDPackage.CrosshairsRight)
		{
			const FVector2d Spread(CrosshairSpreadScaled, 0.f); // Right, positive 
			DrawCrosshair(HUDPackage.CrosshairsRight, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if(HUDPackage.CrosshairsLeft)
		{
			const FVector2d Spread(-CrosshairSpreadScaled, 0.f); // left negative
			DrawCrosshair(HUDPackage.CrosshairsLeft, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if(HUDPackage.CrosshairsTop)
		{
			const FVector2d Spread(0.f,-CrosshairSpreadScaled); // Top is weirdly negative values -.-
			DrawCrosshair(HUDPackage.CrosshairsTop, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
		if(HUDPackage.CrosshairsBottom)
		{
			const FVector2d Spread(0.f,CrosshairSpreadScaled); // bottom, positive
			DrawCrosshair(HUDPackage.CrosshairsBottom, ViewportCenter, Spread, HUDPackage.CrosshairColor);
		}
	}	
}

void AWDNHUD::BeginPlay()
{
	Super::BeginPlay();
	
	AddPlayerOverlay();
}

void AWDNHUD::AddPlayerOverlay()
{
	APlayerController* PlayerController = GetOwningPlayerController();
	if(PlayerController && PlayerOverlayClass)
	{
		PlayerOverlay = CreateWidget<UPlayerOverlayWidget>(PlayerController, PlayerOverlayClass);
		PlayerOverlay->AddToViewport();
	}
}

void AWDNHUD::DrawCrosshair(UTexture2D* Texture, FVector2d ViewPortCenter, FVector2d Spread, FLinearColor CrosshairColor)
{
	const float TextureWidth = Texture->GetSizeX();
	const float TextureHeight = Texture->GetSizeY();
	const FVector2d TextureDrawPoint(ViewPortCenter.X - (TextureWidth / 2.f) + Spread.X,
									 ViewPortCenter.Y - (TextureHeight / 2.f) + Spread.Y);

	// draw the crosshair texture at the TextureDrawPoint
	DrawTexture(
		Texture,
		TextureDrawPoint.X,
		TextureDrawPoint.Y,
		TextureWidth,
		TextureHeight,
		0.f,
		0.f,
		1.f,
		1.f,
		CrosshairColor
		);
}
