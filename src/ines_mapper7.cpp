#include "ines_mapper7.h"
#include "mines_sys.h"


ines_mapper7::ines_mapper7(mines_sys* sys, const std::vector<u8>& filebuf) :
		ines_mapper(sys, filebuf)
{
	// Read PRG banks.
	int prgbanks32k = filebuf[4] / 2;
	int tmp = m_prg_start;
	for (auto i = 0; i < prgbanks32k; i++) 
	{
		std::array<u8, PRGROM_BANK_SZ * 2> bank;

		std::memcpy(&bank[0], &filebuf[i * PRGROM_BANK_SZ * 2 + m_prg_start], PRGROM_BANK_SZ * 2);
		m_prgbanks.push_back(bank);
		
		tmp += PRGROM_BANK_SZ * 2;
	}

	m_prg_bank = 0;
	m_mirroring = ppu_types::NT_MIRROR_SS_2000;
}

u8 ines_mapper7::read_chr(u16 addr)
{
	return m_chrram[addr & 0x1fff];
}

void ines_mapper7::write_chr(u16 addr, u8 d)
{
	m_chrram[addr & 0x1fff] = d;
}

void ines_mapper7::write(u16 addr, u8 d) 
{
	m_prg_bank = d & 7;
	m_mirroring = (d & 0x10 ? ppu_types::NT_MIRROR_SS_2400 : ppu_types::NT_MIRROR_SS_2000);
}

u8 ines_mapper7::read(u16 addr)
{
	return m_prgbanks[m_prg_bank][addr - 0x8000];
}
