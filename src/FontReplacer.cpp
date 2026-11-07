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
#include "TMPro/TMP_Character.hpp"
#include "UnityEngine/TextCore/Glyph.hpp"
#include "UnityEngine/TextCore/GlyphMetrics.hpp"
#include "UnityEngine/TextCore/FaceInfo.hpp"
#include "System/Collections/Generic/Dictionary_2.hpp"
#include "TMPro/TMP_Text.hpp"
#include "UnityEngine/Font.hpp"
#include "UnityEngine/Material.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Shader.hpp"
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
            INFO("loaded font file {}", path);

            // CreateFontAsset は "TextMeshPro/Mobile/Distance Field" を Shader.Find で探してマテリアルを作る。
            // ゲームのビルドにはこのシェーダーが無いので、作る間だけ本体のフォントのシェーダーを渡しておく
            auto previousShader = TMPro::ShaderUtilities::getStaticF_k_ShaderRef_MobileSDF();
            bool overrideShader = !previousShader && templateMaterial && templateMaterial->get_shader();
            if (overrideShader) TMPro::ShaderUtilities::setStaticF_k_ShaderRef_MobileSDF(templateMaterial->get_shader());
            UnityW<TMPro::TMP_FontAsset> font;
            try {
                font = TMPro::TMP_FontAsset::CreateFontAsset(unityFont, 64, 6, UnityEngine::TextCore::LowLevel::GlyphRenderMode::SDFAA, 2048, 2048,
                                                             TMPro::AtlasPopulationMode::Dynamic, true);
            } catch (...) {
                if (overrideShader) TMPro::ShaderUtilities::setStaticF_k_ShaderRef_MobileSDF(previousShader);
                throw;
            }
            if (overrideShader) TMPro::ShaderUtilities::setStaticF_k_ShaderRef_MobileSDF(previousShader);
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

        std::string Describe(UnityEngine::TextCore::FaceInfo fi) {
            return fmt::format("pointSize {} scale {:.3f} lineHeight {:.2f} ascent {:.2f} descent {:.2f}", fi.get_pointSize(), fi.get_scale(),
                               fi.get_lineHeight(), fi.get_ascentLine(), fi.get_descentLine());
        }

        // 「あ」の1字あたりの送り幅を、pointSize あたりの値（字の拡大率も掛けたもの）で返す
        std::optional<float> AdvancePerPoint(TMPro::TMP_FontAsset* font, uint32_t unicode) {
            TMPro::TMP_Character* character = nullptr;
            auto table = font->get_characterLookupTable();
            if (!table || !table->TryGetValue(unicode, byref(character)) || !character) return std::nullopt;
            auto glyph = character->get_glyph();
            if (!glyph) return std::nullopt;
            auto fi = font->get_faceInfo();
            float advance = glyph->get_metrics().get_horizontalAdvance() * glyph->get_scale() * character->get_scale();
            INFO("  {} U+{:04X}: advance {:.2f} glyphScale {:.3f} charScale {:.3f} pointSize {} faceScale {:.3f}", FontName(font), unicode,
                 glyph->get_metrics().get_horizontalAdvance(), glyph->get_scale(), character->get_scale(), fi.get_pointSize(), fi.get_scale());
            return advance / (float) fi.get_pointSize() * fi.get_scale();
        }

        // 本体の日本語フォント（NotoSansJP-Medium SDF）と字の大きさをそろえる。
        // 1行の高さの比率は同じでも、本体は字ごとの拡大率で小さくしてあるので、「あ」の送り幅で比べて faceInfo.scale を合わせる
        void MatchScale(TMPro::TMP_FontAsset* font, TMPro::TMP_FontAsset* reference) {
            if (!font || !reference) return;
            constexpr uint32_t Probe = 0x3042;   // あ
            font->TryAddCharacters(StringW(u"あ"), false);
            auto ref = AdvancePerPoint(reference, Probe);
            auto own = AdvancePerPoint(font, Probe);
            if (!ref || !own || *ref <= 0 || *own <= 0) {
                WARNING("could not compare glyph sizes with {}", FontName(reference));
                return;
            }
            auto fi = font->get_faceInfo();
            fi.set_scale(fi.get_scale() * *ref / *own);
            font->set_faceInfo(fi);
            INFO("custom font scale set to {:.3f} to match {}", fi.get_scale(), FontName(reference));
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
                        for (auto obj : fonts) {
                            auto f = reinterpret_cast<TMPro::TMP_FontAsset*>(obj.ptr());
                            if (f && FontName(f).starts_with("NotoSansJP")) {
                                MatchScale(customFont.ptr(), f);
                                break;
                            }
                        }
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
