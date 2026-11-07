#pragma once

#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig) {
    CONFIG_VALUE(Enabled, bool, "enabled", true);
    CONFIG_VALUE(CollectUntranslated, bool, "collectUntranslated", false);
    // 日本語の字形を、指定したフォントでそろえる
    CONFIG_VALUE(ReplaceFont, bool, "replaceFont", true);
    // フォントファイルのパス。空なら fonts/ フォルダーの最初のファイル、それも無ければ端末の Noto Sans CJK を使う
    CONFIG_VALUE(FontFile, std::string, "fontFile", "");
};
