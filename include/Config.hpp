#pragma once

#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig) {
    CONFIG_VALUE(Enabled, bool, "enabled", true);
    CONFIG_VALUE(CollectUntranslated, bool, "collectUntranslated", false);
};
