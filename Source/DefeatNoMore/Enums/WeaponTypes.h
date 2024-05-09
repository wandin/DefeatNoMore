#pragma once

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_Pistol 			UMETA(DisplayName = "Pistol"),
	EWT_SMG 			UMETA(DisplayName = "Sub Machine Gun"),
	EWT_AssaultRifle	UMETA(DisplayName = "Assault Rifle"),
	EWT_RocketLauncher 	UMETA(DisplayName = "Rocket Launcher"),
	EWT_MAX				UMETA(DisplayName = "DefaultMAX")
	
};