#pragma once

#include "ines_mapper.h"

/*
UxROM
*/
class ines_mapper2 : public ines_mapper
{
public:
	ines_mapper2(mines_sys* sys, const std::vector<u8>& filebuf);

	virtual u8 read(u16 address);
	virtual void write(u16 address, u8 data);
	virtual u8 read_chr(u16 address);
	virtual void write_chr(u16 address, u8 data);

private:

	std::vector<std::array<u8, PRGROM_BANK_SZ>> m_prgbanks;
	int m_prgrom0;	// 16K switchable
	int m_prgrom1;	// 16K fixed

	std::array<u8, CHRROM_BANK_SZ> m_chrrom;	// 8K fixed
};
