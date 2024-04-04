

#include "DFNAnimInstance.h"

#include "DefeatNoMore/Character/DFNCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "DefeatNoMore/Weapons/Weapon.h"

void UDFNAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	DFNCharacter = Cast<ADFNCharacter>(TryGetPawnOwner());
}

void UDFNAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if(!IsValid(DFNCharacter))
	{
		DFNCharacter = Cast<ADFNCharacter>(TryGetPawnOwner());
	}
	if(!IsValid(DFNCharacter)) return;

	FVector Velocity = DFNCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = DFNCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = DFNCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bIsCrouched = DFNCharacter->bIsCrouched;
	bIsWalking = DFNCharacter->GetIsWalking();
	TurnInPlace = DFNCharacter->GetTurnInPlace();
	bIsFiring = DFNCharacter->GetIsFiring();
	bRotateRootBone = DFNCharacter->ShouldRotateRootBone();
	bEliminated = DFNCharacter->IsEliminated();

	//Offsets
	const FRotator AimRotation = DFNCharacter->GetBaseAimRotation();
	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(DFNCharacter->GetVelocity());
	YawOffSet = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	
	//AO - AIM OffSets
	AO_Yaw = DFNCharacter->GetAO_Yaw(); // for turn in place
	AO_Pitch = DFNCharacter->GetAO_Pitch();

	// Weapons
	EquippedWeapon = DFNCharacter->GetEquippedWeapon(); // get equipped weapon
	if(IsValid(EquippedWeapon) && EquippedWeapon->GetWeaponMesh() && DFNCharacter->GetMesh()) //check objects and meshs
	{
		// set our LeftHandSocket transform to world's transform on the weapon
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
		FVector OutPosition; // needed to TransformToBoneSpace
		FRotator OutRotation; // needed to TransformToBoneSpace
		// transform to Bone Space from World space
		DFNCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition); // set LeftHandTransform to our Vector
		LeftHandTransform.SetRotation(FQuat(OutRotation)); // set LeftHandTransform to our Vector - fRotator as fquat
				
		if(DFNCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			const FTransform RightHandTransform = DFNCharacter->GetMesh()->GetSocketTransform(FName("hand_r"), RTS_World);
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - DFNCharacter->GetHitTarget()));
			
			RightHandRotation = FMath::RInterpTo(RightHandRotation,LookAtRotation, DeltaSeconds, 13.5f);
		}
	}
	bUseFrabrik = DFNCharacter->GetCombatState() != ECombatState::ECS_Reloading;
	bUseAimOffsets = DFNCharacter->GetCombatState() != ECombatState::ECS_Reloading;
	bTransformRightHand = DFNCharacter->GetCombatState() != ECombatState::ECS_Reloading;
}