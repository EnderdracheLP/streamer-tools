#pragma once
#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig,

    CONFIG_VALUE(DecimalsForNumbers, int, "DecimalsForNumbers", 2);
    CONFIG_VALUE(DontEnergy, bool, "DontShowEnergyBar", false);
    CONFIG_VALUE(DontMpCode, bool, "DontShowMpCode", false);
    CONFIG_VALUE(AlwaysMpCode, bool, "AlwaysShowMpCode", false);
    CONFIG_VALUE(AlwaysUpdate, bool, "UpdateOnSongSelect", false);

    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(DecimalsForNumbers);
        CONFIG_INIT_VALUE(DontEnergy);
        CONFIG_INIT_VALUE(DontMpCode);
        CONFIG_INIT_VALUE(AlwaysMpCode);
        CONFIG_INIT_VALUE(AlwaysUpdate);
    )
)