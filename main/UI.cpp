#include "UI.h"

namespace {
    UI::TextDrawRequests textRequests;
}

bool UI::CreateText(TextId id, const wchar_t* text, TextFormatId formatId, TextAnchor anchor, float offsetX, float offsetY, float width, float height) {
    return textRequests.emplace(id, TextDrawRequest{ text ? text : L"", formatId, anchor, offsetX, offsetY, width, height }).second;
}

UI::TextDrawRequest* UI::FindText(TextId id) {
    const auto iterator = textRequests.find(id);
    return iterator != textRequests.end() ? &iterator->second : nullptr;
}

bool UI::SetText(TextId id, const wchar_t* text) {
    TextDrawRequest* textRequest = FindText(id);
    if (!textRequest) {
        return false;
    }

    textRequest->text = text ? text : L"";
    return true;
}

bool UI::SetTextLayout(TextId id, TextAnchor anchor, float offsetX, float offsetY, float width, float height) {
    TextDrawRequest* textRequest = FindText(id);
    if (!textRequest) {
        return false;
    }

    textRequest->anchor = anchor;
    textRequest->offsetX = offsetX;
    textRequest->offsetY = offsetY;
    textRequest->width = width;
    textRequest->height = height;
    return true;
}

void UI::RemoveText(TextId id) {
    textRequests.erase(id);
}

void UI::ClearTexts() {
    textRequests.clear();
}

const UI::TextDrawRequests& UI::Texts() {
    return textRequests;
}
