#pragma once

#include "scene/viewport/Sprite.h"

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
