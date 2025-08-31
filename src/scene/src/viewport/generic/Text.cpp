#include "viewport/generic/Text.h"

void CFont::forEachLetter(const std::string& text, std::function<void(const Vector2f& pos, const Vector2f& uv0, const Vector2f& uv1)> f) {
    float x = 0.0f;
    int lineNum = 0;

    auto it = text.begin();
    const auto e = text.end();
    uint32 cp = 0;
    for (auto letter : text) {
        cp = static_cast<uint32>(letter);

        if (cp == static_cast<uint32>('\n')) {
            ++lineNum;
            x = 0.f;
            continue;
        }

        const auto& glyph = letters.contains(cp) ? letters.at(cp) : letters.at('?');
        const auto& uv0 = glyph.mUV0;
        const auto& uv1 = glyph.mUV1;

        float xpos = x + glyph.mBearing.x;
        float ypos = (mAscenderPx - glyph.mBearing.y) + static_cast<float>(lineNum) * mLineSpacing;

        f({xpos, ypos}, uv0, uv1);

        x += glyph.mAdvance;
    }
}