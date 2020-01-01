#include "mines_sys.h"
#include "nes_device.h"
#include "nes_cpu.h"
#include "nes_ppu.h"
#include "nes_apu.h"
#include "nes_cart.h"
#include "nes_controllers.h"
#include "display_window.h"


mines_sys::mines_sys(const std::string& rom_path, bool keep_ppu_aspect) :
	m_cpu(std::make_shared<nes_cpu>(this)),
	m_ppu(std::make_shared<nes_ppu>(this)),
	m_apu(std::make_shared<nes_apu>(this)),
	m_cart(std::make_shared<nes_cart>(this)),
	m_controllers(std::make_shared<nes_controllers>(this)),
	m_display(std::make_shared<display_window>(keep_ppu_aspect)),
	m_should_quit(false),
	m_rom_path(rom_path)
{
}

mines_sys::~mines_sys() 
{

}

void mines_sys::entry() 
{
	nes_device_map_t m_device_map = {
		{ "cpu", m_cpu },
		{ "ppu", m_ppu },
		{ "apu", m_apu },
		{ "cart", m_cart },
		{ "controllers", m_controllers }
	};

	for (const auto& dev : m_device_map)
	{
		dev.second->attach_devices(m_device_map);
	}
	
	// Create the display window.
	m_display->create("miNES", 640, 480);

	// Register keyboard event handler.
	m_display->register_key_event_handler(
		[this](int sdl2_key, bool press)
		{
			m_controllers->on_key_event(sdl2_key, press);
		}
	);

	// Register frame end handler.
	m_ppu->register_frame_end_handler(
		[this](const ppu_types::display_bitmap_t& bitmap)
		{
			static auto start = std::chrono::steady_clock::now();
			auto end = std::chrono::steady_clock::now();

			while (std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() < 1000000.0 / 60.0)
				end = std::chrono::steady_clock::now();

			m_display->update_bitmap(bitmap);
			m_display->render();
			m_display->swap_buffers();

			m_apu->end_frame();

			m_should_quit = m_display->window_should_close();
			m_display->poll_events();

			start = std::chrono::steady_clock::now();
		}
	);

	m_cart->load_rom(m_rom_path);

	m_cpu->reset();
	m_ppu->reset();

	while (!m_should_quit)
	{
		m_cpu->step();
		m_apu->step();
		m_ppu->step();
		m_ppu->step();
		m_ppu->step();
	}
}
