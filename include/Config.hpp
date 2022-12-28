#pragma once
#include "config-utils/shared/config-utils.hpp"
//#include "config-utils-compat.hpp"

DECLARE_CONFIG(ModConfig,

    CONFIG_VALUE(DecimalsForNumbers, int, "DecimalsForNumbers", 2);
    CONFIG_VALUE(DontEnergy, bool, "DontShowEnergyBar", false);
    CONFIG_VALUE(DontMpCode, bool, "DontShowMpCode", false);
    CONFIG_VALUE(AlwaysMpCode, bool, "AlwaysShowMpCode", false);
    CONFIG_VALUE(AlwaysUpdate, bool, "UpdateOnSongSelect", false);
    CONFIG_VALUE(LastChanged, int, "LastChanged", 0);
)