// ChatPlex SDK（BeatSaberPlus が使う独自 UI）のフック。
// ヘッダーが無いので、libchatplex-sdk-bs.so からマングル名で関数を引いてフックする。
#include "Hooks.hpp"
#include "Translator.hpp"
#include "logging.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "utf8.h"

#include <dlfcn.h>

#include <string>
#include <string_view>
#include <vector>

namespace {
    using JapaneseLocalizer::CallerModule;
    using JapaneseLocalizer::Translate;

    std::optional<std::u16string> TranslateU16(std::u16string_view text, std::string const& module) {
        if (text.empty()) return std::nullopt;
        std::string utf8;
        try {
            utf8::utf16to8(text.begin(), text.end(), std::back_inserter(utf8));
        } catch (...) {
            return std::nullopt;
        }
        auto translated = Translate(module, utf8);
        if (!translated) return std::nullopt;
        std::u16string out;
        utf8::utf8to16(translated->begin(), translated->end(), std::back_inserter(out));
        return out;
    }

    // CText / CPOrSButton / TopNavigationView などの「文字列を1つ受け取る」メソッド共通の処理。
    // 戻り値は this を返す流れるようなインターフェースなので、元の関数の戻り値をそのまま返す。
    template <typename Orig>
    void* CallWithTranslated(Orig orig, void* self, std::u16string_view text) {
        if (auto translated = TranslateU16(text, CallerModule())) {
            return orig(self, std::u16string_view(*translated));
        }
        return orig(self, text);
    }
}

MAKE_HOOK(CText_SetText, nullptr, void*, void* self, std::u16string_view text) {
    return CallWithTranslated(CText_SetText, self, text);
}

MAKE_HOOK(CPOrSButton_SetText, nullptr, void*, void* self, std::u16string_view text) {
    return CallWithTranslated(CPOrSButton_SetText, self, text);
}

MAKE_HOOK(CPOrSButton_SetTooltip, nullptr, void*, void* self, std::u16string_view text) {
    return CallWithTranslated(CPOrSButton_SetTooltip, self, text);
}

MAKE_HOOK(CTextInput_SetPlaceHolder, nullptr, void*, void* self, std::u16string_view text) {
    return CallWithTranslated(CTextInput_SetPlaceHolder, self, text);
}

MAKE_HOOK(TopNavigationView_SetTitle, nullptr, void*, void* self, std::u16string_view text) {
    return CallWithTranslated(TopNavigationView_SetTitle, self, text);
}

// タブの見出し。選ばれたタブは番号で扱われるので、表示の文字列だけを訳してよい
MAKE_HOOK(CTextSegmentedControl_SetTexts, nullptr, void*, void* self, std::vector<std::u16string> const& texts) {
    auto module = CallerModule();
    std::vector<std::u16string> translated;
    translated.reserve(texts.size());
    bool changed = false;
    for (auto const& text : texts) {
        if (auto t = TranslateU16(text, module)) {
            translated.push_back(std::move(*t));
            changed = true;
        } else {
            translated.push_back(text);
        }
    }
    return CTextSegmentedControl_SetTexts(self, changed ? translated : texts);
}

namespace JapaneseLocalizer {
    void InstallChatPlexHooks() {
        static constexpr auto logger = Paper::ConstLoggerContext(MOD_ID "_ChatPlexHooks");

        void* handle = dlopen("libchatplex-sdk-bs.so", RTLD_NOW | RTLD_NOLOAD);
        if (!handle) {
            WARNING("libchatplex-sdk-bs.so is not loaded; ChatPlex UI will not be translated");
            return;
        }

        auto find = [&](char const* symbol) -> void* {
            void* addr = dlsym(handle, symbol);
            if (!addr) ERROR("symbol not found: {}", symbol);
            return addr;
        };

#define INSTALL_CP_HOOK(name, symbol)                     \
        if (void* addr = find(symbol)) {                  \
            INSTALL_HOOK_DIRECT(logger, name, addr);      \
        }

        INSTALL_CP_HOOK(CText_SetText,
            "_ZN6CP_SDK2UI10Components5CText7SetTextENSt6__ndk117basic_string_viewIDsNS3_11char_traitsIDsEEEE");
        INSTALL_CP_HOOK(CPOrSButton_SetText,
            "_ZN6CP_SDK2UI10Components11CPOrSButton7SetTextENSt6__ndk117basic_string_viewIDsNS3_11char_traitsIDsEEEE");
        INSTALL_CP_HOOK(CPOrSButton_SetTooltip,
            "_ZN6CP_SDK2UI10Components11CPOrSButton10SetTooltipENSt6__ndk117basic_string_viewIDsNS3_11char_traitsIDsEEEE");
        INSTALL_CP_HOOK(CTextInput_SetPlaceHolder,
            "_ZN6CP_SDK2UI10Components10CTextInput14SetPlaceHolderENSt6__ndk117basic_string_viewIDsNS3_11char_traitsIDsEEEE");
        INSTALL_CP_HOOK(TopNavigationView_SetTitle,
            "_ZN6CP_SDK2UI5Views17TopNavigationView8SetTitleENSt6__ndk117basic_string_viewIDsNS3_11char_traitsIDsEEEE");
        INSTALL_CP_HOOK(CTextSegmentedControl_SetTexts,
            "_ZN6CP_SDK2UI10Components21CTextSegmentedControl8SetTextsERKNSt6__ndk16vectorINS3_12basic_stringIDsNS3_11char_traitsIDsEENS3_9allocatorIDsEEEENS8_ISA_EEEE");
#undef INSTALL_CP_HOOK
    }
}
