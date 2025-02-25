// Copyright Epic Games, Inc. All Rights Reserved.

#include "AudioModuleGameMode.h"
#include "AudioModuleCharacter.h"
#include "UObject/ConstructorHelpers.h"

AAudioModuleGameMode::AAudioModuleGameMode()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
