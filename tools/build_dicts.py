"""tools/dicts_*.py の辞書を dictionaries/<Mod名>.json に書き出す。"""
import json, pathlib, importlib.util, sys

root = pathlib.Path(__file__).resolve().parent.parent
out = root / "dictionaries"
out.mkdir(exist_ok=True)
merged = {}
for src in sorted((root / "tools").glob("dicts_*.py")):
    spec = importlib.util.spec_from_file_location(src.stem, src)
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    for name, entries in mod.D.items():
        merged.setdefault(name, {}).update(entries)
for name, entries in merged.items():
    path = out / f"{name}.json"
    path.write_text(json.dumps(dict(sorted(entries.items())), ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"{path.name}: {len(entries)}")

# qmod のインストール時に辞書を配置するよう、mod.template.json の fileCopies を更新する
template_path = root / "mod.template.json"
template = json.loads(template_path.read_text(encoding="utf-8"))
template["fileCopies"] = [
    {"name": f"{name}.json", "destination": f"/sdcard/ModData/com.beatgames.beatsaber/Mods/JapaneseLocalizer/{name}.json"}
    for name in sorted(merged)
]
template_path.write_text(json.dumps(template, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
