#include "JapaneseLocalizer/Dictionaries.hpp"

#include "rapidjson/document.h"

#include <algorithm>
#include <cctype>

namespace JapaneseLocalizer::Core {
    namespace {
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
    }

    std::string Dictionaries::Lower(std::string_view s) {
        std::string out(s);
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char c) { return (char) std::tolower(c); });
        return out;
    }

    bool Dictionaries::LoadModTable(std::string_view json, std::string* error) {
        rapidjson::Document doc;
        doc.Parse(json.data(), json.size());
        if (doc.HasParseError() || !doc.IsObject()) {
            if (error) *error = "mods.json is not a JSON object";
            return false;
        }
        ids.clear();
        questAliases.clear();
        firstQuestAlias.clear();
        for (auto& m : doc.GetObject()) {
            std::string id = m.name.GetString();
            if (id.empty() || id[0] == '$' || !m.value.IsObject()) continue;
            ids[Lower(id)] = id;
            if (m.value.HasMember("quest") && m.value["quest"].IsArray()) {
                for (auto& alias : m.value["quest"].GetArray()) {
                    if (!alias.IsString()) continue;
                    questAliases[Lower(alias.GetString())] = id;
                    firstQuestAlias.try_emplace(Lower(id), alias.GetString());
                }
            }
        }
        return true;
    }

    std::optional<std::string> Dictionaries::DictionaryIdForFile(std::string_view stem) const {
        auto lower = Lower(stem);
        if (lower == CommonDictionary) return std::string(CommonDictionary);
        if (auto it = ids.find(lower); it != ids.end()) return it->second;
        if (questAliases.contains(lower)) return std::nullopt;
        return std::string(stem);
    }

    std::string Dictionaries::LoadOrderKey(std::string_view stem) const {
        if (auto it = firstQuestAlias.find(Lower(stem)); it != firstQuestAlias.end()) return it->second;
        return std::string(stem);
    }

    std::string Dictionaries::ResolveModule(std::string_view module) const {
        auto lower = Lower(module);
        if (auto it = questAliases.find(lower); it != questAliases.end()) return it->second;
        if (auto it = ids.find(lower); it != ids.end()) return it->second;
        return std::string(module);
    }

    std::optional<Dictionaries::Template> Dictionaries::MakeTemplate(std::string const& key, std::string const& value) {
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
        } catch (std::exception const&) {
            return std::nullopt;
        }
        t.translation = value;
        return t;
    }

    void Dictionaries::AddEntry(Dictionary& dict, std::string const& key, std::string const& value) {
        if (key.find('{') != std::string::npos) {
            if (auto t = MakeTemplate(key, value)) {
                dict.templates.push_back(std::move(*t));
                return;
            }
        }
        dict.exact.emplace(key, value);
    }

    std::optional<std::size_t> Dictionaries::AddDictionary(std::string const& id, std::string_view json, std::string* error) {
        rapidjson::Document doc;
        doc.Parse(json.data(), json.size());
        if (doc.HasParseError() || !doc.IsObject()) {
            if (error) *error = "not a JSON object (parse error " + std::to_string((int) doc.GetParseError()) + " at " + std::to_string(doc.GetErrorOffset()) + ")";
            return std::nullopt;
        }
        auto& dict = dictionaries[Lower(id)];
        std::size_t count = 0;
        for (auto& m : doc.GetObject()) {
            if (!m.name.IsString() || !m.value.IsString()) continue;
            std::string key = m.name.GetString();
            std::string value = m.value.GetString();
            if (key.empty() || value.empty() || key.starts_with("//") || key[0] == '$') continue;
            AddEntry(dict, key, value);
            AddEntry(merged, key, value);
            count++;
        }
        entryCount += count;
        return count;
    }

    void Dictionaries::ClearDictionaries() {
        dictionaries.clear();
        merged = {};
        entryCount = 0;
    }

    std::optional<std::string> Dictionaries::LookupIn(Dictionary const& dict, std::string const& text) {
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

    std::optional<std::string> Dictionaries::Translate(std::string_view module, std::string_view text) const {
        auto begin = text.find_first_not_of(" \t\r\n");
        if (begin == std::string_view::npos) return std::nullopt;
        auto end = text.find_last_not_of(" \t\r\n") + 1;
        std::string core(text.substr(begin, end - begin));

        std::optional<std::string> result;
        if (!module.empty()) {
            if (auto it = dictionaries.find(Lower(ResolveModule(module))); it != dictionaries.end()) result = LookupIn(it->second, core);
        }
        if (!result) {
            if (auto it = dictionaries.find(std::string(CommonDictionary)); it != dictionaries.end()) result = LookupIn(it->second, core);
        }
        if (!result) result = LookupIn(merged, core);
        if (!result) return std::nullopt;
        return std::string(text.substr(0, begin)) + *result + std::string(text.substr(end));
    }
}
