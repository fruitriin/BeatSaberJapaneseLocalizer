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

# ---- 2026-11-07 14:50 の実機の記録から ----
D["playlistmanager"] = {
    ": Add to playlist": ": プレイリストに追加",
    ": Clear selection": ": 選択を解除",
    ": Delete and remove": ": 削除してリストから外す",
    ": Delete selected songs": ": 選んだ曲を削除",
    ": Highlight difficulties": ": 難易度を強調表示",
    ": Invert selection": ": 選択を反転",
    ": Remove from playlist": ": プレイリストから外す",
    ": Select between": ": あいだを選択",
    ": Use covers as cover": ": 曲のジャケットを表紙に使う",
    "All songs": "すべての曲",
    "Are you sure?": "よろしいですか？",
    "Author": "作者",
    "CANCEL": "キャンセル",
    "CONFIRM": "決定",
    "Downloading songs...": "曲をダウンロード中...",
    "Has Characteristic": "モードあり",
    "Has Difficulty": "難易度あり",
    "No songs": "曲なし",
    "Only Owned": "所持している曲のみ",
    "Only Unplayed": "未プレイのみ",
    "Selection options": "選択のオプション",
    "Separate search and filter": "検索と絞込を別々にする",
    "Songs in playlist": "プレイリストの曲",
    "Sync search and filter": "検索と絞込を連動させる",
    "Syncing playlist...": "プレイリストを同期中...",
}

D["transitions"] = {
    "Show Warning": "警告を表示",
    "Shows the health warning when the game launches": "起動時に健康に関する注意を表示します",
}

# はみ出したので英語のままにする
D["EasyOffsetQuest"]["Z Offset"] = ""
D["songdownloader"]["AutoMapper"] = ""

# ---- 曲の「ルール変更」の CUSTOM タブ（ModifiersCoreQuest。項目は BeatLeader が追加する） ----
# 本体の公式訳: Faster Song = 曲スピードアップ、Super Fast Song = スーパー高速曲
D["ModifiersCoreQuest"] = {
    "Better Faster Song": "曲スピードアップ（改）",
    "Better Super Fast Song": "スーパー高速曲（改）",
    "Golf": "ゴルフ",
    "Dance Mode": "ダンスモード",
    "Score goes only to the Golf leaderboard, where lower accuracy is better. Needs accuracy of at most 50%":
        "スコアは Golf のリーダーボードにだけ送られ、精度が低いほど上位になります。精度 50% 以下が必要です",
    "Score is how much you dance. Energy also drains when you stop dancing for a while":
        "どれだけ踊ったかがスコアになります。しばらく踊らないと、エネルギーも減ります",
}

# SongPlayHistory 0.1.2 でトグルの表示名が設定のキー名から英語の文に変わった（コミット 55632bd）
D["songplayhistory"].update({
    "Enable Song Play History": "プレイ履歴を有効にする",
    "Show Failed": "失敗した記録も表示",
    "Average Accuracy": "平均精度",
    "Sort by Date": "新しい順に並べる",
})

# 改造版 BetterSongList 1.0.12 で足した設定
D["bsl"] = {
    "Random without repeats": "重複なしでランダム",
    "The random button picks every song in the current list once before any song comes up again": "ランダムのボタンで、いまの一覧の曲をひと通り選び終えるまで、同じ曲を選びません",
}
