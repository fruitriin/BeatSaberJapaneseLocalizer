// quest/common の辞書の照合を、Mac などの clang で試験する。使い方は tests/run.sh を見る。
#include "JapaneseLocalizer/Dictionaries.hpp"
#include "rapidjson/document.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

namespace fs = std::filesystem;
using JapaneseLocalizer::Core::Dictionaries;

static std::string ReadFile(fs::path const& path) {
    std::ifstream f(path, std::ios::binary);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

int main(int argc, char** argv) {
    fs::path root = argc > 1 ? fs::path(argv[1]) : fs::current_path();
    Dictionaries dicts;
    std::string error;
    if (!dicts.LoadModTable(ReadFile(root / "mods.json"), &error)) {
        std::cerr << "mods.json: " << error << "\n";
        return 2;
    }

    // 実機と同じく、Quest の .so 名の順に読む
    std::vector<fs::path> files;
    for (auto const& e : fs::directory_iterator(root / "dictionaries"))
        if (e.path().extension() == ".json") files.push_back(e.path());
    auto key = [&](fs::path const& p) { return dicts.LoadOrderKey(p.stem().string()) + ".json"; };
    std::sort(files.begin(), files.end(), [&](auto const& a, auto const& b) { return key(a) < key(b); });
    for (auto const& p : files) {
        auto id = dicts.DictionaryIdForFile(p.stem().string());
        if (!id) continue;
        if (!dicts.AddDictionary(*id, ReadFile(p), &error)) {
            std::cerr << p << ": " << error << "\n";
            return 2;
        }
    }

    rapidjson::Document doc;
    auto casesJson = ReadFile(root / "tests" / "cases.json");
    doc.Parse(casesJson.c_str());
    if (doc.HasParseError()) {
        std::cerr << "tests/cases.json is not valid JSON\n";
        return 2;
    }

    int failed = 0, total = 0;
    auto report = [&](std::string const& name, std::optional<std::string> const& actual, rapidjson::Value const& expected) {
        total++;
        bool ok = expected.IsNull() ? !actual : (actual && *actual == expected.GetString());
        if (!ok) {
            failed++;
            std::cout << "NG " << name << "\n   expected: " << (expected.IsNull() ? "(none)" : expected.GetString())
                      << "\n   actual:   " << (actual ? *actual : "(none)") << "\n";
        } else {
            std::cout << "OK " << name << "\n";
        }
    };
    for (auto const& c : doc["cases"].GetArray())
        report(c["name"].GetString(), dicts.Translate(c["module"].GetString(), c["text"].GetString()), c["expected"]);
    for (auto const& c : doc["files"].GetArray())
        report(std::string("file: ") + c["name"].GetString(), dicts.DictionaryIdForFile(c["stem"].GetString()), c["expected"]);

    std::cout << "\n" << (total - failed) << "/" << total << " passed (" << dicts.DictionaryCount() << " dictionaries, "
              << dicts.EntryCount() << " entries)\n";
    return failed == 0 ? 0 : 1;
}
