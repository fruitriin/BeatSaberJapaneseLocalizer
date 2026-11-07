"""tools/dicts_*.py の辞書を dictionaries/<正式な Mod の ID>.json に書き出す。

- 辞書の名前は mods.json の正式な ID（と共通辞書の _common）に限る
- 訳が空文字列の見出し語は、英語のまま残すという意味なので書き出さない
- quest/1.40.8/mod.template.json の fileCopies を、辞書と mods.json を配置するよう作り直す
"""
import importlib.util
import json
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parent.parent
out = root / "dictionaries"
out.mkdir(exist_ok=True)
mods = {k: v for k, v in json.loads((root / "mods.json").read_text(encoding="utf-8")).items() if not k.startswith("$")}

merged: dict[str, dict[str, str]] = {}
# ファイル名の順に読み込み、後のファイルが同じ見出し語を上書きする
for src in sorted((root / "tools").glob("dicts_*.py")):
    spec = importlib.util.spec_from_file_location(src.stem, src)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    for name, entries in mod.D.items():
        if name != "_common" and name not in mods:
            sys.exit(f"{src.name}: 辞書の名前 {name} は mods.json の正式な ID ではない")
        merged.setdefault(name, {}).update(entries)
        for key in [k for k, v in entries.items() if not v]:
            merged[name].pop(key, None)

# 古いファイル名の辞書を消してから書き出す
for old in out.glob("*.json"):
    old.unlink()
for name, entries in sorted(merged.items()):
    path = out / f"{name}.json"
    path.write_text(json.dumps(dict(sorted(entries.items())), ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"{path.name}: {len(entries)}")

dest = "/sdcard/ModData/com.beatgames.beatsaber/Mods/JapaneseLocalizer/"
template_path = root / "quest" / "1.40.8" / "mod.template.json"
template = json.loads(template_path.read_text(encoding="utf-8"))
template["fileCopies"] = [{"name": "mods.json", "destination": dest + "mods.json"}] + [
    {"name": f"{name}.json", "destination": dest + f"{name}.json"} for name in sorted(merged)
]
template_path.write_text(json.dumps(template, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
