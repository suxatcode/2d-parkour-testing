// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "side2d0Character.generated.h"

UCLASS(config=Game)
class Aside2d0Character : public ACharacter
{
	GENERATED_BODY()

	/** Side view camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* SideViewCameraComponent;

	/** Camera boom positioning the camera beside the character */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* CameraBoom;

protected:

	/** Called for side to side input */
	void MoveRight(float Val);

	/** Handle touch inputs. */
	void TouchStarted(const ETouchIndex::Type FingerIndex, const FVector Location);

	/** Handle touch stop event. */
	void TouchStopped(const ETouchIndex::Type FingerIndex, const FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* InputComponent) override;
	// End of APawn interface

	void StartJump();
	void StopJump();

	void ApplyWallrunImpulse(EMovementMode movement);
	void ApplyRetourImpulse(EMovementMode movement);

public:
	Aside2d0Character();

	/** Returns SideViewCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetSideViewCameraComponent() const { return SideViewCameraComponent; }
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

	/**
	 * Amount of running speed translated into upward force when running at a
	 * wall and jumping in front of it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", ClampMax="1", UIMax="1"))
	float WallrunSpeedToUpwardForceTransitionRatio;
	/**
	 * Implications:
	 *  - wallrun: distance from wall at which upward force translation occurs
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", ClampMax="500", UIMax="500"))
	float LegLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta=(ClampMin="0", UIMin="0", ClampMax="20", UIMax="20"))
	float TMPRetourUpwardBoost;
};
