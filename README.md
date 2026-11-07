# JapaneseLocalizer

Quest 版 Beat Saber（1.40.8_7379）で、ほかの Mod の UI を日本語にする Mod です。本体は公式に日本語対応しているので、訳す対象は Mod の UI だけです。

## しくみ

文字を描画する段階（TMP）ではなく、UI の部品を作る関数に渡される文字列を、Mod ごとの JSON 辞書で訳す。

| 対象 | フックする関数 | 訳すもの |
|---|---|---|
| .bsml | `BSML::BSMLParser::parse` | `text`、`hover-hint`、`tab-name` などの属性。`~` で始まる値と、`choices`・`options` は訳さない |
| BSML-Lite | `CreateText`、`CreateUIButton`、`CreateToggle`、`CreateSliderSetting`、`CreateIncrementSetting`、`CreateStringSetting`、`CreateDropdown`（ラベルだけ）、`AddHoverHint`、`SetButtonText` | 表示する文字列 |
| ChatPlex SDK（BeatSaberPlus） | `CText::SetText`、`CPOrSButton::SetText` / `SetTooltip`、`CTextInput::SetPlaceHolder`、`TopNavigationView::SetTitle`、`CTextSegmentedControl::SetTexts` | 表示する文字列。`CDropdown::SetOptions` は値も変わるので訳さない |

どの Mod の文字列かは、スタックをたどって .so 名から決める（`libsongdownloader.so` → `songdownloader`）。

## 辞書

- 置き場所: `/sdcard/ModData/com.beatgames.beatsaber/Mods/JapaneseLocalizer/<Mod名>.json`
- 形式: `{"原文": "訳"}`。`{0}` を含む見出し語はテンプレートとして照合する
- 引く順: その Mod の辞書 → `_common.json` → 全辞書
- 元データは `tools/dicts_*.py`。`python3 tools/build_dicts.py` で `dictionaries/` と `mod.template.json` の `fileCopies` を作り直す

## 設定（Mod 設定の「日本語化」）

- **日本語に置き換える**: 翻訳のオン / オフ
- **未翻訳収集モード**: オンにすると、次の2つを辞書と同じフォルダーに書き出す
  - `untranslated.json`: 辞書に無い文字列（Mod 名つき）
  - `overflow.json`: 1行に収まらない訳（原文より行が増えたもの、枠からはみ出したもの）
- **日本語フォントをそろえる**: フォントファイルから実行時に TMP フォントを作り、全フォントのフォールバックの先頭に入れる。本体の日本語フォント（NotoSansJP-Medium SDF）は字を絞ってあり、無い漢字が中国語版の字形（SourceHanSansCN）で表示されるのを防ぐ。使うフォントは、設定の `fontFile` → `fonts/` フォルダーの最初のファイル → 端末の `/system/fonts/NotoSansCJK-Regular.ttc` の順に選ぶ

## ビルド

```bash
qpm restore
qpm s build -- -release
rm -f mod.json && qpm s qmod
```

paper2_scotland2 は実機に合わせて `~4.7.0` に固定している。
