# 2026-11-07 14:4x の実機の untranslated.json / overflow.json を元に足した訳と、はみ出しを直した訳。
# ファイル名の順で最後に読み込まれ、それまでの辞書を上書きする。
D = {}

D["ClockMod"] = {
    "24/12 Toggle": "12 / 24 時間表示",
    "Show Battery Percentage": "電池の残量を表示",
    "Show During Replay": "リプレイ中も表示",
    "Show During Song": "プレイ中も表示",
    "Show Seconds": "秒を表示",
    "Your Timezone -  {0}\nUTC offset -  {1}": "タイムゾーン -  {0}\nUTC との時差 -  {1}",
}

D["EasyOffsetQuest"] = {
    "<color=#7777ff>Right</color>": "<color=#7777ff>右</color>",
    "<color=#ff6666>Left</color>": "<color=#ff6666>左</color>",
    "Button": "ボタン",
    "Reset L": "L をリセット",
    "Reset R": "R をリセット",
    "Z Offset": "Z オフセット",
    # 幅 8〜13 のボタン
    "Free hand": "フリー",
    "Load": "読込",
    "UI Lock": "ロック",
    "Save As": "",   # 全角3字に収まる訳が無いので英語のまま（空の訳は読み込み時に飛ばされる）
}

D["HeartBeatQuest"] = {
    "Age": "年齢",
    "Waiting...": "待機中...",
    "data source(need restart)": "データの取得元（再起動が必要）",
    "enabled(need restart)": "有効（再起動が必要）",
    "hiding mac address, click to show.": "MAC アドレスを隠しています。押すと表示します。",
    "Start Scan": "検索開始",
    "Stop Scan": "検索停止",
}

D["anytext"] = {
    "Find": "検索語",
    "Replace": "置換語",
}

D["beatsync"] = {
    "Port": "ポート",
    "Pull Beatmaps": "譜面取込",
}

D["bl"] = {
    "Logout": "ログアウト",
    "Password": "パスワード",
    "Show Noticeboard in main menu": "メインメニューにお知らせを表示",
    "Check their settings for 'force' or 'hitbox'": "その人の設定の「force」か「hitbox」を確認してください",
    "Please log in or sign up in settings to post scores!": "スコア送信には設定からログインしてください",
}

# AntiVertigo の足場の設定（.bsml を BSML 本体が読む形なので、呼び出し元が bsml になる）
D["bsml"] = {
    "Platform Shape": "足場の形",
    "The shape of the platform": "足場の形",
    "Platform front side offset": "足場の前側の広さ",
    "Platform back side offset": "足場の後ろ側の広さ",
    "Platform left side offset": "足場の左側の広さ",
    "Platform right side offset": "足場の右側の広さ",
    "How far the platform extends to the front side": "足場を前側にどこまで広げるか",
    "How far the platform extends to the back side": "足場を後ろ側にどこまで広げるか",
    "How far the platform extends to the left side": "足場を左側にどこまで広げるか",
    "How far the platform extends to the right side": "足場を右側にどこまで広げるか",
}

D["imagecoverexpander"] = {
    "Brightness": "明るさ",
    "Opacity": "不透明度",
}

D["jdfixer"] = {
    "Add new condition for the current preset": "このプリセットに条件を追加",
    "Add new preset": "プリセットを追加",
    "Delete current preset": "このプリセットを削除",
    "Prevents calculated jump distance from being under or over configured bounds": "計算したジャンプ距離が、設定した範囲から外れないようにします",
    "Remove specific settings for this level": "このレベル専用の設定を外す",
    "Set To Level Values": "レベルの値にする",
    "The default for the level (green) and the applied (red) values. Click to set to the level default": "レベルの既定値（緑）と、適用中の値（赤）。押すとレベルの既定値に戻します",
    "Whether to use half values for distance and duration, which correspond to the real note movements but may be less familiar": "距離と時間を半分の値で表示するか。実際のノーツの動きに合いますが、見慣れないかもしれません",
    "to": "〜",
    "Fix Practice NJS": "練習NJS固定",
}

D["noodleextensions"] = {
    "Smart: Changes obstacle material based on Chroma coloring to simulate distortion opacity.\nSemiBasic: switches material opaque/transparent if dissolve is applied.\nBasic: retains opaque material color once dissolve is applied":
        "Smart: Chroma の色に合わせて壁のマテリアルを変え、ゆがみの透け具合を再現します。\nSemiBasic: 溶かすと、不透明と透明のマテリアルを切り替えます。\nBasic: 溶かしたあとも不透明な色を保ちます",
    "Disable Qosmetics models on NE maps": "NE 譜面で Qosmetics を使わない",
}

D["RandomSongPicker"] = {
    "ControllerButton Binding": "ボタンの割り当て",
}

D["airbuds-search"] = {
    "Clear": "消去",
    "Enables better Japanese song/artist search by converting Japanese to romaji for additional search.": "日本語をローマ字でも検索し、曲やアーティストを見つかりやすくします。",
}

D["songdownloader"] = {
    "AutoMapper": "自動生成",
}

D["songplayhistory"] = {
    "EnableSongPlayHistory": "プレイ履歴を有効にする",
    "ShowFailed": "失敗した記録も表示",
    "SortByDate": "新しい順に並べる",
    "AverageAccuracy": "平均精度",
}

D["tricksaberlite"] = {
    "--- Left Saber Controls ---": "--- 左セイバーの操作 ---",
    "--- Right Saber Controls ---": "--- 右セイバーの操作 ---",
}
