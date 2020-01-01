#pragma once

#include "ines_mapper.h"

/*
MMC1

NOT WORKING

*/
class ines_mapper1 : public ines_mapper
{
public:
	ines_mapper1(mines_sys* sys, const std::vector<u8>& filebuf);

	virtual u8 read(u16 address);
	virtual void write(u16 address, u8 data);
	virtual u8 read_chr(u16 address);
	virtual void write_chr(u16 address, u8 data);

private:
	void apply();

	std::vector<std::array<u8, PRGROM_BANK_SZ>> m_prgbanks;
	int m_prgrom0;	// 16K. Either switchable or fixed to the first bank
	int m_prgrom1;	// 16K. Either fixed to the last bank or switchable

	int m_writes;
	u8 m_tmpreg;
	u8 m_regs[4];

	std::array<u8, CHRROM_BANK_SZ> m_chrrom;	// 8K
	std::array<u8, 0x2000> m_prgram;			// 8K
};

