// Copyright Epic Games, Inc. All Rights Reserved.

#include "side2d0Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

Aside2d0Character::Aside2d0Character()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	// Don't rotate when the controller rotates.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// Create a camera boom attached to the root (capsule)
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Rotation of the character should not affect rotation of boom
	CameraBoom->bDoCollisionTest = false;
	CameraBoom->TargetArmLength = 500.f;
	CameraBoom->SocketOffset = FVector(0.f,0.f,75.f);
	CameraBoom->SetRelativeRotation(FRotator(0.f,180.f,0.f));

	// Create a camera and attach to boom
	SideViewCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("SideViewCamera"));
	SideViewCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	SideViewCameraComponent->bUsePawnControlRotation = false; // We don't want the controller rotating the camera

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Face in the direction we are moving..
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 720.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->GravityScale = 2.f;
	GetCharacterMovement()->AirControl = 0.10f;
	GetCharacterMovement()->JumpZVelocity = 1000.f;
	GetCharacterMovement()->GroundFriction = 3.f;
	GetCharacterMovement()->MaxWalkSpeed = 600.f;
	GetCharacterMovement()->MaxFlySpeed = 600.f;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

void Aside2d0Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	UE_LOG(LogTemp, Warning, TEXT("XXX123"));
	// set up gameplay key bindings
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &Aside2d0Character::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &Aside2d0Character::StopJump);
	PlayerInputComponent->BindAxis("MoveRight", this, &Aside2d0Character::MoveRight);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &Aside2d0Character::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &Aside2d0Character::TouchStopped);
}


void Aside2d0Character::StartJump() {
	auto v = this->GetCharacterMovement()->GetLastUpdateVelocity();
	UE_LOG(LogTemp, Warning, TEXT("velocity=%s"), *v.ToString());
	FHitResult objectInMoveDirection;
	auto EndSearch = GetActorLocation();
	float legLength = 500.0f; // very high for testing
	EndSearch.Y += legLength;
	auto StartSearch = GetActorLocation();
	StartSearch.X += 30.0f; // get some offset so we dont hit the character mesh components while sweeping, there must be a better way..
	StartSearch.Y += 30.0f;
	StartSearch.Z += 30.0f;
	GetWorld()->SweepSingleByChannel(
		objectInMoveDirection, StartSearch, EndSearch, FQuat::Identity, ECollisionChannel::ECC_PhysicsBody, FCollisionShape::MakeSphere(25.0f), FCollisionObjectQueryParams::AllObjects
	);
	UE_LOG(LogTemp, Warning, TEXT("objectInMoveDir=%s"), *objectInMoveDirection.ToString());
	//auto v2 = AActor::GetActorForwardVector();
	//UE_LOG(LogTemp, Warning, TEXT("forward=%s"), *v2.ToString());
	ACharacter::Jump();
}
void Aside2d0Character::StopJump() {
	ACharacter::StopJumping();
}

void Aside2d0Character::MoveRight(float Value)
{
	// add movement in that direction
	AddMovementInput(FVector(0.f,-1.f,0.f), Value);
}

void Aside2d0Character::TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	// jump on any touch
	Jump();
}

void Aside2d0Character::TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location)
{
	StopJumping();
}

