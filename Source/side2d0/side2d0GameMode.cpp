// Copyright Epic Games, Inc. All Rights Reserved.

#include "side2d0GameMode.h"
#include "side2d0Character.h"
#include "UObject/ConstructorHelpers.h"

Aside2d0GameMode::Aside2d0GameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/SideScrollerCPP/Blueprints/SideScrollerCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
