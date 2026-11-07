// 曲の「ルール変更」の CUSTOM タブ（ModifiersCoreQuest）に Mod が追加する項目を訳す。
//
// ModifiersCoreQuest は、項目を C++ の構造体（CustomModifier / Modifier）で受け取り、static な辞書に持つ。
// パネルを作るときにこの辞書から名前（Name）と説明（Description）を読み、TMP に直接書き込むので、
// BSML-Lite のフックは通らない。そこで、公開されている辞書のシンボルを dlsym で引き、名前と説明を書き換える。
//
// 構造体と辞書の型は ModifiersCoreQuest 0.1.0 の shared/Core/{Modifier,CustomModifier,ModifiersManager}.hpp と同じにしてある。
// 型が変わるとメモリを壊すので、ModifiersCoreQuest を上げたときはヘッダーと見比べること。
#include "Config.hpp"
#include "Translator.hpp"
#include "logging.hpp"

#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "UnityEngine/Color.hpp"
#include "UnityEngine/Sprite.hpp"

#include <dlfcn.h>

#include <map>
#include <optional>
#include <regex>
#include <string>
#include <unordered_map>
#include <vector>

namespace ModifiersCoreQuest {
    struct Modifier {
        std::string Id;
        std::string Name;
        std::string Description;
        UnityW<UnityEngine::Sprite> Icon;
        float Multiplier;
        std::optional<std::vector<std::string>> Categories;
        std::optional<std::vector<std::string>> MutuallyExclusiveCategories;
        std::optional<std::vector<std::string>> MutuallyExclusiveModifiers;
        std::optional<std::vector<std::string>> RequiresModifiers;
        std::optional<std::vector<std::string>> RequiredByModifiers;
    };

    struct CustomModifier : Modifier {
        std::optional<UnityEngine::Color> Color;
        std::optional<UnityEngine::Color> MultiplierColor;
    };
}

namespace JapaneseLocalizer {
    namespace {
        constexpr float Interval = 2.0f;
        constexpr std::string_view Module = "ModifiersCoreQuest";

        using AllModifiersMap = std::unordered_map<std::string, ModifiersCoreQuest::Modifier>;
        using CustomModifiersMap = std::map<std::string, ModifiersCoreQuest::CustomModifier>;
        using PendingModifiersMap = std::unordered_map<std::string, ModifiersCoreQuest::CustomModifier>;

        AllModifiersMap* allModifiers = nullptr;
        CustomModifiersMap* customModifiers = nullptr;
        PendingModifiersMap* pendingModifiers = nullptr;
        bool resolved = false;

        // 本体の項目の翻訳キー（"MODIFIER_NO_FAIL" など）。これは本体の翻訳に任せる
        bool IsGameKey(std::string const& key) {
            static std::regex const pattern("^[A-Z0-9_]+$");
            return std::regex_match(key, pattern);
        }

        void TranslateText(std::string& text, std::string_view kind, std::string const& id) {
            if (text.empty() || IsGameKey(text)) return;
            auto translated = Translate(Module, text);
            if (!translated || *translated == text) return;
            INFO("modifier {} {}: '{}' -> '{}'", id, kind, text, *translated);
            text = *translated;
        }

        void TranslateModifier(ModifiersCoreQuest::Modifier& modifier) {
            TranslateText(modifier.Name, "name", modifier.Id);
            TranslateText(modifier.Description, "description", modifier.Id);
        }

        void Resolve() {
            resolved = true;
            void* handle = dlopen("libModifiersCoreQuest.so", RTLD_NOW | RTLD_NOLOAD);
            if (!handle) {
                INFO("ModifiersCoreQuest is not loaded; custom modifiers will not be translated");
                return;
            }
            allModifiers = static_cast<AllModifiersMap*>(dlsym(handle, "_ZN18ModifiersCoreQuest16ModifiersManager12AllModifiersE"));
            customModifiers = static_cast<CustomModifiersMap*>(dlsym(handle, "_ZN18ModifiersCoreQuest16ModifiersManager23InternalCustomModifiersE"));
            pendingModifiers = static_cast<PendingModifiersMap*>(dlsym(handle, "_ZN18ModifiersCoreQuest16ModifiersManager24InternalPendingModifiersE"));
            INFO("ModifiersCoreQuest maps: all {} custom {} pending {}", (void*) allModifiers, (void*) customModifiers, (void*) pendingModifiers);
        }

        void Tick() {
            if (IsEnabled() && getModConfig().TranslateModifiers.GetValue()) {
                try {
                    if (!resolved) Resolve();
                    if (allModifiers)
                        for (auto& [id, modifier] : *allModifiers) TranslateModifier(modifier);
                    if (customModifiers)
                        for (auto& [id, modifier] : *customModifiers) TranslateModifier(modifier);
                    if (pendingModifiers)
                        for (auto& [id, modifier] : *pendingModifiers) TranslateModifier(modifier);
                } catch (std::exception const& e) {
                    ERROR("modifier translation failed: {}", e.what());
                }
            }
            BSML::MainThreadScheduler::ScheduleAfterTime(Interval, Tick);
        }
    }

    void StartModifierLocalizer() {
        BSML::MainThreadScheduler::ScheduleAfterTime(Interval, Tick);
    }
}
