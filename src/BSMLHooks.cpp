// BSML（.bsml の読み込み）と BSML-Lite（コードから部品を作る API）のフック
#include "Hooks.hpp"
#include "Translator.hpp"
#include "logging.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "bsml/shared/BSML-Lite.hpp"
#include "bsml/shared/BSML/Parsing/BSMLParser.hpp"
#include "tinyxml2/shared/tinyxml2.h"

#include <array>

using namespace BSML::Lite;
using UnityEngine::Vector2;
using UnityEngine::Vector3;

namespace {
    using JapaneseLocalizer::CallerModule;
    using JapaneseLocalizer::Translate;

    // 表示される文字列を持つ属性。choices / options / value / id などは値が変わると動作が壊れるので訳さない
    constexpr std::array<std::string_view, 10> TranslatableAttributes = {
        "text", "hover-hint", "tab-name", "header", "title", "subtitle", "label", "on-text", "off-text", "placeholder",
    };

    bool TranslateElement(tinyxml2::XMLElement* element, std::string const& module) {
        bool changed = false;
        for (auto name : TranslatableAttributes) {
            auto* value = element->Attribute(std::string(name).c_str());
            if (!value || !*value || value[0] == '~') continue;
            if (auto translated = Translate(module, value)) {
                element->SetAttribute(std::string(name).c_str(), translated->c_str());
                changed = true;
            }
        }
        for (auto* child = element->FirstChildElement(); child; child = child->NextSiblingElement()) {
            changed |= TranslateElement(child, module);
        }
        return changed;
    }

    std::optional<std::string> TranslateBSML(std::string_view content, std::string const& module) {
        tinyxml2::XMLDocument doc;
        if (doc.Parse(content.data(), content.size()) != tinyxml2::XML_SUCCESS) return std::nullopt;
        bool changed = false;
        for (auto* element = doc.FirstChildElement(); element; element = element->NextSiblingElement()) {
            changed |= TranslateElement(element, module);
        }
        if (!changed) return std::nullopt;
        tinyxml2::XMLPrinter printer;
        doc.Print(&printer);
        return std::string(printer.CStr(), printer.CStrSize() > 0 ? printer.CStrSize() - 1 : 0);
    }

    // StringW の中身を訳す。訳が無ければそのまま返す
    StringW TranslateStringW(StringW str, std::string const& module) {
        if (!str) return str;
        std::string text = static_cast<std::string>(str);
        if (text.empty() || text[0] == '~') return str;
        if (auto translated = Translate(module, text)) return StringW(*translated);
        return str;
    }
}

MAKE_HOOK(BSMLParser_parse, nullptr, std::shared_ptr<BSML::BSMLParser>, std::string_view str) {
    auto module = CallerModule();
    if (auto translated = TranslateBSML(str, module)) {
        DEBUG("translated BSML from {}", module);
        return BSMLParser_parse(*translated);
    }
    return BSMLParser_parse(str);
}

MAKE_HOOK(Lite_CreateText, nullptr, HMUI::CurvedTextMeshPro*, TransformWrapper const& parent, StringW text, TMPro::FontStyles fontStyle, float fontSize, Vector2 anchoredPosition, Vector2 sizeDelta) {
    return Lite_CreateText(parent, TranslateStringW(text, CallerModule()), fontStyle, fontSize, anchoredPosition, sizeDelta);
}

MAKE_HOOK(Lite_CreateUIButton_Template, nullptr, UnityEngine::UI::Button*, TransformWrapper const& parent, StringW buttonText, std::string_view const& buttonTemplate, Vector2 anchoredPosition, Vector2 sizeDelta, std::function<void()> onClick) {
    return Lite_CreateUIButton_Template(parent, TranslateStringW(buttonText, CallerModule()), buttonTemplate, anchoredPosition, sizeDelta, std::move(onClick));
}

MAKE_HOOK(Lite_CreateUIButton, nullptr, UnityEngine::UI::Button*, TransformWrapper const& parent, StringW buttonText, Vector2 anchoredPosition, Vector2 sizeDelta, std::function<void()> onClick) {
    return Lite_CreateUIButton(parent, TranslateStringW(buttonText, CallerModule()), anchoredPosition, sizeDelta, std::move(onClick));
}

MAKE_HOOK(Lite_SetButtonText, nullptr, void, UnityEngine::UI::Button* button, StringW text) {
    Lite_SetButtonText(button, TranslateStringW(text, CallerModule()));
}

MAKE_HOOK(Lite_AddHoverHint, nullptr, HMUI::HoverHint*, GameObjectWrapper const& parent, StringW text) {
    return Lite_AddHoverHint(parent, TranslateStringW(text, CallerModule()));
}

MAKE_HOOK(Lite_CreateToggle, nullptr, BSML::ToggleSetting*, TransformWrapper const& parent, StringW text, bool currentValue, Vector2 anchoredPosition, std::function<void(bool)> onToggle) {
    return Lite_CreateToggle(parent, TranslateStringW(text, CallerModule()), currentValue, anchoredPosition, std::move(onToggle));
}

MAKE_HOOK(Lite_CreateSliderSetting, nullptr, BSML::SliderSetting*, TransformWrapper const& parent, StringW label, float increment, float currentValue, float minValue, float maxValue, float applyValueTime, Vector2 anchoredPosition, std::function<void(float)> onValueChange) {
    return Lite_CreateSliderSetting(parent, TranslateStringW(label, CallerModule()), increment, currentValue, minValue, maxValue, applyValueTime, anchoredPosition, std::move(onValueChange));
}

