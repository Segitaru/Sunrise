// Copyright Epic Games, Inc. All Rights Reserved.

#include "System/EFAssetManagerStartupJob.h"

#include "EFLogChannels.h"

TSharedPtr<FStreamableHandle> FEFAssetManagerStartupJob::DoJob() const
{
	const double JobStartTime = FPlatformTime::Seconds();

	TSharedPtr<FStreamableHandle> Handle;
	UE_LOG(LogExperienceFramework, Display, TEXT("Startup job \"%s\" starting"), *JobName);
	JobFunc(*this, Handle);

	if (Handle.IsValid())
	{
		Handle->BindUpdateDelegate(
			FStreamableUpdateDelegate::CreateRaw(this, &FEFAssetManagerStartupJob::UpdateSubstepProgressFromStreamable));
		Handle->WaitUntilComplete(0.0f, false);
		Handle->BindUpdateDelegate(FStreamableUpdateDelegate());
	}

	UE_LOG(LogExperienceFramework, Display, TEXT("Startup job \"%s\" took %.2f seconds to complete"), *JobName,
		FPlatformTime::Seconds() - JobStartTime);

	return Handle;
}
