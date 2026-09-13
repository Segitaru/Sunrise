#pragma once

#include "CoreMinimal.h"

namespace OverloadTeamIds
{
	inline constexpr int32 Neutral = 0;
	inline constexpr int32 InitialPlayer = 1;
	inline constexpr int32 FirstOpponent = 2;

	inline bool IsPlayable(int32 TeamId)
	{
		return TeamId >= InitialPlayer && TeamId < MAX_uint8;
	}
} // namespace OverloadTeamIds
