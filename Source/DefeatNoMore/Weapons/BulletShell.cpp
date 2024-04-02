

#include "BulletShell.h"

#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"


ABulletShell::ABulletShell()
{
	PrimaryActorTick.bCanEverTick = false;
	
	BulletShellMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Bullet Shell Mesh"));
	SetRootComponent(BulletShellMesh);

	BulletShellMesh->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	BulletShellMesh->SetSimulatePhysics(true);
	BulletShellMesh->SetEnableGravity(true);
	BulletShellMesh->SetNotifyRigidBodyCollision(true);
	ShellEjectionImpulse = FMath::RandRange(0.f, 10.f);// random impulse

	LifeSpan = FMath::RandRange(1.f, 2.5f); 
}

void ABulletShell::BeginPlay()
{
	Super::BeginPlay();
	BulletShellMesh->OnComponentHit.AddDynamic(this, &ABulletShell::OnHit);
	BulletShellMesh->AddImpulse(GetActorForwardVector()* ShellEjectionImpulse); // add random impulse to the shell
	BulletShellMesh->AddRelativeRotation(GetActorRotation() * ShellEjectionImpulse); // add random rotation to the shell
	SetLifeSpan(LifeSpan); // will be destroyed when our LifeSpan reaches 0.f
}

void ABulletShell::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	FVector NormalImpulse, const FHitResult& Hit)
{
	if(DroppingShellSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, DroppingShellSound, GetActorLocation());
	}
	// deactivates the BodyCollision to play the DroppingShellSound only once.
	BulletShellMesh->SetNotifyRigidBodyCollision(false);
}