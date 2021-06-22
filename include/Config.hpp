#pragma once
//#include "config-utils/shared/config-utils.hpp"
#include "config-utils-compat.hpp"

DECLARE_CONFIG(ModConfig,

//#ifdef BS__1_16
    CONFIG_VALUE(DecimalsForNumbers, int, "DecimalsForNumbers", 2);
    CONFIG_VALUE(DontEnergy, bool, "DontShowEnergyBar", false);
    CONFIG_VALUE(DontMpCode, bool, "DontShowMpCode", false);
    CONFIG_VALUE(AlwaysMpCode, bool, "AlwaysShowMpCode", false);
    CONFIG_VALUE(AlwaysUpdate, bool, "UpdateOnSongSelect", false);
    CONFIG_VALUE(LastChanged, int, "LastChanged", 0);

    CONFIG_INIT_FUNCTION(
        CONFIG_INIT_VALUE(DecimalsForNumbers);
        CONFIG_INIT_VALUE(DontEnergy);
        CONFIG_INIT_VALUE(DontMpCode);
        CONFIG_INIT_VALUE(AlwaysMpCode);
        CONFIG_INIT_VALUE(AlwaysUpdate);
        CONFIG_INIT_VALUE(LastChanged);
    )
)
//#elif defined(BS__1_13_2)
//    DECLARE_VALUE(DecimalsForNumbers, int, "DecimalsForNumbers", 2);
//    DECLARE_VALUE(DontEnergy, bool, "DontShowEnergyBar", false);
//    DECLARE_VALUE(DontMpCode, bool, "DontShowMpCode", false);
//    DECLARE_VALUE(AlwaysMpCode, bool, "AlwaysShowMpCode", false);
//    DECLARE_VALUE(AlwaysUpdate, bool, "UpdateOnSongSelect", false);
//    DECLARE_VALUE(LastChanged, int, "LastChanged", 0);
//
//    INIT_FUNCTION(
//        INIT_VALUE(DecimalsForNumbers);
//        INIT_VALUE(DontEnergy);
//        INIT_VALUE(DontMpCode);
//        INIT_VALUE(AlwaysMpCode);
//        INIT_VALUE(AlwaysUpdate);
//        INIT_VALUE(LastChanged);
//    )
//)
//#endif