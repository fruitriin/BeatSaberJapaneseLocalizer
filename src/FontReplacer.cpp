// 日本語の字形を1つのフォントにそろえる。
// フォントファイルから実行時に Dynamic な TMP フォントを作り、ゲーム内の全フォントのフォールバックの先頭に入れる。
#include "Config.hpp"
#include "Translator.hpp"
#include "logging.hpp"

#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "System/Collections/Generic/List_1.hpp"
#include "TMPro/AtlasPopulationMode.hpp"
#include "TMPro/ShaderUtilities.hpp"
#include "TMPro/TMP_FontAsset.hpp"
#include "TMPro/TMP_Text.hpp"
#include "UnityEngine/Font.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Resources.hpp"
#include "UnityEngine/TextCore/LowLevel/GlyphRenderMode.hpp"
#include "UnityEngine/Texture2D.hpp"

#include <dirent.h>
#include <sys/stat.h>

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace JapaneseLocalizer {
    namespace {
        constexpr float Interval = 2.0f;
        constexpr char const* SystemFont = "/system/fonts/NotoSansCJK-Regular.ttc";

        UnityW<TMPro::TMP_FontAsset> customFont;
        bool failed = false;
        std::set<int> processed;   // フォールバックを書き換えたフォントの InstanceID

        bool FileExists(std::string const& path) {
            struct stat st{};
            return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
        }

        std::string ChooseFontFile() {
            auto configured = getModConfig().FontFile.GetValue();
            if (!configured.empty()) {
                if (FileExists(configured)) return configured;
                WARNING("font file not found: {}", configured);
            }
            auto dir = DictionaryDir() + "fonts/";
            mkdir(dir.c_str(), 0777);
            std::vector<std::string> files;
            if (auto* d = opendir(dir.c_str())) {
                while (auto* ent = readdir(d)) {
                    std::string name = ent->d_name;
                    std::string lower = name;
                    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
                    if (lower.ends_with(".ttf") || lower.ends_with(".otf") || lower.ends_with(".ttc")) files.push_back(dir + name);
                }
                closedir(d);
            }
            if (!files.empty()) {
                std::sort(files.begin(), files.end());
                return files.front();
            }
            return FileExists(SystemFont) ? SystemFont : "";
        }

        std::string FontName(TMPro::TMP_FontAsset* font) {
            if (!font) return "null";
            StringW name = font->get_name();
            return name ? static_cast<std::string>(name) : "(no name)";
        }

        // ゲームのフォントのマテリアルを写して、作ったフォントのアトラスに付け替える（シェーダーを本体と同じにするため）
        void ApplyMaterial(TMPro::TMP_FontAsset* font, UnityEngine::Material* templateMaterial) {
            if (!templateMaterial) return;
            auto material = UnityEngine::Material::New_ctor(templateMaterial);
            auto atlas = font->get_atlasTexture();
            material->SetTexture(TMPro::ShaderUtilities::getStaticF_ID_MainTex(), atlas.ptr());
            if (atlas) {
                material->SetFloat(TMPro::ShaderUtilities::getStaticF_ID_TextureWidth(), (float) atlas->get_width());
                material->SetFloat(TMPro::ShaderUtilities::getStaticF_ID_TextureHeight(), (float) atlas->get_height());
            }
            material->SetFloat(TMPro::ShaderUtilities::getStaticF_ID_GradientScale(), (float) font->get_atlasPadding() + 1);
            font->material = material;
        }

        bool CreateCustomFont(UnityEngine::Material* templateMaterial) {
            auto path = ChooseFontFile();
            if (path.empty()) {
                WARNING("no font file to use");
                return false;
            }
            auto unityFont = UnityEngine::Font::New_ctor(StringW(path));
            if (!unityFont) {
                ERROR("could not load font file {}", path);
                return false;
            }
            auto font = TMPro::TMP_FontAsset::CreateFontAsset(unityFont, 64, 6, UnityEngine::TextCore::LowLevel::GlyphRenderMode::SDFAA, 2048, 2048,
                                                              TMPro::AtlasPopulationMode::Dynamic, true);
            if (!font) {
                ERROR("could not create a TMP font from {}", path);
                return false;
            }
            font->set_name(StringW("JapaneseLocalizer Font"));
            UnityEngine::Object::DontDestroyOnLoad(font);
            UnityEngine::Object::DontDestroyOnLoad(unityFont);
            ApplyMaterial(font, templateMaterial);
            customFont = font;
            INFO("created TMP font from {}", path);
            return true;
        }

        void RefreshTexts() {
            auto texts = UnityEngine::Object::FindObjectsOfType(csTypeOf(TMPro::TMP_Text*), false);
            for (auto obj : texts) {
                if (!obj) continue;
                auto text = reinterpret_cast<TMPro::TMP_Text*>(obj.ptr());
                text->SetAllDirty();
            }
        }

        void Tick() {
            if (!failed && getModConfig().ReplaceFont.GetValue()) {
                try {
                    auto fonts = UnityEngine::Resources::FindObjectsOfTypeAll(csTypeOf(TMPro::TMP_FontAsset*));
                    if (!customFont) {
                        UnityEngine::Material* templateMaterial = nullptr;
                        for (auto obj : fonts) {
                            auto f = reinterpret_cast<TMPro::TMP_FontAsset*>(obj.ptr());
                            if (f && f->material) {
                                templateMaterial = f->material.ptr();
                                if (FontName(f).starts_with("Teko-Medium")) break;
                            }
                        }
                        if (!CreateCustomFont(templateMaterial)) failed = true;
                    }
                    bool changed = false;
                    if (customFont) {
                        for (auto obj : fonts) {
                            auto f = reinterpret_cast<TMPro::TMP_FontAsset*>(obj.ptr());
                            if (!f || f == customFont.ptr()) continue;
                            if (!processed.insert(f->GetInstanceID()).second) continue;
                            auto table = f->get_fallbackFontAssetTable();
                            std::string chain;
                            if (table) {
                                for (int i = 0; i < table->get_Count(); i++) chain += FontName(table->get_Item(i).ptr()) + ", ";
                            } else {
                                table = System::Collections::Generic::List_1<UnityW<TMPro::TMP_FontAsset>>::New_ctor();
                                f->set_fallbackFontAssetTable(table);
                            }
                            INFO("font {} fallbacks: [{}]", FontName(f), chain);
                            table->Insert(0, customFont);
                            changed = true;
                        }
                    }
                    if (changed) RefreshTexts();
                } catch (std::exception const& e) {
                    ERROR("font replacement failed: {}", e.what());
                    failed = true;
                }
            }
            BSML::MainThreadScheduler::ScheduleAfterTime(Interval, Tick);
        }
    }

    void StartFontReplacer() {
        BSML::MainThreadScheduler::ScheduleAfterTime(Interval, Tick);
    }
}
