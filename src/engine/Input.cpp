#include "Input.h"

void CInput::processSDLEvent(const SDL_Event& inEvent) {
	switch (inEvent.type) {
		case SDL_EVENT_KEY_DOWN:
			mKeyMap[static_cast<EKey>(inEvent.key.key)] = true;
			break;
		case SDL_EVENT_KEY_UP:
			mKeyMap[static_cast<EKey>(inEvent.key.key)] = false;
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
			mMouseArray[inEvent.button.button] = true;
			break;
		case SDL_EVENT_MOUSE_BUTTON_UP:
			mMouseArray[inEvent.button.button] = false;
			break;
		case SDL_EVENT_MOUSE_MOTION:
			mMousePosition = {inEvent.motion.x, inEvent.motion.y};
			mMouseVelocity += Vector2f{inEvent.motion.xrel, inEvent.motion.yrel};
			break;
		default:
			break;
	}
}