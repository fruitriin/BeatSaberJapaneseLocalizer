#include "Translator.hpp"
#include "Config.hpp"
#include "logging.hpp"

#include "beatsaber-hook/shared/rapidjson/include/rapidjson/document.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/prettywriter.h"
#include "beatsaber-hook/shared/rapidjson/include/rapidjson/stringbuffer.h"

#include <dirent.h>
#include <dlfcn.h>
#include <sys/stat.h>
#include <unwind.h>

#include <fstream>
#include <map>
#include <mutex>
#include <regex>
#include <set>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace JapaneseLocalizer {
    namespace {
        // {0} などを含む見出し語を正規表現にしたもの
        struct Template {
            std::regex pattern;
            std::vector<int> slots;      // 見出し語の中の {n} の並び（キャプチャの順）
            std::string translation;     // {n} を含む訳
        };

        struct Dictionary {
            std::unordered_map<std::string, std::string> exact;
            std::vector<Template> templates;
        };

        std::shared_mutex dictMutex;
        std::unordered_map<std::string, Dictionary> dictionaries;   // キーはファイル名（拡張子なし）
        Dictionary merged;                                            // 全辞書をまとめたもの（Mod 名が決まらないとき用）

        std::mutex sourceMutex;
        std::unordered_map<std::string, TranslationSource> sources;   // 訳文 → 原文と Mod 名

        std::mutex collectMutex;
        std::map<std::string, std::set<std::string>> untranslated;   // Mod 名 → 未翻訳の文字列
        bool untranslatedLoaded = false;

        constexpr std::string_view CommonDictName = "_common";

        std::string EscapeRegex(std::string_view s) {
            static constexpr std::string_view special = R"(\^$.|?*+()[]{})";
            std::string out;
            out.reserve(s.size() * 2);
            for (char c : s) {
                if (special.find(c) != std::string_view::npos) out.push_back('\\');
                out.push_back(c);
            }
            return out;
        }

        // "{0} songs" → regex "^(.*?) songs$"、slots = {0}
        std::optional<Template> MakeTemplate(std::string const& key, std::string const& value) {
            static std::regex const placeholder(R"(\{(\d+)\})");
            Template t;
            std::string pattern;
            std::size_t last = 0;
            for (auto it = std::sregex_iterator(key.begin(), key.end(), placeholder); it != std::sregex_iterator(); ++it) {
                auto const& m = *it;
                pattern += EscapeRegex(std::string_view(key).substr(last, m.position() - last));
                pattern += "([\\s\\S]*?)";
                t.slots.push_back(std::stoi(m[1].str()));
                last = m.position() + m.length();
            }
            if (t.slots.empty()) return std::nullopt;
            pattern += EscapeRegex(std::string_view(key).substr(last));
            try {
                t.pattern = std::regex(pattern, std::regex::ECMAScript | std::regex::optimize);
            } catch (std::exception const& e) {
                ERROR("bad template '{}': {}", key, e.what());
                return std::nullopt;
            }
            t.translation = value;
            return t;
        }

        void AddEntry(Dictionary& dict, std::string const& key, std::string const& value) {
            if (key.find('{') != std::string::npos) {
                if (auto t = MakeTemplate(key, value)) {
                    dict.templates.push_back(std::move(*t));
                    return;
                }
            }
            dict.exact.emplace(key, value);
        }

        std::optional<std::string> ReadFile(std::string const& path) {
            std::ifstream f(path, std::ios::binary);
            if (!f) return std::nullopt;
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
        }

        std::optional<std::string> LookupIn(Dictionary const& dict, std::string const& text) {
            if (auto it = dict.exact.find(text); it != dict.exact.end()) return it->second;
            for (auto const& t : dict.templates) {
                std::smatch m;
                if (!std::regex_match(text, m, t.pattern)) continue;
                std::string out = t.translation;
                for (std::size_t i = 0; i < t.slots.size(); i++) {
                    std::string ph = "{" + std::to_string(t.slots[i]) + "}";
                    std::size_t pos;
                    while ((pos = out.find(ph)) != std::string::npos) out.replace(pos, ph.size(), m[i + 1].str());
                }
                return out;
            }
            return std::nullopt;
        }

        // 収集に値しない文字列（数値だけ、日本語を含む、など）
        bool WorthCollecting(std::string_view text) {
            bool hasLetter = false;
            for (unsigned char c : text) {
                if (c >= 0x80) return false;   // 非 ASCII は訳済みか固有名とみなす
                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')) hasLetter = true;
            }
            return hasLetter;
        }

        void SaveUntranslatedLocked() {
            rapidjson::Document doc;
            doc.SetObject();
            auto& alloc = doc.GetAllocator();
            for (auto const& [mod, strings] : untranslated) {
                rapidjson::Value obj(rapidjson::kObjectType);
                for (auto const& s : strings) {
                    obj.AddMember(rapidjson::Value(s.c_str(), alloc), rapidjson::Value("", alloc), alloc);
                }
                doc.AddMember(rapidjson::Value(mod.c_str(), alloc), obj, alloc);
            }
            rapidjson::StringBuffer buf;
            rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(buf);
            writer.SetIndent(' ', 2);
            doc.Accept(writer);
            std::ofstream f(DictionaryDir() + "untranslated.json", std::ios::binary | std::ios::trunc);
            f << buf.GetString();
        }

        void LoadUntranslatedLocked() {
            if (untranslatedLoaded) return;
            untranslatedLoaded = true;
            auto content = ReadFile(DictionaryDir() + "untranslated.json");
            if (!content) return;
            rapidjson::Document doc;
            doc.Parse(content->c_str());
            if (doc.HasParseError() || !doc.IsObject()) return;
            for (auto& m : doc.GetObject()) {
                if (!m.value.IsObject()) continue;
                auto& set = untranslated[m.name.GetString()];
                for (auto& s : m.value.GetObject()) set.emplace(s.name.GetString());
            }
        }

        void Collect(std::string_view module, std::string const& text) {
            if (!IsCollecting() || !WorthCollecting(text)) return;
            std::lock_guard lock(collectMutex);
            LoadUntranslatedLocked();
            auto key = module.empty() ? std::string("_unknown") : std::string(module);
            if (untranslated[key].emplace(text).second) SaveUntranslatedLocked();
        }

        // スタックをたどるときに飛ばす .so（UI の仕組みそのものや、ゲーム本体）
        bool IsInfrastructure(std::string_view name) {
            static constexpr std::string_view skip[] = {
                "libJapaneseLocalizer.so", "libbsml.so", "libchatplex-sdk-bs.so", "libbeatsaber-hook.so",
                "libil2cpp.so", "libunity.so", "libcustom-types.so", "libc.so", "libsl2.so", "libscotland2.so",
                "libmain.so", "libtinyxml2.so", "libpaper2_scotland2.so", "libc++_shared.so",
            };
            for (auto s : skip) if (name == s) return true;
            return false;
        }

        std::string ModuleNameFromPath(std::string_view path) {
            auto slash = path.rfind('/');
            auto file = slash == std::string_view::npos ? path : path.substr(slash + 1);
            if (file.starts_with("lib")) file.remove_prefix(3);
            if (file.ends_with(".so")) file.remove_suffix(3);
            return std::string(file);
        }

        struct BacktraceState {
            std::string result;
            std::string firstBridge;   // 見つからないときの代わり（libbsml / libchatplex-sdk-bs）
            int depth = 0;
        };

        _Unwind_Reason_Code UnwindCallback(_Unwind_Context* ctx, void* arg) {
            auto* state = static_cast<BacktraceState*>(arg);
            if (++state->depth > 48) return _URC_END_OF_STACK;
            auto pc = _Unwind_GetIP(ctx);
            if (!pc) return _URC_NO_REASON;
            Dl_info info{};
            if (!dladdr(reinterpret_cast<void*>(pc), &info) || !info.dli_fname) return _URC_NO_REASON;
            std::string_view path = info.dli_fname;
            auto slash = path.rfind('/');
            auto file = slash == std::string_view::npos ? path : path.substr(slash + 1);
            if (IsInfrastructure(file)) {
                if (state->firstBridge.empty() && (file == "libchatplex-sdk-bs.so" || file == "libbsml.so"))
                    state->firstBridge = ModuleNameFromPath(file);
                return _URC_NO_REASON;
            }
            // ゲーム本体や OS の .so は飛ばす
            if (path.starts_with("/data/app/") || path.starts_with("/system/") || path.starts_with("/apex/") || path.starts_with("/vendor/"))
                return _URC_NO_REASON;
            state->result = ModuleNameFromPath(file);
            return _URC_END_OF_STACK;
        }
    }

    std::string const& DictionaryDir() {
        static std::string const dir = "/sdcard/ModData/com.beatgames.beatsaber/Mods/JapaneseLocalizer/";
        return dir;
    }

    bool IsEnabled() { return getModConfig().Enabled.GetValue(); }
    bool IsCollecting() { return getModConfig().CollectUntranslated.GetValue(); }

    void LoadDictionaries() {
        mkdir("/sdcard/ModData/com.beatgames.beatsaber/Mods/", 0777);
        mkdir(DictionaryDir().c_str(), 0777);

        std::unordered_map<std::string, Dictionary> loaded;
        Dictionary all;
        std::size_t total = 0;

        if (auto* dir = opendir(DictionaryDir().c_str())) {
            std::vector<std::string> files;
            while (auto* ent = readdir(dir)) {
                std::string name = ent->d_name;
                if (!name.ends_with(".json") || name == "untranslated.json") continue;
                files.push_back(name);
            }
            closedir(dir);
            std::sort(files.begin(), files.end());

            for (auto const& name : files) {
                auto content = ReadFile(DictionaryDir() + name);
                if (!content) continue;
                rapidjson::Document doc;
                doc.Parse(content->c_str());
                if (doc.HasParseError() || !doc.IsObject()) {
                    ERROR("dictionary {} is not a JSON object (parse error {} at {})", name, (int) doc.GetParseError(), doc.GetErrorOffset());
                    continue;
                }
                auto module = name.substr(0, name.size() - 5);
                auto& dict = loaded[module];
                for (auto& m : doc.GetObject()) {
                    if (!m.name.IsString() || !m.value.IsString()) continue;
                    std::string key = m.name.GetString();
                    std::string value = m.value.GetString();
                    if (key.empty() || value.empty() || key.starts_with("//")) continue;
                    AddEntry(dict, key, value);
                    AddEntry(all, key, value);
                    total++;
                }
                INFO("loaded dictionary {}: {} exact, {} templates", module, dict.exact.size(), dict.templates.size());
            }
        }

        std::unique_lock lock(dictMutex);
        dictionaries = std::move(loaded);
        merged = std::move(all);
        INFO("loaded {} entries from {} dictionaries", total, dictionaries.size());
    }

    std::string CallerModule() {
        BacktraceState state;
        _Unwind_Backtrace(UnwindCallback, &state);
        return state.result.empty() ? state.firstBridge : state.result;
    }

    std::optional<std::string> Translate(std::string_view module, std::string_view text) {
        if (text.empty() || !IsEnabled()) return std::nullopt;

        // 前後の空白は残したまま、中身だけを辞書で引く
        auto begin = text.find_first_not_of(" \t\r\n");
        if (begin == std::string_view::npos) return std::nullopt;
        auto end = text.find_last_not_of(" \t\r\n") + 1;
        std::string core(text.substr(begin, end - begin));

        std::optional<std::string> result;
        {
            std::shared_lock lock(dictMutex);
            if (!module.empty()) {
                if (auto it = dictionaries.find(std::string(module)); it != dictionaries.end())
                    result = LookupIn(it->second, core);
            }
            if (!result) {
                if (auto it = dictionaries.find(std::string(CommonDictName)); it != dictionaries.end())
                    result = LookupIn(it->second, core);
            }
            if (!result) result = LookupIn(merged, core);
        }

        if (!result) {
            Collect(module, core);
            return std::nullopt;
        }
        auto translated = std::string(text.substr(0, begin)) + *result + std::string(text.substr(end));
        if (IsCollecting()) {
            std::lock_guard lock(sourceMutex);
            sources.try_emplace(translated, TranslationSource{std::string(module), std::string(text)});
        }
        return translated;
    }

    std::optional<TranslationSource> FindTranslationSource(std::string const& translated) {
        std::lock_guard lock(sourceMutex);
        if (auto it = sources.find(translated); it != sources.end()) return it->second;
        return std::nullopt;
    }
}
