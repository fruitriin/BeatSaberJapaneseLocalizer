// 曲一覧のセルで曲名が「…」に省略されるときの寸法をログに出す（調査用。未翻訳収集モードのときだけ動く）
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
    std::set<std::string> logged;

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
    if (!JapaneseLocalizer::IsCollecting()) return;
    // ほかの Mod の後処理と配置が済んでから測る
    UnityW<GlobalNamespace::LevelListTableCell> cell = self;
    BSML::MainThreadScheduler::ScheduleNextFrame([cell]() {
        try {
            LogCell(cell);
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
