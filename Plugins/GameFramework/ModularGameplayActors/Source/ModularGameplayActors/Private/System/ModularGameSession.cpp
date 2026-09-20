// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/ModularGameSession.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ModularGameSession)

AModularGameSession::AModularGameSession(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

bool AModularGameSession::ProcessAutoLogin()
{
	// This is actually handled in LyraGameMode::TryDedicatedServerLogin
	return true;
}

void AModularGameSession::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();
}

void AModularGameSession::HandleMatchHasEnded()
{
	Super::HandleMatchHasEnded();
}