MAKE_HOOK(Lite_CreateSliderSetting_Buttons, nullptr, BSML::SliderSetting*, TransformWrapper const& parent, StringW label, float increment, float currentValue, float minValue, float maxValue, float applyValueTime, bool showButtons, Vector2 anchoredPosition, std::function<void(float)> onValueChange) {
    return Lite_CreateSliderSetting_Buttons(parent, TranslateStringW(label, CallerModule()), increment, currentValue, minValue, maxValue, applyValueTime, showButtons, anchoredPosition, std::move(onValueChange));
}

MAKE_HOOK(Lite_CreateIncrementSetting, nullptr, BSML::IncrementSetting*, TransformWrapper const& parent, StringW label, int decimals, float increment, float currentValue, bool hasMin, bool hasMax, float minValue, float maxValue, Vector2 anchoredPosition, std::function<void(float)> onValueChange) {
    return Lite_CreateIncrementSetting(parent, TranslateStringW(label, CallerModule()), decimals, increment, currentValue, hasMin, hasMax, minValue, maxValue, anchoredPosition, std::move(onValueChange));
}

MAKE_HOOK(Lite_CreateStringSetting, nullptr, HMUI::InputFieldView*, TransformWrapper const& parent, StringW settingsName, StringW currentValue, Vector2 anchoredPosition, Vector3 keyboardPositionOffset, std::function<void(StringW)> onValueChange) {
    return Lite_CreateStringSetting(parent, TranslateStringW(settingsName, CallerModule()), currentValue, anchoredPosition, keyboardPositionOffset, std::move(onValueChange));
}

// 選択肢（values）は値そのものなので訳さない。ラベルだけを訳す
MAKE_HOOK(Lite_CreateDropdown, nullptr, BSML::DropdownListSetting*, TransformWrapper const& parent, StringW label, StringW currentValue, std::span<std::string_view> values, std::function<void(StringW)> onValueChange) {
    return Lite_CreateDropdown(parent, TranslateStringW(label, CallerModule()), currentValue, values, std::move(onValueChange));
}

namespace JapaneseLocalizer {
    void InstallBSMLHooks() {
        static constexpr auto logger = Paper::ConstLoggerContext(MOD_ID "_BSMLHooks");

        INSTALL_HOOK_DIRECT(logger, BSMLParser_parse, (void*) static_cast<std::shared_ptr<BSML::BSMLParser> (*)(std::string_view)>(&BSML::BSMLParser::parse));

        INSTALL_HOOK_DIRECT(logger, Lite_CreateText,
            (void*) static_cast<HMUI::CurvedTextMeshPro* (*)(TransformWrapper const&, StringW, TMPro::FontStyles, float, Vector2, Vector2)>(&CreateText));
        INSTALL_HOOK_DIRECT(logger, Lite_CreateUIButton_Template,
            (void*) static_cast<UnityEngine::UI::Button* (*)(TransformWrapper const&, StringW, std::string_view const&, Vector2, Vector2, std::function<void()>)>(&CreateUIButton));
        INSTALL_HOOK_DIRECT(logger, Lite_CreateUIButton,
            (void*) static_cast<UnityEngine::UI::Button* (*)(TransformWrapper const&, StringW, Vector2, Vector2, std::function<void()>)>(&CreateUIButton));
        INSTALL_HOOK_DIRECT(logger, Lite_SetButtonText, (void*) &SetButtonText);
        INSTALL_HOOK_DIRECT(logger, Lite_AddHoverHint, (void*) &AddHoverHint);
        INSTALL_HOOK_DIRECT(logger, Lite_CreateToggle,
            (void*) static_cast<BSML::ToggleSetting* (*)(TransformWrapper const&, StringW, bool, Vector2, std::function<void(bool)>)>(&CreateToggle));
        INSTALL_HOOK_DIRECT(logger, Lite_CreateSliderSetting,
            (void*) static_cast<BSML::SliderSetting* (*)(TransformWrapper const&, StringW, float, float, float, float, float, Vector2, std::function<void(float)>)>(&CreateSliderSetting));
        INSTALL_HOOK_DIRECT(logger, Lite_CreateSliderSetting_Buttons,
            (void*) static_cast<BSML::SliderSetting* (*)(TransformWrapper const&, StringW, float, float, float, float, float, bool, Vector2, std::function<void(float)>)>(&CreateSliderSetting));
        INSTALL_HOOK_DIRECT(logger, Lite_CreateIncrementSetting,
            (void*) static_cast<BSML::IncrementSetting* (*)(TransformWrapper const&, StringW, int, float, float, bool, bool, float, float, Vector2, std::function<void(float)>)>(&CreateIncrementSetting));
        INSTALL_HOOK_DIRECT(logger, Lite_CreateStringSetting,
            (void*) static_cast<HMUI::InputFieldView* (*)(TransformWrapper const&, StringW, StringW, Vector2, Vector3, std::function<void(StringW)>)>(&CreateStringSetting));
        INSTALL_HOOK_DIRECT(logger, Lite_CreateDropdown,
            (void*) static_cast<BSML::DropdownListSetting* (*)(TransformWrapper const&, StringW, StringW, std::span<std::string_view>, std::function<void(StringW)>)>(&CreateDropdown));
    }
}
