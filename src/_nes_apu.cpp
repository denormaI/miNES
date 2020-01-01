#include "mines_sys.h"
#include "nes_cpu.h"
#include "nes_apu.h"
#include "nes_device.h"

/* 
	Source file named _nes_apu to disambiguate object file created with Nes_Apu.cpp.
*/

static const nes_apu* g_apu = NULL;

static int read_dmc(void*, cpu_addr_t addr)
{
	if (g_apu)
		return g_apu->cpu()->read(addr);
	else
		return 0;
}

nes_apu::nes_apu(mines_sys* sys) :
		m_sys(sys),
		m_cputime(0)
{
	blargg_err_t error = m_buf.sample_rate(SAMPLE_RATE);
	if (error)
		throw mines_exception("error setting sample rate");

	m_buf.clock_rate(1789773);
	m_apu.output(&m_buf);
	m_apu.volume(0.55);

	g_apu = this;
	m_apu.dmc_reader(read_dmc);

	// Initialise Sound_Queue.
	if (SDL_Init(SDL_INIT_AUDIO) < 0)
		throw mines_exception("SDL_Init() error");

	atexit(SDL_Quit);

	if (m_sq.init(SAMPLE_RATE))
		throw mines_exception("Sound_Queue.init() error");
}

void nes_apu::attach_devices(nes_device_map_t device_map)
{
	m_cpu = std::dynamic_pointer_cast<nes_cpu>(device_map["cpu"]);
	if (!m_cpu) throw mines_exception("nes_apu: Failed to get nes_cpu device.");
}

void nes_apu::step(const int n)
{
	m_cputime += n;
}

void nes_apu::end_frame()
{
	m_apu.end_frame(m_cputime);
	m_buf.end_frame(m_cputime);
	m_cputime = 0;

	/*	Read some samples out of Blip_Buffer if there are enough to
		fill the output buffer. */
	if (m_buf.samples_avail() >= OUT_SIZE)
	{
		size_t count = m_buf.read_samples(m_outbuf, OUT_SIZE);
		m_sq.write(m_outbuf, count);
		//m_wr.write(m_outbuf, count);
	}
}

u8 nes_apu::read(u16 addr) 
{
	if (addr == m_apu.status_addr)
		return m_apu.read_status(m_cputime);
}

void nes_apu::write(u16 addr, u8 d) 
{
	if (addr >= m_apu.start_addr && addr <= m_apu.end_addr)
		m_apu.write_register(m_cputime, addr, d);
}
