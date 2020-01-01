#pragma once

#include "types.h"
#include "nes_device.h"

#define HAVE_CONFIG_H 1

#include "nes_apu/apu_snapshot.h"
#include "nes_apu/Nes_Apu.h"
#include "nes_apu/Blip_Buffer.h"
#include "Wave_Writer.hpp"
#include "Sound_Queue.h"

class mines_sys;
class nes_cpu;

class nes_apu : public nes_device 
{
public:
	nes_apu(mines_sys*);
	~nes_apu() {}

	virtual void attach_devices(nes_device_map_t device_map);

	u8 read(u16 addr);
	void write(u16 addr, u8 d);

	void step(const int n = 1);

	void end_frame();

	std::shared_ptr<nes_cpu> cpu() const { return m_cpu; }

private:
	static const size_t OUT_SIZE = 4096;
	static const long SAMPLE_RATE = 41000;	// TODO: 96000 may play back better on Linux. Investigate.

	Sound_Queue m_sq;
	blip_sample_t m_outbuf[OUT_SIZE];
	Blip_Buffer m_buf;
	Nes_Apu m_apu;
	int m_cputime;

	std::shared_ptr<nes_cpu> m_cpu;
	mines_sys* m_sys;
};
