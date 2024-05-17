#pragma once

#define TRACE_LENGTH 80000.f // define our trace length

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	EWT_Pistol 			UMETA(DisplayName = "Pistol"),
	EWT_SMG 			UMETA(DisplayName = "Sub Machine Gun"),
	EWT_ShotGun			UMETA(DisplayName = "ShotGun"),
	EWT_AssaultRifle	UMETA(DisplayName = "Assault Rifle"),
	EWT_SniperRifle		UMETA(DisplayName = "Sniper Rifle"),
	EWT_RocketLauncher 	UMETA(DisplayName = "Rocket Launcher"),
	EWT_GrenadeLauncher	UMETA(DisplayName = "Grenade Launcher"),
	EWT_MAX				UMETA(DisplayName = "DefaultMAX")
	
};