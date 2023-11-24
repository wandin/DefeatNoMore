#pragma once

/**
 * @brief our turn in place states, in order to play TurnInPlace animations in ABP_WdnCharacter
 */
UENUM(BlueprintType)
enum class ETurnInPlace : uint8
{
	ETIP_Left			UMETA(DisplayName = "TurnLeft"),
	ETIP_Right			UMETA(DisplayName = "TurnRight"),
	ETIP_NotTurning 	UMETA(DisplayName = "NotTurning"),

	ETIP_Max 			UMETA(DisplayName = "DefaultMax")
};
