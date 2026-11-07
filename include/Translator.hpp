#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace JapaneseLocalizer {
    /// 辞書を置くディレクトリ（末尾は '/'）
    std::string const& DictionaryDir();

    /// 辞書を読み直す。起動時と、設定画面のボタンから呼ぶ。
    void LoadDictionaries();

    /// 呼び出し元の Mod 名を、スタックをたどって .so 名から決める。
    /// 例: libsongdownloader.so → "songdownloader"。決められないときは空文字列を返す。
    std::string CallerModule();

    /// 文字列を訳す。訳が無いときは std::nullopt を返す（収集モードなら記録する）。
    /// module が空なら共通辞書と全辞書を探す。
    std::optional<std::string> Translate(std::string_view module, std::string_view text);

    /// 訳した結果から、元の文字列と Mod 名を引く（1行に収まらない訳を調べるため）
    struct TranslationSource {
        std::string module;
        std::string original;
    };
    std::optional<TranslationSource> FindTranslationSource(std::string const& translated);

    /// 1行に収まらない訳の検出を始める（late_load から1回だけ呼ぶ）
    void StartLayoutCheck();

    /// 日本語の字形をそろえるフォントの差し替えを始める（late_load から1回だけ呼ぶ）
    void StartFontReplacer();

    /// 曲の「ルール変更」に Mod が追加する項目を訳し始める（late_load から1回だけ呼ぶ）
    void StartModifierLocalizer();

    /// 翻訳と収集の設定
    bool IsEnabled();
    bool IsCollecting();
}
