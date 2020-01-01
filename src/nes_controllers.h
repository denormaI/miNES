#pragma once

#include <GL/glew.h>

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "types.h"
#include "nes_device.h"

class mines_sys;

class nes_controllers : public nes_device
{
public:
	nes_controllers(mines_sys*);
	~nes_controllers() {}

	virtual void attach_devices(nes_device_map_t device_map) {}

	u8 read(u16 /*address*/) { return 0; }
	void write(u16 /*address*/, u8 /*d*/) {}

	u8 get_controller_state(int n);

	void write_strobe(u8 d);
	u8 read_state(int n);

	void on_key_event(int sdl2_key, bool press);

private:
	bool get_button_state(int button) const { return m_buttonstates[button < 8 ? button : 0]; }

	static const std::string BUTTON_ALIAS_UP;
	static const std::string BUTTON_ALIAS_DOWN;
	static const std::string BUTTON_ALIAS_LEFT;
	static const std::string BUTTON_ALIAS_RIGHT;
	static const std::string BUTTON_ALIAS_B;
	static const std::string BUTTON_ALIAS_A;
	static const std::string BUTTON_ALIAS_START;
	static const std::string BUTTON_ALIAS_SELECT;

	// Controller to keyboard mapping.
	std::map<std::string, int> m_keymap;

	// Controller state array.
	std::array<bool, 8> m_buttonstates;

	enum
	{
		BUTTON_STATE_UP = 0,
		BUTTON_STATE_DOWN,
		BUTTON_STATE_LEFT,
		BUTTON_STATE_RIGHT,
		BUTTON_STATE_B,
		BUTTON_STATE_A,
		BUTTON_STATE_START,
		BUTTON_STATE_SELECT,
	};

	u8 m_regs[2];  // Joypad shift registers.
	bool m_strobe;        // Joypad strobe latch.

	mines_sys* m_sys;
};
