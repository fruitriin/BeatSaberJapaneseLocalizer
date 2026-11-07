// 曲一覧のセルの曲名欄を直す。
// SongCore は曲名欄の高さを 6.5 に縮める。日本語（本体の日本語フォント）と英字（Teko）が同じ行に混ざると
// 行の高さが 6.55 になり、高さからはみ出したと判定されて「祝福/The Blessing」が「祝…」に省略される。
// そこで高さを 7 に広げる。未翻訳収集モードのときは、まだ省略されている曲名の寸法をログに出す。
#include "Config.hpp"
#include "Translator.hpp"
#include "logging.hpp"

#include "beatsaber-hook/shared/utils/hooking.hpp"
#include "bsml/shared/BSML/MainThreadScheduler.hpp"
#include "GlobalNamespace/BeatmapLevel.hpp"
#include "GlobalNamespace/LevelListTableCell.hpp"
#include "TMPro/TMP_LineInfo.hpp"
#include "TMPro/TMP_TextInfo.hpp"
#include "TMPro/TextMeshProUGUI.hpp"
#include "UnityEngine/Rect.hpp"
#include "UnityEngine/RectTransform.hpp"

#include <set>

namespace {
    constexpr float SongNameHeight = 7.0f;
    std::set<std::string> logged;

    void FixHeight(GlobalNamespace::LevelListTableCell* cell) {
        if (!getModConfig().FixSongNameHeight.GetValue()) return;
        auto text = cell->_songNameText;
        if (!text) return;
        auto rt = text->get_rectTransform();
        auto size = rt->get_sizeDelta();
        if (size.y < SongNameHeight) {
            size.y = SongNameHeight;
            rt->set_sizeDelta(size);
        }
    }

    void LogCell(UnityW<GlobalNamespace::LevelListTableCell> cell) {
        if (!cell) return;
        auto text = cell->_songNameText;
        if (!text) return;
        if (!text->get_isTextOverflowing()) return;
        std::string content = text->get_text();
        if (!logged.insert(content).second) return;

        auto rect = text->get_rectTransform()->get_rect();
        auto sizeDelta = text->get_rectTransform()->get_sizeDelta();
        auto preferred = text->GetPreferredValues(text->get_text());
        auto info = text->get_textInfo();
        std::string lines;
        if (info) {
            for (int i = 0; i < info->lineCount && i < 3; i++) {
                auto const& l = info->lineInfo[i];
                lines += fmt::format("[asc {:.2f} desc {:.2f} h {:.2f} w {:.2f}] ", l.ascender, l.descender, l.lineHeight, l.width);
            }
        }
        INFO("cell overflow: '{}' rect {:.2f}x{:.2f} sizeDelta {:.2f}x{:.2f} preferred {:.2f}x{:.2f} wrap {} overflowMode {} firstOverflow {} chars {} lines {} {}",
             content, rect.width, rect.height, sizeDelta.x, sizeDelta.y, preferred.x, preferred.y, text->get_enableWordWrapping(),
             (int) text->get_overflowMode(), text->get_firstOverflowCharacterIndex(), info ? info->characterCount : -1, info ? info->lineCount : -1, lines);
    }
}

MAKE_HOOK_MATCH(LevelListTableCell_SetDataFromLevelAsync, &GlobalNamespace::LevelListTableCell::SetDataFromLevelAsync, void,
                GlobalNamespace::LevelListTableCell* self, GlobalNamespace::BeatmapLevel* level, bool isFavorite, bool isPromoted, bool isUpdated,
                bool interactable) {
    LevelListTableCell_SetDataFromLevelAsync(self, level, isFavorite, isPromoted, isUpdated, interactable);
    FixHeight(self);
    // SongCore のフックがこのフックより後に動く場合に備えて、次のフレームでも直す
    UnityW<GlobalNamespace::LevelListTableCell> cell = self;
    BSML::MainThreadScheduler::ScheduleNextFrame([cell]() mutable {
        try {
            if (!cell) return;
            FixHeight(cell.ptr());
            if (JapaneseLocalizer::IsCollecting()) LogCell(cell);
        } catch (std::exception const& e) {
            ERROR("cell diagnostics failed: {}", e.what());
        }
    });
}

namespace JapaneseLocalizer {
    void InstallCellDiagnostics() {
        static constexpr auto logger = Paper::ConstLoggerContext(MOD_ID "_CellDiagnostics");
        INSTALL_HOOK(logger, LevelListTableCell_SetDataFromLevelAsync);
    }
}
