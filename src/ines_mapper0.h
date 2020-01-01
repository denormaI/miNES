#pragma once

#include "ines_mapper.h"

/*
NROM
*/
class ines_mapper0 : public ines_mapper
{
public:
	ines_mapper0(mines_sys* sys, const std::vector<u8>& filebuf);

	virtual u8 read(u16 address);
	virtual void write(u16 address, u8 data);
	virtual u8 read_chr(u16 address);
	virtual void write_chr(u16 /*address*/, u8 /*data*/) {}

private:
	std::array<u8, 0x8000> m_prgrom;	// 32K
	std::array<u8, 0x2000> m_chrrom;	// 8K
};
