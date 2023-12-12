

#include "WdnAnimInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "WDN/Weapons/Weapon.h"

void UWdnAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	WdnCharacter = Cast<AWdnCharacter>(TryGetPawnOwner());
}

void UWdnAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if(!IsValid(WdnCharacter))
	{
		WdnCharacter = Cast<AWdnCharacter>(TryGetPawnOwner());
	}
	if(!IsValid(WdnCharacter)) return;

	FVector Velocity = WdnCharacter->GetVelocity();
	Velocity.Z = 0.f;
	Speed = Velocity.Size();

	bIsInAir = WdnCharacter->GetCharacterMovement()->IsFalling();
	bIsAccelerating = WdnCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f ? true : false;
	bIsCrouched = WdnCharacter->bIsCrouched;
	bIsWalking = WdnCharacter->GetIsWalking();
	TurnInPlace = WdnCharacter->GetTurnInPlace();
	bIsFiring = WdnCharacter->GetIsFiring();
	bRotateRootBone = WdnCharacter->ShouldRotateRootBone();
	bEliminated = WdnCharacter->IsEliminated();

	//Offsets
	const FRotator AimRotation = WdnCharacter->GetBaseAimRotation();
	const FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(WdnCharacter->GetVelocity());
	YawOffSet = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
	
	//AO - AIM OffSets
	AO_Yaw = WdnCharacter->GetAO_Yaw(); // for turn in place
	AO_Pitch = WdnCharacter->GetAO_Pitch();

	// Weapons
	EquippedWeapon = WdnCharacter->GetEquippedWeapon(); // get equipped weapon
	if(IsValid(EquippedWeapon) && EquippedWeapon->GetWeaponMesh() && WdnCharacter->GetMesh()) //check objects and meshs
	{
		// set our LeftHandSocket transform to world's transform on the weapon
		LeftHandTransform = EquippedWeapon->GetWeaponMesh()->GetSocketTransform(FName("LeftHandSocket"), RTS_World);
		FVector OutPosition; // needed to TransformToBoneSpace
		FRotator OutRotation; // needed to TransformToBoneSpace
		// transform to Bone Space from World space
		WdnCharacter->GetMesh()->TransformToBoneSpace(FName("hand_r"), LeftHandTransform.GetLocation(), FRotator::ZeroRotator, OutPosition, OutRotation);
		LeftHandTransform.SetLocation(OutPosition); // set LeftHandTransform to our Vector
		LeftHandTransform.SetRotation(FQuat(OutRotation)); // set LeftHandTransform to our Vector - fRotator as fquat
				
		if(WdnCharacter->IsLocallyControlled())
		{
			bLocallyControlled = true;
			const FTransform RightHandTransform = WdnCharacter->GetMesh()->GetSocketTransform(FName("hand_r"), RTS_World);
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(RightHandTransform.GetLocation(), RightHandTransform.GetLocation() + (RightHandTransform.GetLocation() - WdnCharacter->GetHitTarget()));
			
			RightHandRotation = FMath::RInterpTo(RightHandRotation,LookAtRotation, DeltaSeconds, 13.5f);
		}
	}
}

