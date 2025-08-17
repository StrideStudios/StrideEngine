#include "Input.h"

static CInput gInput;

void CInput::tick() {
	gInput.mMouseVelocity = {0.f, 0.f};
}

void CInput::process(const SDL_Event& inEvent) {
	switch (inEvent.type) {
		case SDL_EVENT_KEY_DOWN:
			gInput.mKeyMap[static_cast<EKey>(inEvent.key.key)] = true;
			break;
		case SDL_EVENT_KEY_UP:
			gInput.mKeyMap[static_cast<EKey>(inEvent.key.key)] = false;
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			gInput.mMouseArray[inEvent.button.button] = true;
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			gInput.mMouseArray[inEvent.button.button] = false;
			break;
		case SDL_EVENT_MOUSE_MOTION:
			gInput.mMousePosition = {inEvent.motion.x, inEvent.motion.y};
			gInput.mMouseVelocity += Vector2f{inEvent.motion.xrel, inEvent.motion.yrel};
			break;
		default:
			break;
	}
	setShowMouse(!getMousePressed(3)); //Right click (engine only)
}

bool CInput::shouldShowMouse() {
	return gInput.mShowMouse;
}

void CInput::setShowMouse(const bool inShowMouse) {
	gInput.mShowMouse = inShowMouse;
}


bool CInput::getKeyPressed(const EKey inKey) {
	return gInput.mKeyMap[inKey];
}

// 1 is left-click, 2 is middle-click, 3 is right-click
bool CInput::getMousePressed(const int32 inMouseButton) {
	return gInput.mMouseArray[inMouseButton];
}

Vector2f CInput::getMousePosition() {
	return gInput.mMousePosition;
}

Vector2f CInput::getMouseVelocity() {
	return gInput.mMouseVelocity;
}
