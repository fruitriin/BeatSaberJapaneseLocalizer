#pragma once

#include "config-utils/shared/config-utils.hpp"

DECLARE_CONFIG(ModConfig) {
    CONFIG_VALUE(Enabled, bool, "enabled", true);
    CONFIG_VALUE(CollectUntranslated, bool, "collectUntranslated", false);
    // 日本語の字形を、指定したフォントでそろえる
    // 曲一覧の曲名欄の高さを広げ、日本語と英字が混ざる曲名が「…」に省略されるのを防ぐ
    CONFIG_VALUE(FixSongNameHeight, bool, "fixSongNameHeight", true);
    CONFIG_VALUE(ReplaceFont, bool, "replaceFont", true);
    // フォントファイルのパス。空なら fonts/ フォルダーの最初のファイル、それも無ければ端末の Noto Sans CJK を使う
    CONFIG_VALUE(FontFile, std::string, "fontFile", "");
};
