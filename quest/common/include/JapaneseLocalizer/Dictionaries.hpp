#pragma once

// ゲームの版に依存しない、辞書の読み込みと照合。
// Beat Saber や il2cpp のヘッダーを使わないので、Mac の clang でも試験できる（tests/）。

#include <cstddef>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace JapaneseLocalizer::Core {
    /// 共通辞書の名前
    inline constexpr std::string_view CommonDictionary = "_common";

    class Dictionaries {
    public:
        /// mods.json（正式な ID → 各プラットフォームでの別名）を読む。読めなければ false を返し、error に理由を入れる
        bool LoadModTable(std::string_view json, std::string* error = nullptr);

        /// 辞書ファイルの名前（拡張子なし）から、その辞書の ID を決める。
        /// - 正式な ID と大文字小文字を除いて同じなら、その ID
        /// - 正式な ID と違う古い別名（bsl など）なら std::nullopt（古い版が残したファイルなので読まない）
        /// - mods.json に無い名前なら、そのままの名前（.so 名で置いた利用者の辞書など）
        std::optional<std::string> DictionaryIdForFile(std::string_view stem) const;

        /// 辞書ファイルを読む順の並べ替えのキー。Quest の最初の別名（.so 名）を返す。
        /// 全辞書をまとめた辞書では先に読んだ訳が勝つので、ファイル名が正式な ID に変わっても、
        /// 以前（.so 名のファイル名だったとき）と同じ順に読むために使う
        std::string LoadOrderKey(std::string_view stem) const;

        /// 辞書を足す。同じ ID の辞書があれば見出し語を足し、上書きはしない。読めた見出し語の数を返す（読めなければ std::nullopt）
        std::optional<std::size_t> AddDictionary(std::string const& id, std::string_view json, std::string* error = nullptr);

        /// 辞書を空にする（mods.json の対応表は残す）
        void ClearDictionaries();

        /// 呼び出し元の Mod 名（Quest では .so 名から lib と .so を除いたもの）を、正式な ID にする。
        /// 対応表に無ければ、渡された名前をそのまま返す
        std::string ResolveModule(std::string_view module) const;

        /// 文字列を訳す。その Mod の辞書 → 共通辞書 → 全辞書の順に引く。前後の空白は残す。訳が無ければ std::nullopt
        std::optional<std::string> Translate(std::string_view module, std::string_view text) const;

        std::size_t DictionaryCount() const { return dictionaries.size(); }
        std::size_t EntryCount() const { return entryCount; }

    private:
        struct Template {
            std::regex pattern;
            std::vector<int> slots;
            std::string translation;
        };

        struct Dictionary {
            std::unordered_map<std::string, std::string> exact;
            std::vector<Template> templates;
        };

        static std::string Lower(std::string_view s);
        static std::optional<Template> MakeTemplate(std::string const& key, std::string const& value);
        static void AddEntry(Dictionary& dict, std::string const& key, std::string const& value);
        static std::optional<std::string> LookupIn(Dictionary const& dict, std::string const& text);

        // 正式な ID（小文字にしたもの → 元の表記）
        std::unordered_map<std::string, std::string> ids;
        // 正式な ID（小文字にしたもの → Quest の最初の別名）
        std::unordered_map<std::string, std::string> firstQuestAlias;
        // Quest の別名（小文字にしたもの → 正式な ID）
        std::unordered_map<std::string, std::string> questAliases;

        // 辞書（正式な ID を小文字にしたもの → 辞書）
        std::unordered_map<std::string, Dictionary> dictionaries;
        // 全辞書をまとめたもの（Mod が決まらないときや、その Mod の辞書に無いとき用）
        Dictionary merged;
        std::size_t entryCount = 0;
    };
}
