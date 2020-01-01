#pragma once

#include "ines_mapper.h"

/*
CNROM
*/
class ines_mapper3 : public ines_mapper
{
public:
	ines_mapper3(mines_sys* sys, const std::vector<u8>& filebuf);

	virtual u8 read(u16 address);
	virtual void write(u16 address, u8 data);
	virtual u8 read_chr(u16 address);
	virtual void write_chr(u16 address, u8 d) {}

private:
	int m_bankSel;
	std::array<u8, 0x8000> m_prgRom;	// 32K
	std::array<u8, 0x8000> m_chrrom;	// 32K
};
