// Copyright Epic Games, Inc. All Rights Reserved.

#include "side2d0Character.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DrawDebugHelpers.h"

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

	WallrunSpeedToUpwardForceTransitionRatio = 0.5f;
	LegLength = 100.f;
	CharacterWeightKG = 80.f;
	FootSizeCM = 25.f;
	TMPRetourUpwardBoost = 20.f;
	TMPDownScaleAppliedForceByJump = 100.f;

	// Note: The skeletal mesh and anim blueprint references on the Mesh component (inherited from Character) 
	// are set in the derived blueprint asset named MyCharacter (to avoid direct content references in C++)
}

//////////////////////////////////////////////////////////////////////////
// Input

const FName JumpObjectInterference("JumpObjectInterference");

void Aside2d0Character::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &Aside2d0Character::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &Aside2d0Character::StopJump);
	PlayerInputComponent->BindAxis("MoveRight", this, &Aside2d0Character::MoveRight);
	PlayerInputComponent->BindAction("UseHands", IE_Pressed, this, &Aside2d0Character::Grab);
	PlayerInputComponent->BindAction("UseHands", IE_Released, this, &Aside2d0Character::UnGrab);

	PlayerInputComponent->BindTouch(IE_Pressed, this, &Aside2d0Character::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &Aside2d0Character::TouchStopped);

	GetWorld()->DebugDrawTraceTag = JumpObjectInterference;
}


void Aside2d0Character::StartJump() {
	auto movementMode = GetCharacterMovement()->MovementMode;
	ApplyWallrunImpulse(movementMode);
	ApplyRetourImpulse(movementMode);
	ACharacter::Jump();
}
void Aside2d0Character::StopJump() {
	ACharacter::StopJumping();
}

static bool SweepFromActor(FHitResult& result, const AActor* const a, const FVector Offset, float radius) {
	FCollisionQueryParams params;
	params.TraceTag = JumpObjectInterference;
	params.AddIgnoredActor(a);
	return a->GetWorld()->SweepSingleByChannel(
		result, a->GetActorLocation(), a->GetActorLocation() + Offset,
		FQuat::Identity, ECollisionChannel::ECC_WorldStatic, FCollisionShape::MakeSphere(radius),
		params
	);
}
void Aside2d0Character::ApplyRetourImpulse(EMovementMode movement) {
	if (movement != EMovementMode::MOVE_Falling) {
		return;
	}
	auto userDirection = this->LastControlInputVector;
	float behind = -(userDirection.Y / FMath::Abs(userDirection.Y)); // search in movement direction
	FHitResult obj;
	bool hit = SweepFromActor(obj, this, FVector(0, behind * LegLength, 0), FootSizeCM);
	if (hit && obj.bBlockingHit) {
		auto src = obj.Location;
		src.Z -= TMPRetourUpwardBoost;
		float ForceRadius = 200.f;
		float pushOffSrcForce = GetCharacterMovement()->JumpZVelocity; // jump-strength
		DrawDebugSphere(GetWorld(), src, ForceRadius, 16, FColor(255.f, 0.f, 0.f), false, 2.f);
		this->GetCharacterMovement()->AddRadialImpulse(src, ForceRadius, pushOffSrcForce, ERadialImpulseFalloff::RIF_Linear, true);
		if (obj.Component->IsSimulatingPhysics()) {
			auto src2 = GetActorLocation();
			src2.Z += TMPRetourUpwardBoost; // simmulate force in opposite direction
			float objectForce = pushOffSrcForce / TMPDownScaleAppliedForceByJump * GetMass();
			UE_LOG(LogTemp, Warning, TEXT("objectForce(%f) = pushOffSrcForce(%f) / (%f) * GetMass()(%f)"), objectForce, pushOffSrcForce, 1000.f, GetMass());
			obj.Component->AddRadialImpulse(src2, ForceRadius, objectForce, ERadialImpulseFalloff::RIF_Constant, true);
		}
	}
}

void Aside2d0Character::ApplyWallrunImpulse(EMovementMode movement) {
	auto v = this->GetCharacterMovement()->GetLastUpdateVelocity();
	if (v.Y == 0 || movement != EMovementMode::MOVE_Walking) {
		// no horizontal speed || not attached to ground => no wallrun
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("velocity=%s"), *v.ToString());
	float moveDirectionY = (v.Y / FMath::Abs(v.Y)); // search in movement direction
	FHitResult obj;
	bool hit = SweepFromActor(obj, this, FVector(0, moveDirectionY * LegLength, 0), FootSizeCM);
	if (hit && obj.bBlockingHit) {
		//UE_LOG(LogTemp, Warning, TEXT("objectInMoveDir=%s"), *objectInMoveDirection.ToString());
		auto src = GetActorLocation();
		src.Z -= 80.f;
		float ForceRadius = 200.f;
		float WallrunUpForce = 100.f /* boost + leg strength x wall grip */ + WallrunSpeedToUpwardForceTransitionRatio * FMath::Abs(v.Y);
		UE_LOG(LogTemp, Warning, TEXT("WallrunUpForce=%f (= 100.f + %f * %f)"), WallrunUpForce, WallrunSpeedToUpwardForceTransitionRatio, FMath::Abs(v.Y));
		DrawDebugSphere(GetWorld(), src, ForceRadius, 16, FColor(255.f, 0.f, 0.f), false, 2.f);
		this->GetCharacterMovement()->AddRadialImpulse(src, ForceRadius, WallrunUpForce, ERadialImpulseFalloff::RIF_Constant, true);
		if (obj.Component->IsSimulatingPhysics()) {
			auto src2 = GetActorLocation();
			src2.Z += 80.f; // simmulate force in opposite direction
			float objectForce = WallrunUpForce / TMPDownScaleAppliedForceByJump * GetMass();
			obj.Component->AddRadialImpulse(src2, ForceRadius, objectForce, ERadialImpulseFalloff::RIF_Constant, true);
		}
	}
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

void Aside2d0Character::Grab() {
	UE_LOG(LogTemp, Warning, TEXT("pressed 'e'"));
	const auto& m = GetCharacterMovement();
	m->MovementMode = EMovementMode::MOVE_Custom;
}
void Aside2d0Character::UnGrab() {
	UE_LOG(LogTemp, Warning, TEXT("released 'e'"));
	const auto& m = GetCharacterMovement();
	m->MovementMode = EMovementMode::MOVE_Falling;
}

float Aside2d0Character::GetMass() {
	// would like to use GetCapsuleComponent()->GetMass(), but that needs
	// physics simmulation on that capsule enabled
	return CharacterWeightKG;
}
