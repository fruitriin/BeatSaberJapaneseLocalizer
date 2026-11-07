// 1行に収まらない訳を見つけて overflow.json に書き出す。
// 未翻訳収集モードがオンのあいだだけ、数秒おきに画面上の TMP テキストを調べる。
#include "Translator.hpp"
#include "logging.hpp"

#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/prettywriter.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/stringbuffer.h"
#include "beatsaber-hook/shared/utils/il2cpp-utils.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "TMPro/TMP_Text.hpp"
#include "UnityEngine/Object.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/RectTransform.hpp"

#include <cmath>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

namespace JapaneseLocalizer {
    namespace {
        constexpr float CheckInterval = 3.0f;

        struct Finding {
            std::string translation;
            float width;
            int lines;
            int originalLines;
            bool wordWrapping;
            float translatedWidth;
            float originalWidth;
        };

        std::map<std::string, std::map<std::string, Finding>> findings;   // Mod 名 → 原文 → 結果
        std::set<std::string> checked;                                     // 測り終えた「訳文 + 枠の幅」
        bool loaded = false;

        std::string FilePath() { return DictionaryDir() + "overflow.json"; }

        void Load() {
            if (loaded) return;
            loaded = true;
            std::ifstream f(FilePath(), std::ios::binary);
            if (!f) return;
            std::stringstream ss;
            ss << f.rdbuf();
            rapidjson::Document doc;
            doc.Parse(ss.str().c_str());
            if (doc.HasParseError() || !doc.IsObject()) return;
            for (auto& mod : doc.GetObject()) {
                if (!mod.value.IsObject()) continue;
                for (auto& entry : mod.value.GetObject()) {
                    auto& v = entry.value;
                    if (!v.IsObject() || !v.HasMember("translation")) continue;
                    Finding finding{
                        v["translation"].GetString(),
                        v.HasMember("width") ? v["width"].GetFloat() : 0,
                        v.HasMember("lines") ? v["lines"].GetInt() : 0,
                        v.HasMember("originalLines") ? v["originalLines"].GetInt() : 0,
                        v.HasMember("wordWrapping") && v["wordWrapping"].GetBool(),
                        v.HasMember("translatedWidth") ? v["translatedWidth"].GetFloat() : 0,
                        v.HasMember("originalWidth") ? v["originalWidth"].GetFloat() : 0,
                    };
                    findings[mod.name.GetString()].emplace(entry.name.GetString(), finding);
                }
            }
        }

        void Save() {
            rapidjson::Document doc;
            doc.SetObject();
            auto& a = doc.GetAllocator();
            for (auto const& [mod, entries] : findings) {
                rapidjson::Value obj(rapidjson::kObjectType);
                for (auto const& [original, f] : entries) {
                    rapidjson::Value v(rapidjson::kObjectType);
                    v.AddMember("translation", rapidjson::Value(f.translation.c_str(), a), a);
                    v.AddMember("width", std::round(f.width * 10) / 10, a);
                    v.AddMember("wordWrapping", f.wordWrapping, a);
                    if (f.wordWrapping) {
                        v.AddMember("lines", f.lines, a);
                        v.AddMember("originalLines", f.originalLines, a);
                    } else {
                        v.AddMember("translatedWidth", std::round(f.translatedWidth * 10) / 10, a);
                        v.AddMember("originalWidth", std::round(f.originalWidth * 10) / 10, a);
                    }
                    obj.AddMember(rapidjson::Value(original.c_str(), a), v, a);
                }
                doc.AddMember(rapidjson::Value(mod.c_str(), a), obj, a);
            }
            rapidjson::StringBuffer buf;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
            writer.SetIndent(' ', 2);
            doc.Accept(writer);
            std::ofstream f(FilePath(), std::ios::binary | std::ios::trunc);
            f << buf.GetString();
        }

        // 枠の幅で折り返したときの行数を、1行の高さから見積もる
        int EstimateLines(TMPro::TMP_Text* text, StringW content, float width, float lineHeight) {
            auto height = text->GetPreferredValues(content, width, 100000.0f).y;
            if (lineHeight <= 0) return 1;
            return std::max(1, (int) std::lround(height / lineHeight));
        }

        bool CheckText(TMPro::TMP_Text* text) {
            if (!text || !text->get_isActiveAndEnabled()) return false;
            StringW content = text->get_text();
            if (!content) return false;
            std::string translated = content;
            auto source = FindTranslationSource(translated);
            if (!source) return false;

            float width = text->get_rectTransform()->get_rect().width;
            if (width <= 1.0f) return false;   // まだ配置が済んでいない

            auto key = translated + "\x1f" + std::to_string((int) std::lround(width));
            if (!checked.insert(key).second) return false;

            Finding f{translated, width, 1, 1, text->get_enableWordWrapping(), 0, 0};
            bool problem;
            if (f.wordWrapping) {
                float lineHeight = text->GetPreferredValues(StringW("あ"), width, 100000.0f).y;
                f.lines = EstimateLines(text, content, width, lineHeight);
                f.originalLines = EstimateLines(text, StringW(source->original), width, lineHeight);
                problem = f.lines > f.originalLines && f.originalLines == 1;
            } else {
                f.translatedWidth = text->GetPreferredValues(content).x;
                f.originalWidth = text->GetPreferredValues(StringW(source->original)).x;
                // 原文も収まっていないなら、配置が決まる前に測ったものとみなす
                problem = f.translatedWidth > width + 0.5f && f.originalWidth <= width + 0.5f;
            }
            if (!problem) return false;

            auto module = source->module.empty() ? std::string("_unknown") : source->module;
            findings[module].insert_or_assign(source->original, f);
            INFO("overflow: [{}] '{}' -> '{}'", module, source->original, translated);
            return true;
        }

        void Tick() {
            if (IsEnabled() && IsCollecting()) {
                Load();
                bool changed = false;
                auto texts = UnityEngine::Object::FindObjectsOfType(csTypeOf(TMPro::TMP_Text*), false);
                for (auto obj : texts) {
                    if (!obj) continue;
                    try {
                        changed |= CheckText(reinterpret_cast<TMPro::TMP_Text*>(obj.ptr()));
                    } catch (std::exception const& e) {
                        ERROR("layout check failed: {}", e.what());
                    }
                }
                if (changed) Save();
            }
            BSML::MainThreadScheduler::ScheduleAfterTime(CheckInterval, Tick);
        }
    }

    void StartLayoutCheck() {
        BSML::MainThreadScheduler::ScheduleAfterTime(CheckInterval, Tick);
    }
}
