#include "Translator.hpp"
#include "Config.hpp"
#include "logging.hpp"
#include "JapaneseLocalizer/Dictionaries.hpp"

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
#include <set>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace JapaneseLocalizer {
    namespace {
        std::shared_mutex dictMutex;
        Core::Dictionaries dictionaries;   // 辞書の照合（quest/common）。mods.json の対応表もここに持つ

        std::mutex sourceMutex;
        std::unordered_map<std::string, TranslationSource> sources;   // 訳文 → 原文と Mod の ID

        std::mutex collectMutex;
        std::map<std::string, std::set<std::string>> untranslated;   // Mod の ID → 未翻訳の文字列
        bool untranslatedLoaded = false;

        // 辞書ではないファイル（書き出したログと対応表）
        bool IsDictionaryFile(std::string const& name) {
            if (!name.ends_with(".json")) return false;
            if (name == "mods.json") return false;
            if (name.starts_with("untranslated") || name.starts_with("overflow")) return false;
            return true;
        }

        std::optional<std::string> ReadFile(std::string const& path) {
            std::ifstream f(path, std::ios::binary);
            if (!f) return std::nullopt;
            std::stringstream ss;
            ss << f.rdbuf();
            return ss.str();
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

        Core::Dictionaries loaded;
        // 正式な Mod の ID と、各プラットフォームでの別名の対応表。無ければ .so 名をそのまま辞書名として使う
        if (auto table = ReadFile(DictionaryDir() + "mods.json")) {
            std::string error;
            if (!loaded.LoadModTable(*table, &error)) ERROR("mods.json: {}", error);
        } else {
            WARNING("mods.json not found; dictionaries are looked up by .so name");
        }

        std::vector<std::string> files;
        if (auto* dir = opendir(DictionaryDir().c_str())) {
            while (auto* ent = readdir(dir)) {
                std::string name = ent->d_name;
                if (IsDictionaryFile(name)) files.push_back(name);
            }
            closedir(dir);
        }
        // 以前（.so 名のファイル名だったとき）と同じ順に読む。全辞書をまとめた辞書では先に読んだ訳が勝つため
        auto orderKey = [&](std::string const& name) { return loaded.LoadOrderKey(name.substr(0, name.size() - 5)) + ".json"; };
        std::sort(files.begin(), files.end(), [&](auto const& a, auto const& b) { return orderKey(a) < orderKey(b); });

        for (auto const& name : files) {
            auto stem = name.substr(0, name.size() - 5);
            auto id = loaded.DictionaryIdForFile(stem);
            if (!id) {
                INFO("skip dictionary {}: old file name of {}", name, loaded.ResolveModule(stem));
                continue;
            }
            auto content = ReadFile(DictionaryDir() + name);
            if (!content) continue;
            std::string error;
            if (auto count = loaded.AddDictionary(*id, *content, &error))
                INFO("loaded dictionary {} ({}): {} entries", *id, name, *count);
            else
                ERROR("dictionary {}: {}", name, error);
        }

        std::unique_lock lock(dictMutex);
        dictionaries = std::move(loaded);
        INFO("loaded {} entries from {} dictionaries", dictionaries.EntryCount(), dictionaries.DictionaryCount());
    }

    std::string CallerModule() {
        BacktraceState state;
        _Unwind_Backtrace(UnwindCallback, &state);
        return state.result.empty() ? state.firstBridge : state.result;
    }

    std::optional<std::string> Translate(std::string_view module, std::string_view text) {
        if (text.empty() || !IsEnabled()) return std::nullopt;

        std::optional<std::string> translated;
        std::string id;
        {
            std::shared_lock lock(dictMutex);
            id = module.empty() ? std::string() : dictionaries.ResolveModule(module);
            translated = dictionaries.Translate(module, text);
        }

        if (!translated) {
            auto begin = text.find_first_not_of(" \t\r\n");
            if (begin != std::string_view::npos) {
                auto end = text.find_last_not_of(" \t\r\n") + 1;
                Collect(id, std::string(text.substr(begin, end - begin)));
            }
            return std::nullopt;
        }
        if (IsCollecting()) {
            std::lock_guard lock(sourceMutex);
            sources.try_emplace(*translated, TranslationSource{id, std::string(text)});
        }
        return translated;
    }

    std::optional<TranslationSource> FindTranslationSource(std::string const& translated) {
        std::lock_guard lock(sourceMutex);
        if (auto it = sources.find(translated); it != sources.end()) return it->second;
        return std::nullopt;
    }
}
