#include "mines_sys.h"
#include "nes_controllers.h"
#include "types.h"

const std::string nes_controllers::BUTTON_ALIAS_UP = "D-Pad_UP";
const std::string nes_controllers::BUTTON_ALIAS_DOWN = "D-Pad_DOWN";
const std::string nes_controllers::BUTTON_ALIAS_LEFT = "D-Pad_LEFT";
const std::string nes_controllers::BUTTON_ALIAS_RIGHT = "D-Pad_RIGHT";
const std::string nes_controllers::BUTTON_ALIAS_B = "ButtonB";
const std::string nes_controllers::BUTTON_ALIAS_A = "ButtonA";
const std::string nes_controllers::BUTTON_ALIAS_START = "ButtonStart";
const std::string nes_controllers::BUTTON_ALIAS_SELECT = "ButtonSelect";

nes_controllers::nes_controllers(mines_sys* sys) :
		m_sys(sys)
{
	// Reset key states.
	for (auto& state : m_buttonstates)
		state = false;

	// Create key mappings.
	m_keymap.insert(std::make_pair(BUTTON_ALIAS_UP, SDLK_UP));
	m_keymap.insert(std::make_pair(BUTTON_ALIAS_DOWN, SDLK_DOWN));
	m_keymap.insert(std::make_pair(BUTTON_ALIAS_LEFT, SDLK_LEFT));
	m_keymap.insert(std::make_pair(BUTTON_ALIAS_RIGHT, SDLK_RIGHT));
	m_keymap.insert(std::make_pair(BUTTON_ALIAS_B, SDLK_c));
	m_keymap.insert(std::make_pair(BUTTON_ALIAS_A, SDLK_x));
	m_keymap.insert(std::make_pair(BUTTON_ALIAS_START, SDLK_s));
	m_keymap.insert(std::make_pair(BUTTON_ALIAS_SELECT, SDLK_a));
}

void nes_controllers::on_key_event(int sdl2_key, bool press)
{
	if (sdl2_key == m_keymap[BUTTON_ALIAS_UP])
	{
		m_buttonstates[BUTTON_STATE_UP] = press;
	}
	else if (sdl2_key == m_keymap[BUTTON_ALIAS_DOWN])
	{
		m_buttonstates[BUTTON_STATE_DOWN] = press;
	}
	else if (sdl2_key == m_keymap[BUTTON_ALIAS_LEFT])
	{
		m_buttonstates[BUTTON_STATE_LEFT] = press;
	}
	else if (sdl2_key == m_keymap[BUTTON_ALIAS_RIGHT])
	{
		m_buttonstates[BUTTON_STATE_RIGHT] = press;
	}
	else if (sdl2_key == m_keymap[BUTTON_ALIAS_B])
	{
		m_buttonstates[BUTTON_STATE_B] = press;
	}
	else if (sdl2_key == m_keymap[BUTTON_ALIAS_A])
	{
		m_buttonstates[BUTTON_STATE_A] = press;
	}
	else if (sdl2_key == m_keymap[BUTTON_ALIAS_START])
	{
		m_buttonstates[BUTTON_STATE_START] = press;
	}
	else if (sdl2_key == m_keymap[BUTTON_ALIAS_SELECT])
	{
		m_buttonstates[BUTTON_STATE_SELECT] = press;
	}
}

u8 nes_controllers::get_controller_state(int n)
{
	u8 state = 0;

	if (n == 0)
	{
		int a = get_button_state(BUTTON_STATE_A);
		int b = get_button_state(BUTTON_STATE_B);
		int select = get_button_state(BUTTON_STATE_SELECT);		
		int start = get_button_state(BUTTON_STATE_START);
		int up = get_button_state(BUTTON_STATE_UP);
		int down = get_button_state(BUTTON_STATE_DOWN);
		int left = get_button_state(BUTTON_STATE_LEFT);
		int right = get_button_state(BUTTON_STATE_RIGHT);

		state |= a << 0;
		state |= b << 1;
		state |= select << 2;
		state |= start << 3;
		state |= up << 4;
		state |= down << 5;
		state |= left << 6;
		state |= right << 7;
	}

	return state;
}

void nes_controllers::write_strobe(u8 d)
{
	// On strobe high to low transition read the joypad data.
	if (m_strobe && !d)
	{
		m_regs[0] = get_controller_state(0);
		m_regs[1] = get_controller_state(1);		
	}		

	m_strobe = (d ? true : false);
}

u8 nes_controllers::read_state(int n)
{
	// If strobe is high reading will keep returning the current state of the first button (A).
	if (m_strobe)
		return 0x40 | (get_controller_state(n) & 1);

	// Get the status of a button and shift the register.
	u8 stat = 0x40 | (m_regs[n] & 1);
	m_regs[n] = 0x80 | (m_regs[n] >> 1);
	return stat;
}