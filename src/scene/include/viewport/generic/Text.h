#pragma once

#include <vector>

#include "viewport/Sprite.h"

class CFont {
public:

	struct Letter {
		Vector2f mUV0, mUV1;
		Extent32 mBearing;
		int64 mAdvance = 0;

		friend CArchive& operator<<(CArchive& inArchive, const Letter& inLetter) {
			inArchive << inLetter.mUV0 << inLetter.mUV1;
			inArchive << inLetter.mBearing;
			inArchive << inLetter.mAdvance;
			return inArchive;
		}

		friend CArchive& operator>>(CArchive& inArchive, Letter& inLetter) {
			inArchive >> inLetter.mUV0 >> inLetter.mUV1;
			inArchive >> inLetter.mBearing;
			inArchive >> inLetter.mAdvance;
			return inArchive;
		}
	};

	EXPORT void forEachLetter(const std::string& text, std::function<void(const Vector2f& pos, const Vector2f& uv0, const Vector2f& uv1)> f);

	std::string mName;

	std::unordered_map<uint8, Letter> letters;

	SImage_T* mAtlasImage;

	Extent32u mAtlasSize = {0, 0};
	int mSize = 0;
	float mLineSpacing = 0.f;
	float mAscenderPx = 0.f;
	float mDescenderPx = 0.f;

	friend CArchive& operator<<(CArchive& inArchive, const CFont& inFont) {
		inArchive << inFont.mName;
		inArchive << inFont.mAtlasSize;
		inArchive << inFont.mSize;
		inArchive << inFont.mLineSpacing;
		inArchive << inFont.mAscenderPx;
		inArchive << inFont.mDescenderPx;
		inArchive << inFont.letters;
		return inArchive;
	}

	friend CArchive& operator>>(CArchive& inArchive, CFont& inFont) {
		inArchive >> inFont.mName;
		inArchive >> inFont.mAtlasSize;
		inArchive >> inFont.mSize;
		inArchive >> inFont.mLineSpacing;
		inArchive >> inFont.mAscenderPx;
		inArchive >> inFont.mDescenderPx;
		inArchive >> inFont.letters;
		return inArchive;
	}
};

class CTextSprite : public CSprite {

public:

	CTextSprite() = default;

	CTextSprite(const std::string& inText) : m_Text(inText) {}

	std::string getText() const { return m_Text; }

	void setText(const std::string& inText) {
		m_Text = inText;
	}

private:
	std::string m_Text = "Text";
};
