#pragma once

#include <memory>
#include <fstream>
#include <iterator>
#include <vector>
#include <ctime>
#include <chrono>
#include <random>

#include "types.h"

typedef std::runtime_error mines_exception;

class nes_cpu;
class nes_ppu;
class nes_apu;
class nes_device;
class nes_cart;
class nes_controllers;
class display_window;

class mines_sys
{
public:
	mines_sys(const std::string& rom_path, bool keep_ppu_aspect);
	~mines_sys();
	
	void entry();

private:
	std::string m_rom_path;

	std::shared_ptr<nes_cpu> m_cpu;
	std::shared_ptr<nes_ppu> m_ppu;
	std::shared_ptr<nes_apu> m_apu;
	std::shared_ptr<nes_cart> m_cart;
	std::shared_ptr<nes_controllers> m_controllers;

	std::shared_ptr<display_window> m_display;

	bool m_should_quit;
};
