#pragma once

#include <cstdint>
#include <cstring>
#include <array>
#include <vector>
#include <memory>
#include <fstream>
#include <iterator>

#include "types.h"
#include "nes_device.h"
#include "ines_mapper.h"

class mines_sys;

class nes_cart : public nes_device
{
public:
	nes_cart(mines_sys*);
	~nes_cart() {}

	virtual void attach_devices(nes_device_map_t device_map);

	void load_rom(const std::string& romPath);

	void write(u16 address, u8 data);
	u8 read(u16 address);

	u8 read_chr(u16 addr);
	void write_chr(u16 addr, u8 d);

	int mirroring() const { return m_mapper->mirroring(); }

private:
	void load_ines_rom(const std::vector<u8>& filebuf);

	int m_romsize;
	mines_sys* m_sys;
	std::unique_ptr<ines_mapper> m_mapper;
};
