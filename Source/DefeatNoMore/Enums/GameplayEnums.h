#pragma once

/* ------------------------------------------------------	/*
			/* TURN IN PLACE ENUM */
/**
 * @brief our turn in place states, in order to play TurnInPlace animations in ABP_DFNCharacter
 */
UENUM(BlueprintType)
enum class ETurnInPlace : uint8
{
	ETIP_Left			UMETA(DisplayName = "TurnLeft"),
	ETIP_Right			UMETA(DisplayName = "TurnRight"),
	ETIP_NotTurning 	UMETA(DisplayName = "NotTurning"),

	ETIP_Max 			UMETA(DisplayName = "DefaultMax")
};
/* ------------------------------------------------------	*/

/* ------------------------------------------------------	*/
			/* Combat State ENUM */
UENUM(BlueprintType)
enum class ECombatState : uint8
{
	ECS_Unoccupied UMETA(DisplayName = "Unoccupied"),
	ECS_Reloading UMETA(DisplayName = "Reloading"),

	ECS_MAX UMETA(DisplayName = "DefaultMAX")
};
/* ------------------------------------------------------	*/

UENUM(BlueprintType)
enum class ECameraMode : uint8
{
	ECM_FirstPerson UMETA(DisplayName = "FirstPersonCameraMode"),
	ECM_ThirdPerson UMETA(DisplayName = "ThirdPersonCameraMode"),

	ECM_MAX UMETA(DisplayName = "Default")
};