# 1行に収まらない訳の短縮案

overflow-list.md の 28 件について、短い訳の候補を挙げる。
「目安の字数」は、実機で測った幅から見積もった、1行に収まる全角の字数である。訳文の幅は全角1字あたり約 4 だったので、枠の幅を 4 で割った。英数字は全角の半分ほどの幅として数える。

| Mod | 原文 | いまの訳 | 目安の字数 | 候補（第1案 / 第2案） |
|---|---|---|---|---|
| BetterSongSearch | Clear | クリア | 2 | 消去 / 解除 |
| BetterSongSearch | Curated Maps | キュレーション済みの譜面 | 9 | キュレート済み / 厳選された譜面 |
| BetterSongSearch | Downloaded | ダウンロード済み | 5 | DL 済み / 取得済み |
| BetterSongSearch | FILTERS | フィルター | 2 | 絞込 / 条件 |
| BetterSongSearch | Filter by Uploader | アップロード者で絞り込む | 6 | 投稿者で絞込 / 投稿者で検索 |
| BetterSongSearch | Local score | 自分のスコア | 5 | 自己スコア / マイスコア |
| BetterSongSearch | Map Style | 譜面のスタイル | 6 | スタイル / 譜面傾向 |
| BetterSongSearch | More | その他 | 2 | 他 / 詳細 |
| BetterSongSearch | Sort | 並べ替え | 2 | 順序 / 並順 |
| BetterSongSearch | Upload Details | アップロードの詳細 | 6 | 投稿情報 / 投稿の詳細 |
| BetterSongSearch | Uploader(s) | アップロード者 | 5 | 投稿者 / 作者 |
| RecentlyPlayed | Max History Length (after next map) | 履歴の最大件数（次の譜面から反映） | 13 | 履歴の上限（次の譜面から） / 最大履歴数（次回から） |
| bsl | Clear Sort and Filter in Playlists | プレイリストでは並べ替えとフィルターを解除 | 12 | プレイリストで条件を解除 / リストでは並替・絞込を解除 |
| bsl | Extend Basegame scrollbar | 本体のスクロールバーを拡張 | 12 | スクロールバーを拡張 / スクロールバー拡張 |
| bsl | Filter | フィルター | 2 | 絞込 / 条件 |
| bsl | Preferred Leaderboard | 優先するリーダーボード | 8 | 優先ランキング / 使うランキング |
| bsl | Sort | 並べ替え | 2 | 順序 / 並順 |
| jdfixer | Disable Mod | Mod を無効にする | 3（原文も2行） | Mod 無効 / 無効化 |
| jdfixer | Fix Practice NJS | 練習モードの NJS を固定 | 5 | 練習 NJS 固定 / 練習時 NJS 固定 |
| jdfixer | Half Values | 半分の値で表示 | 5 | 半分の値 / 半値表示 |
| jdfixer | Level Specific Settings | レベルごとの設定 | 7 | レベル別設定 / 曲ごとの設定 |
| playlistcore | Playlist Scroll Speed | プレイリストのスクロール速度 | 9 | スクロール速度 / リストのスクロール速度 |
| playlistcore | Reload All Playlists | すべてのプレイリストを読み直す | 7 | すべて読み直す / 全リスト再読込 |
| qbeatsaberplus-gametweaker | Remove cut particles | 切ったときのパーティクルを消す | 13 | 切断パーティクルを消す / 切った時の粒子を消す |
| qbeatsaberplus-gametweaker | Remove floor burn particles | 床の焦げのパーティクルを消す | 13 | 床焦げのパーティクルを消す / 床焦げの粒子を消す |
| qbeatsaberplus-gametweaker | Remove saber clash effect | セイバー同士がぶつかる演出を消す | 13 | セイバー衝突の演出を消す / 衝突エフェクトを消す |
| songdownloader | Download To Playlist | プレイリストにダウンロード | 12（原文も2行） | プレイリストへ DL / リストにダウンロード |
| songdownloader | Service | サービス | 3 | 取得元 / API |

## 補足

- 幅が 8〜9 の欄（Clear、FILTERS、More、Sort、Filter）は、全角2字が限度である。並べ替え・フィルターは本体の UI では「並べ替え」「フィルター」と呼んでいるので、2字にすると本体と呼び名がずれる。
- 目安の字数は概算である。採用した訳は、未翻訳収集モードでもう一度 overflow.json を取って確かめる。

## 採用した訳（2026-11-07）

第1案を採用した。ただし Reload All Playlists は「曲目リスト再読込」にした。PP Accuracy Scale (SS Only) は「PP 精度の倍率（SS のみ）」にした（値の表示と重なっていたため）。辞書の元データは tools/dicts_short.py にある。
