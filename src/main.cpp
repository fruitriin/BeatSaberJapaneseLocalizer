#include "Config.hpp"
#include "Hooks.hpp"
#include "Translator.hpp"
#include "logging.hpp"

#include "scotland2/shared/loader.hpp"
#include "bsml/shared/BSML.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "HMUI/ViewController.hpp"
#include "UnityEngine/RectTransform.hpp"
#include "UnityEngine/GameObject.hpp"
#include "UnityEngine/UI/LayoutElement.hpp"

static modloader::ModInfo modInfo{MOD_ID, VERSION, 0};

namespace {
    // 縦並びのレイアウトの中で、高さを持った1行のテキストを置く（高さが無いと上下の部品と重なる）
    void AddLine(UnityEngine::Transform* parent, std::string_view text, float fontSize) {
        auto line = BSML::Lite::CreateText(parent, text, TMPro::FontStyles::Normal, fontSize);
        auto layout = line->get_gameObject()->AddComponent<UnityEngine::UI::LayoutElement*>();
        layout->set_preferredHeight(fontSize + 2.0f);
        layout->set_preferredWidth(90.0f);
    }

    void SettingsDidActivate(HMUI::ViewController* self, bool firstActivation, bool, bool) {
        if (!firstActivation) return;
        using namespace BSML::Lite;
        auto container = CreateScrollableSettingsContainer(self->transform);
        auto parent = container->transform;

        // 説明文の行は、縦並びの中で高さを持たずトグルに重なったので、説明はマウスを乗せたときの文に入れる
        auto enabled = CreateToggle(parent, "日本語に置き換える", getModConfig().Enabled.GetValue(), [](bool value) {
            getModConfig().Enabled.SetValue(value);
        });
        AddHoverHint(enabled, "ほかの Mod の UI を、Mods/JapaneseLocalizer/<Mod名>.json の辞書で日本語に置き換えます");
        auto collect = CreateToggle(parent, "未翻訳収集モード", getModConfig().CollectUntranslated.GetValue(), [](bool value) {
            getModConfig().CollectUntranslated.SetValue(value);
        });
        AddHoverHint(collect, "オンにすると、辞書に無い文字列を untranslated.json に、1行に収まらない訳を overflow.json に書き出します");

        auto songName = CreateToggle(parent, "曲名の省略を直す", getModConfig().FixSongNameHeight.GetValue(), [](bool value) {
            getModConfig().FixSongNameHeight.SetValue(value);
        });
        AddHoverHint(songName, "曲一覧で「祝福/The Blessing」のような日本語と英字が混ざる曲名が「祝…」に省略されるのを直します");

        auto modifiers = CreateToggle(parent, "ルール変更の追加項目を訳す", getModConfig().TranslateModifiers.GetValue(), [](bool value) {
            getModConfig().TranslateModifiers.SetValue(value);
        });
        AddHoverHint(modifiers, "曲の「ルール変更」の CUSTOM タブに Mod が追加する項目（Dance Mode など）の名前と説明を訳します。再起動後に反映されます");

        auto font = CreateToggle(parent, "日本語フォントをそろえる", getModConfig().ReplaceFont.GetValue(), [](bool value) {
            getModConfig().ReplaceFont.SetValue(value);
        });
        AddHoverHint(font, "Mods/JapaneseLocalizer/fonts/ に置いたフォント（無ければ端末の Noto Sans CJK）で日本語を表示します。再起動後に反映されます");

        CreateUIButton(parent, "辞書を読み直す", []() { JapaneseLocalizer::LoadDictionaries(); });
        AddLine(parent, "読み直した訳は、画面を作り直したときに反映されます。", 3.0f);
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
    JapaneseLocalizer::InstallCellDiagnostics();
    JapaneseLocalizer::StartLayoutCheck();
    JapaneseLocalizer::StartFontReplacer();
    JapaneseLocalizer::StartModifierLocalizer();

    BSML::Register::RegisterSettingsMenu("日本語化", SettingsDidActivate, false);
    INFO("late_load done");
}
