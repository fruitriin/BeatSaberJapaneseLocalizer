// 曲の「ルール変更」に Mod が追加する項目（ModifiersCoreQuest の CUSTOM タブ）を訳す。
// ModifiersCoreQuest は GameplayModifierParamsSO の名前と説明の「翻訳キー」を、そのまま TMP や HoverTextSetter に渡して表示する。
// BeatLeader などはこのキーに英語の名前そのもの（"Dance Mode" など）を入れているので、項目のデータを日本語に書き換える。
#include "Config.hpp"
#include "Translator.hpp"
#include "logging.hpp"

#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "GlobalNamespace/GameplayModifierParamsSO.hpp"
#include "UnityEngine/Resources.hpp"

#include <regex>
#include <set>

namespace JapaneseLocalizer {
    namespace {
        constexpr float Interval = 2.0f;
        constexpr std::string_view Module = "ModifiersCoreQuest";
        std::set<int> processed;

        // 本体の項目の翻訳キー（"MODIFIER_NO_FAIL" など）。これは本体の翻訳に任せる
        bool IsGameKey(std::string const& key) {
            static std::regex const pattern("^[A-Z0-9_]+$");
            return std::regex_match(key, pattern);
        }

        void TranslateField(GlobalNamespace::GameplayModifierParamsSO* params, bool description) {
            StringW value = description ? params->_descriptionLocalizationKey : params->_modifierNameLocalizationKey;
            if (!value) return;
            std::string text = value;
            if (text.empty() || IsGameKey(text)) return;
            auto translated = Translate(Module, text);
            if (!translated) return;
            if (description)
                params->_descriptionLocalizationKey = StringW(*translated);
            else
                params->_modifierNameLocalizationKey = StringW(*translated);
            INFO("modifier {}: '{}' -> '{}'", description ? "description" : "name", text, *translated);
        }

        void Tick() {
            if (IsEnabled() && getModConfig().TranslateModifiers.GetValue()) {
                try {
                    auto all = UnityEngine::Resources::FindObjectsOfTypeAll(csTypeOf(GlobalNamespace::GameplayModifierParamsSO*));
                    for (auto obj : all) {
                        auto params = reinterpret_cast<GlobalNamespace::GameplayModifierParamsSO*>(obj.ptr());
                        if (!params || !processed.insert(params->GetInstanceID()).second) continue;
                        TranslateField(params, false);
                        TranslateField(params, true);
                    }
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
