#pragma once

#include "ines_mapper.h"

/*
AxROM
*/
class ines_mapper7 : public ines_mapper
{
public:
	ines_mapper7(mines_sys* sys, const std::vector<u8>& filebuf);

	virtual u8 read(u16 address);
	virtual void write(u16 address, u8 data);
	virtual u8 read_chr(u16 address);
	virtual void write_chr(u16 address, u8 data);

private:
	std::vector<std::array<u8, PRGROM_BANK_SZ * 2>> m_prgbanks;
	int m_prg_bank;	// 32K switchable

	std::array<u8, CHRROM_BANK_SZ> m_chrram;	// 8K fixed
};
