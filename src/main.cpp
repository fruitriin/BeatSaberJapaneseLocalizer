#include "Config.hpp"
#include "Hooks.hpp"
#include "Translator.hpp"
#include "logging.hpp"

#include "scotland2/shared/loader.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "HMUI/ViewController.hpp"
#include "UnityEngine/RectTransform.hpp"

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

namespace {
    void SettingsDidActivate(HMUI::ViewController* self, bool firstActivation, bool, bool) {
        if (!firstActivation) return;
        using namespace BSML::Lite;
        auto container = CreateScrollableSettingsContainer(self->transform);
        auto parent = container->transform;

        CreateText(parent, "ほかの Mod の UI を、JSON 辞書で日本語に置き換えます。", TMPro::FontStyles::Normal, 3.5f);
        CreateText(parent, "辞書の場所: Mods/JapaneseLocalizer/<Mod名>.json", TMPro::FontStyles::Normal, 3.0f);

        CreateToggle(parent, "日本語に置き換える", getModConfig().Enabled.GetValue(), [](bool value) {
            getModConfig().Enabled.SetValue(value);
        });
        auto collect = CreateToggle(parent, "未翻訳収集モード", getModConfig().CollectUntranslated.GetValue(), [](bool value) {
            getModConfig().CollectUntranslated.SetValue(value);
        });
        AddHoverHint(collect, "オンにすると、辞書に無い文字列を untranslated.json に、1行に収まらない訳を overflow.json に書き出します");

        CreateUIButton(parent, "辞書を読み直す", []() { JapaneseLocalizer::LoadDictionaries(); });
        CreateText(parent, "読み直した訳は、画面を作り直したときに反映されます。", TMPro::FontStyles::Italic, 3.0f);
    }
}

extern "C" __attribute__((visibility("default"))) void setup(CModInfo* info) {
    *info = modInfo.to_c();
    getModConfig().Init(modInfo);
    INFO("setup");
}

extern "C" __attribute__((visibility("default"))) void late_load() {
    il2cpp_functions::Init();
    BSML::Init();

    JapaneseLocalizer::LoadDictionaries();
    JapaneseLocalizer::InstallBSMLHooks();
    JapaneseLocalizer::InstallChatPlexHooks();
    JapaneseLocalizer::StartLayoutCheck();

    BSML::Register::RegisterSettingsMenu("日本語化", SettingsDidActivate, false);
    INFO("late_load done");
}
