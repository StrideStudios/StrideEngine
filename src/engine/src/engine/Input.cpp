#include "engine/Input.h"
#include <SDL3/SDL_events.h>

static CInput gInput;

// A map for converting SDLK keys to Engine Keys
static std::map keycodes = {
	std::make_pair(SDLK_UNKNOWN, EKey::UNKNOWN),
	std::make_pair(SDLK_RETURN, EKey::RETURN),
	std::make_pair(SDLK_ESCAPE, EKey::ESCAPE),
	std::make_pair(SDLK_BACKSPACE, EKey::BACKSPACE),
	std::make_pair(SDLK_TAB, EKey::TAB),
	std::make_pair(SDLK_SPACE, EKey::SPACE),
	std::make_pair(SDLK_EXCLAIM, EKey::EXCLAIM),
	std::make_pair(SDLK_DBLAPOSTROPHE, EKey::DBLAPOSTROPHE),
	std::make_pair(SDLK_HASH, EKey::HASH),
	std::make_pair(SDLK_DOLLAR, EKey::DOLLAR),
	std::make_pair(SDLK_PERCENT, EKey::PERCENT),
	std::make_pair(SDLK_AMPERSAND, EKey::AMPERSAND),
	std::make_pair(SDLK_APOSTROPHE, EKey::APOSTROPHE),
	std::make_pair(SDLK_LEFTPAREN, EKey::LEFTPAREN),
	std::make_pair(SDLK_RIGHTPAREN, EKey::RIGHTPAREN),
	std::make_pair(SDLK_ASTERISK, EKey::ASTERISK),
	std::make_pair(SDLK_PLUS, EKey::PLUS),
	std::make_pair(SDLK_COMMA, EKey::COMMA),
	std::make_pair(SDLK_MINUS, EKey::MINUS),
	std::make_pair(SDLK_PERIOD, EKey::PERIOD),
	std::make_pair(SDLK_SLASH, EKey::SLASH),
	std::make_pair(SDLK_0, EKey::ZERO),
	std::make_pair(SDLK_1, EKey::ONE),
	std::make_pair(SDLK_2, EKey::TWO),
	std::make_pair(SDLK_3, EKey::THREE),
	std::make_pair(SDLK_4, EKey::FOUR),
	std::make_pair(SDLK_5, EKey::FIVE),
	std::make_pair(SDLK_6, EKey::SIX),
	std::make_pair(SDLK_7, EKey::SEVEN),
	std::make_pair(SDLK_8, EKey::EIGHT),
	std::make_pair(SDLK_9, EKey::NINE),
	std::make_pair(SDLK_COLON, EKey::COLON),
	std::make_pair(SDLK_SEMICOLON, EKey::SEMICOLON),
	std::make_pair(SDLK_LESS, EKey::LESS),
	std::make_pair(SDLK_EQUALS, EKey::EQUALS),
	std::make_pair(SDLK_GREATER, EKey::GREATER),
	std::make_pair(SDLK_QUESTION, EKey::QUESTION),
	std::make_pair(SDLK_AT, EKey::AT),
	std::make_pair(SDLK_LEFTBRACKET, EKey::LEFTBRACKET),
	std::make_pair(SDLK_BACKSLASH, EKey::BACKSLASH),
	std::make_pair(SDLK_RIGHTBRACKET, EKey::RIGHTBRACKET),
	std::make_pair(SDLK_CARET, EKey::CARET),
	std::make_pair(SDLK_UNDERSCORE, EKey::UNDERSCORE),
	std::make_pair(SDLK_GRAVE, EKey::GRAVE),
	std::make_pair(SDLK_A, EKey::A),
	std::make_pair(SDLK_B, EKey::B),
	std::make_pair(SDLK_C, EKey::C),
	std::make_pair(SDLK_D, EKey::D),
	std::make_pair(SDLK_E, EKey::E),
	std::make_pair(SDLK_F, EKey::F),
	std::make_pair(SDLK_G, EKey::G),
	std::make_pair(SDLK_H, EKey::H),
	std::make_pair(SDLK_I, EKey::I),
	std::make_pair(SDLK_J, EKey::J),
	std::make_pair(SDLK_K, EKey::K),
	std::make_pair(SDLK_L, EKey::L),
	std::make_pair(SDLK_M, EKey::M),
	std::make_pair(SDLK_N, EKey::N),
	std::make_pair(SDLK_O, EKey::O),
	std::make_pair(SDLK_P, EKey::P),
	std::make_pair(SDLK_Q, EKey::Q),
	std::make_pair(SDLK_R, EKey::R),
	std::make_pair(SDLK_S, EKey::S),
	std::make_pair(SDLK_T, EKey::T),
	std::make_pair(SDLK_U, EKey::U),
	std::make_pair(SDLK_V, EKey::V),
	std::make_pair(SDLK_W, EKey::W),
	std::make_pair(SDLK_X, EKey::X),
	std::make_pair(SDLK_Y, EKey::Y),
	std::make_pair(SDLK_Z, EKey::Z),
	std::make_pair(SDLK_LEFTBRACE, EKey::LEFTBRACE),
	std::make_pair(SDLK_PIPE, EKey::PIPE),
	std::make_pair(SDLK_RIGHTBRACE, EKey::RIGHTBRACE),
	std::make_pair(SDLK_TILDE, EKey::TILDE),
	std::make_pair(SDLK_DELETE, EKey::DELETE),
	std::make_pair(SDLK_PLUSMINUS, EKey::PLUSMINUS),
	std::make_pair(SDLK_CAPSLOCK, EKey::CAPSLOCK),
	std::make_pair(SDLK_F1, EKey::F1),
	std::make_pair(SDLK_F2, EKey::F2),
	std::make_pair(SDLK_F3, EKey::F3),
	std::make_pair(SDLK_F4, EKey::F4),
	std::make_pair(SDLK_F5, EKey::F5),
	std::make_pair(SDLK_F6, EKey::F6),
	std::make_pair(SDLK_F7, EKey::F7),
	std::make_pair(SDLK_F8, EKey::F8),
	std::make_pair(SDLK_F9, EKey::F9),
	std::make_pair(SDLK_F10, EKey::F10),
	std::make_pair(SDLK_F11, EKey::F11),
	std::make_pair(SDLK_F12, EKey::F12),
	std::make_pair(SDLK_PRINTSCREEN, EKey::PRINTSCREEN),
	std::make_pair(SDLK_SCROLLLOCK, EKey::SCROLLLOCK),
	std::make_pair(SDLK_PAUSE, EKey::PAUSE),
	std::make_pair(SDLK_INSERT, EKey::INSERT),
	std::make_pair(SDLK_HOME, EKey::HOME),
	std::make_pair(SDLK_PAGEUP, EKey::PAGEUP),
	std::make_pair(SDLK_END, EKey::END),
	std::make_pair(SDLK_PAGEDOWN, EKey::PAGEDOWN),
	std::make_pair(SDLK_RIGHT, EKey::RIGHT),
	std::make_pair(SDLK_LEFT, EKey::LEFT),
	std::make_pair(SDLK_DOWN, EKey::DOWN),
	std::make_pair(SDLK_UP, EKey::UP),
	std::make_pair(SDLK_NUMLOCKCLEAR, EKey::NUMLOCKCLEAR),
	std::make_pair(SDLK_KP_DIVIDE, EKey::KP_DIVIDE),
	std::make_pair(SDLK_KP_MULTIPLY, EKey::KP_MULTIPLY),
	std::make_pair(SDLK_KP_MINUS, EKey::KP_MINUS),
	std::make_pair(SDLK_KP_PLUS, EKey::KP_PLUS),
	std::make_pair(SDLK_KP_ENTER, EKey::KP_ENTER),
	std::make_pair(SDLK_KP_1, EKey::KP_1),
	std::make_pair(SDLK_KP_2, EKey::KP_2),
	std::make_pair(SDLK_KP_3, EKey::KP_3),
	std::make_pair(SDLK_KP_4, EKey::KP_4),
	std::make_pair(SDLK_KP_5, EKey::KP_5),
	std::make_pair(SDLK_KP_6, EKey::KP_6),
	std::make_pair(SDLK_KP_7, EKey::KP_7),
	std::make_pair(SDLK_KP_8, EKey::KP_8),
	std::make_pair(SDLK_KP_9, EKey::KP_9),
	std::make_pair(SDLK_KP_0, EKey::KP_0),
	std::make_pair(SDLK_KP_PERIOD, EKey::KP_PERIOD),
	std::make_pair(SDLK_APPLICATION, EKey::APPLICATION),
	std::make_pair(SDLK_POWER, EKey::POWER),
	std::make_pair(SDLK_KP_EQUALS, EKey::KP_EQUALS),
	std::make_pair(SDLK_F13, EKey::F13),
	std::make_pair(SDLK_F14, EKey::F14),
	std::make_pair(SDLK_F15, EKey::F15),
	std::make_pair(SDLK_F16, EKey::F16),
	std::make_pair(SDLK_F17, EKey::F17),
	std::make_pair(SDLK_F18, EKey::F18),
	std::make_pair(SDLK_F19, EKey::F19),
	std::make_pair(SDLK_F20, EKey::F20),
	std::make_pair(SDLK_F21, EKey::F21),
	std::make_pair(SDLK_F22, EKey::F22),
	std::make_pair(SDLK_F23, EKey::F23),
	std::make_pair(SDLK_F24, EKey::F24),
	std::make_pair(SDLK_EXECUTE, EKey::EXECUTE),
	std::make_pair(SDLK_HELP, EKey::HELP),
	std::make_pair(SDLK_MENU, EKey::MENU),
	std::make_pair(SDLK_SELECT, EKey::SELECT),
	std::make_pair(SDLK_STOP, EKey::STOP),
	std::make_pair(SDLK_AGAIN, EKey::AGAIN),
	std::make_pair(SDLK_UNDO, EKey::UNDO),
	std::make_pair(SDLK_CUT, EKey::CUT),
	std::make_pair(SDLK_COPY, EKey::COPY),
	std::make_pair(SDLK_PASTE, EKey::PASTE),
	std::make_pair(SDLK_FIND, EKey::FIND),
	std::make_pair(SDLK_MUTE, EKey::MUTE),
	std::make_pair(SDLK_VOLUMEUP, EKey::VOLUMEUP),
	std::make_pair(SDLK_VOLUMEDOWN, EKey::VOLUMEDOWN),
	std::make_pair(SDLK_KP_COMMA, EKey::KP_COMMA),
	std::make_pair(SDLK_KP_EQUALSAS400, EKey::KP_EQUALSAS400),
	std::make_pair(SDLK_ALTERASE, EKey::ALTERASE),
	std::make_pair(SDLK_SYSREQ, EKey::SYSREQ),
	std::make_pair(SDLK_CANCEL, EKey::CANCEL),
	std::make_pair(SDLK_CLEAR, EKey::CLEAR),
	std::make_pair(SDLK_PRIOR, EKey::PRIOR),
	std::make_pair(SDLK_RETURN2, EKey::RETURN2),
	std::make_pair(SDLK_SEPARATOR, EKey::SEPARATOR),
	std::make_pair(SDLK_OUT, EKey::OUT),
	std::make_pair(SDLK_OPER, EKey::OPER),
	std::make_pair(SDLK_CLEARAGAIN, EKey::CLEARAGAIN),
	std::make_pair(SDLK_CRSEL, EKey::CRSEL),
	std::make_pair(SDLK_EXSEL, EKey::EXSEL),
	std::make_pair(SDLK_KP_00, EKey::KP_00),
	std::make_pair(SDLK_KP_000, EKey::KP_000),
	std::make_pair(SDLK_THOUSANDSSEPARATOR, EKey::THOUSANDSSEPARATOR),
	std::make_pair(SDLK_DECIMALSEPARATOR, EKey::DECIMALSEPARATOR),
	std::make_pair(SDLK_CURRENCYUNIT, EKey::CURRENCYUNIT),
	std::make_pair(SDLK_CURRENCYSUBUNIT, EKey::CURRENCYSUBUNIT),
	std::make_pair(SDLK_KP_LEFTPAREN, EKey::KP_LEFTPAREN),
	std::make_pair(SDLK_KP_RIGHTPAREN, EKey::KP_RIGHTPAREN),
	std::make_pair(SDLK_KP_LEFTBRACE, EKey::KP_LEFTBRACE),
	std::make_pair(SDLK_KP_RIGHTBRACE, EKey::KP_RIGHTBRACE),
	std::make_pair(SDLK_KP_TAB, EKey::KP_TAB),
	std::make_pair(SDLK_KP_BACKSPACE, EKey::KP_BACKSPACE),
	std::make_pair(SDLK_KP_A, EKey::KP_A),
	std::make_pair(SDLK_KP_B, EKey::KP_B),
	std::make_pair(SDLK_KP_C, EKey::KP_C),
	std::make_pair(SDLK_KP_D, EKey::KP_D),
	std::make_pair(SDLK_KP_E, EKey::KP_E),
	std::make_pair(SDLK_KP_F, EKey::KP_F),
	std::make_pair(SDLK_KP_XOR, EKey::KP_XOR),
	std::make_pair(SDLK_KP_POWER, EKey::KP_POWER),
	std::make_pair(SDLK_KP_PERCENT, EKey::KP_PERCENT),
	std::make_pair(SDLK_KP_LESS, EKey::KP_LESS),
	std::make_pair(SDLK_KP_GREATER, EKey::KP_GREATER),
	std::make_pair(SDLK_KP_AMPERSAND, EKey::KP_AMPERSAND),
	std::make_pair(SDLK_KP_DBLAMPERSAND, EKey::KP_DBLAMPERSAND),
	std::make_pair(SDLK_KP_VERTICALBAR, EKey::KP_VERTICALBAR),
	std::make_pair(SDLK_KP_DBLVERTICALBAR, EKey::KP_DBLVERTICALBAR),
	std::make_pair(SDLK_KP_COLON, EKey::KP_COLON),
	std::make_pair(SDLK_KP_HASH, EKey::KP_HASH),
	std::make_pair(SDLK_KP_SPACE, EKey::KP_SPACE),
	std::make_pair(SDLK_KP_AT, EKey::KP_AT),
	std::make_pair(SDLK_KP_EXCLAM, EKey::KP_EXCLAM),
	std::make_pair(SDLK_KP_MEMSTORE, EKey::KP_MEMSTORE),
	std::make_pair(SDLK_KP_MEMRECALL, EKey::KP_MEMRECALL),
	std::make_pair(SDLK_KP_MEMCLEAR, EKey::KP_MEMCLEAR),
	std::make_pair(SDLK_KP_MEMADD, EKey::KP_MEMADD),
	std::make_pair(SDLK_KP_MEMSUBTRACT, EKey::KP_MEMSUBTRACT),
	std::make_pair(SDLK_KP_MEMMULTIPLY, EKey::KP_MEMMULTIPLY),
	std::make_pair(SDLK_KP_MEMDIVIDE, EKey::KP_MEMDIVIDE),
	std::make_pair(SDLK_KP_PLUSMINUS, EKey::KP_PLUSMINUS),
	std::make_pair(SDLK_KP_CLEAR, EKey::KP_CLEAR),
	std::make_pair(SDLK_KP_CLEARENTRY, EKey::KP_CLEARENTRY),
	std::make_pair(SDLK_KP_BINARY, EKey::KP_BINARY),
	std::make_pair(SDLK_KP_OCTAL, EKey::KP_OCTAL),
	std::make_pair(SDLK_KP_DECIMAL, EKey::KP_DECIMAL),
	std::make_pair(SDLK_KP_HEXADECIMAL, EKey::KP_HEXADECIMAL),
	std::make_pair(SDLK_LCTRL, EKey::LCTRL),
	std::make_pair(SDLK_LSHIFT, EKey::LSHIFT),
	std::make_pair(SDLK_LALT, EKey::LALT),
	std::make_pair(SDLK_LGUI, EKey::LGUI),
	std::make_pair(SDLK_RCTRL, EKey::RCTRL),
	std::make_pair(SDLK_RSHIFT, EKey::RSHIFT),
	std::make_pair(SDLK_RALT, EKey::RALT),
	std::make_pair(SDLK_RGUI, EKey::RGUI),
	std::make_pair(SDLK_MODE, EKey::MODE),
	std::make_pair(SDLK_SLEEP, EKey::SLEEP),
	std::make_pair(SDLK_WAKE, EKey::WAKE),
	std::make_pair(SDLK_CHANNEL_INCREMENT, EKey::CHANNEL_INCREMENT),
	std::make_pair(SDLK_CHANNEL_DECREMENT, EKey::CHANNEL_DECREMENT),
	std::make_pair(SDLK_MEDIA_PLAY, EKey::MEDIA_PLAY),
	std::make_pair(SDLK_MEDIA_PAUSE, EKey::MEDIA_PAUSE),
	std::make_pair(SDLK_MEDIA_RECORD, EKey::MEDIA_RECORD),
	std::make_pair(SDLK_MEDIA_FAST_FORWARD, EKey::MEDIA_FAST_FORWARD),
	std::make_pair(SDLK_MEDIA_REWIND, EKey::MEDIA_REWIND),
	std::make_pair(SDLK_MEDIA_NEXT_TRACK, EKey::MEDIA_NEXT_TRACK),
	std::make_pair(SDLK_MEDIA_PREVIOUS_TRACK, EKey::MEDIA_PREVIOUS_TRACK),
	std::make_pair(SDLK_MEDIA_STOP, EKey::MEDIA_STOP),
	std::make_pair(SDLK_MEDIA_EJECT, EKey::MEDIA_EJECT),
	std::make_pair(SDLK_MEDIA_PLAY_PAUSE, EKey::MEDIA_PLAY_PAUSE),
	std::make_pair(SDLK_MEDIA_SELECT, EKey::MEDIA_SELECT),
	std::make_pair(SDLK_AC_NEW, EKey::AC_NEW),
	std::make_pair(SDLK_AC_OPEN, EKey::AC_OPEN),
	std::make_pair(SDLK_AC_CLOSE, EKey::AC_CLOSE),
	std::make_pair(SDLK_AC_EXIT, EKey::AC_EXIT),
	std::make_pair(SDLK_AC_SAVE, EKey::AC_SAVE),
	std::make_pair(SDLK_AC_PRINT, EKey::AC_PRINT),
	std::make_pair(SDLK_AC_PROPERTIES, EKey::AC_PROPERTIES),
	std::make_pair(SDLK_AC_SEARCH, EKey::AC_SEARCH),
	std::make_pair(SDLK_AC_HOME, EKey::AC_HOME),
	std::make_pair(SDLK_AC_BACK, EKey::AC_BACK),
	std::make_pair(SDLK_AC_FORWARD, EKey::AC_FORWARD),
	std::make_pair(SDLK_AC_STOP, EKey::AC_STOP),
	std::make_pair(SDLK_AC_REFRESH, EKey::AC_REFRESH),
	std::make_pair(SDLK_AC_BOOKMARKS, EKey::AC_BOOKMARKS),
	std::make_pair(SDLK_SOFTLEFT, EKey::SOFTLEFT),
	std::make_pair(SDLK_SOFTRIGHT, EKey::SOFTRIGHT),
	std::make_pair(SDLK_CALL, EKey::CALL),
	std::make_pair(SDLK_ENDCALL, EKey::ENDCALL),
	std::make_pair(SDLK_LEFT_TAB, EKey::LEFT_TAB),
	std::make_pair(SDLK_LEVEL5_SHIFT, EKey::LEVEL5_SHIFT),
	std::make_pair(SDLK_MULTI_KEY_COMPOSE, EKey::MULTI_KEY_COMPOSE),
	std::make_pair(SDLK_LMETA, EKey::LMETA),
	std::make_pair(SDLK_RMETA, EKey::RMETA),
	std::make_pair(SDLK_LHYPER, EKey::LHYPER),
	std::make_pair(SDLK_RHYPER, EKey::RHYPER)
};

void CInput::tick() {
	gInput.mMouseVelocity = {0.f, 0.f};
}

void CInput::process(const SDL_Event& inEvent) {
	switch (inEvent.type) {
		case SDL_EVENT_KEY_DOWN:
			gInput.mKeyMap[keycodes[inEvent.key.key]] = true;
			break;
		case SDL_EVENT_KEY_UP:
			gInput.mKeyMap[keycodes[inEvent.key.key]] = false;
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
