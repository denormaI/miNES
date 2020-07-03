#include "nes_cart.h"
#include "mines_sys.h"
#include "ines_mapper0.h"
#include "ines_mapper1.h"
#include "ines_mapper2.h"
#include "ines_mapper3.h"
#include "ines_mapper7.h"


#include <iostream>

nes_cart::nes_cart(mines_sys* sys) : 
		m_sys(sys),
		m_romsize(0) 
{
	m_mapper.reset(new ines_dummy_mapper(sys));
}

void nes_cart::attach_devices(nes_device_map_t device_map)
{
}

void nes_cart::write(u16 address, u8 data)
{
	m_mapper->write(address, data);
}

u8 nes_cart::read(u16 address)
{
	return m_mapper->read(address);
}

u8 nes_cart::read_chr(u16 addr)
{
	return m_mapper->read_chr(addr);
}

void nes_cart::write_chr(u16 addr, u8 d)
{
	m_mapper->write_chr(addr, d);
}

void nes_cart::load_rom(const std::string& path)
{
	std::ifstream file;
	file.open(path.c_str(), std::ios::binary);
	if (!file.is_open()) 
	{
		throw mines_exception(
			std::string("Failed to open ") + std::string(path));
	}

	file.seekg(0, std::ios::end);
	m_romsize = file.tellg();
	file.seekg(0, std::ios::beg);

    std::cout << "reading file\n";
	const std::vector<u8> buffer(
		(std::istreambuf_iterator<char>(file)),
		(std::istreambuf_iterator<char>()));

	file.close();

	if (m_romsize < 16)
	{
		file.close();
		throw mines_exception("ROM is an invalid size");
	}

	// iNES: First 4 bytes are $4E $45 $53 $1A ("NES" followed by $1A)
	if (*((u32*)&buffer[0]) == 0x1a53454e) 
	{
		load_ines_rom(buffer);
	}
	else 
	{
		file.close();
		throw mines_exception("ROM format unrecognised");		
	}
}

void nes_cart::load_ines_rom(const std::vector<u8>& filebuf)
{
	if (filebuf[6] & 4) 
	{
		throw mines_exception("Trainer not supported.");
	}

	int mapperNum = (filebuf[7] & 0xf0) | (filebuf[6] >> 4);
	switch (mapperNum)
	{
		case 0:
			m_mapper.reset(new ines_mapper0(m_sys, filebuf));
			break;

#if 1
		case 1:
			m_mapper.reset(new ines_mapper1(m_sys, filebuf));
			break;
#endif

		case 2:
			m_mapper.reset(new ines_mapper2(m_sys, filebuf));
			break;

		case 3:
			m_mapper.reset(new ines_mapper3(m_sys, filebuf));
			break;

		case 7:
			m_mapper.reset(new ines_mapper7(m_sys, filebuf));
			break;			

		default:
			throw mines_exception("Mapper not supported");
	}
}
