#pragma once

#include "Renderer.h"

namespace UI {
    bool CreateText(TextId id, _In_opt_z_ const wchar_t* text, TextFormatId formatId, TextAnchor anchor, float offsetX, float offsetY, float width, float height);
    TextDrawRequest* FindText(TextId id);
    bool SetText(TextId id, _In_opt_z_ const wchar_t* text);
    bool SetTextLayout(TextId id, TextAnchor anchor, float offsetX, float offsetY, float width, float height);
    void RemoveText(TextId id);
    void ClearTexts();
    const TextDrawRequests& Texts();
}
