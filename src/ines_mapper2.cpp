#include "ines_mapper2.h"
#include "mines_sys.h"


ines_mapper2::ines_mapper2(mines_sys* sys, const std::vector<u8>& filebuf) :
		ines_mapper(sys, filebuf)
{
	// Read PRG banks.
	int prgbanks = filebuf[4];
	int tmp = m_prg_start;
	for (auto i = 0; i < prgbanks; i++) 
	{
		std::array<u8, PRGROM_BANK_SZ> bank;

		std::memcpy(&bank[0], &filebuf[i * PRGROM_BANK_SZ + m_prg_start], PRGROM_BANK_SZ);
		m_prgbanks.push_back(bank);
		
		tmp += PRGROM_BANK_SZ;
	}

	// Initially 1st and LAST 16K banks are mapped to $8000 and $C000.
	m_prgrom0 = 0;
	m_prgrom1 = m_prgbanks.size()-1;

	// Any CHR-ROM is mapped to the pattern table.
	if (filebuf[5] == 1)
	{
		m_chr_start = tmp;
		std::memcpy(&m_chrrom[0], &filebuf[m_chr_start], CHRROM_BANK_SZ);
	}
}

u8 ines_mapper2::read_chr(u16 addr)
{
	return m_chrrom[addr & 0x1fff];
}

void ines_mapper2::write_chr(u16 addr, u8 d)
{
	m_chrrom[addr & 0x1fff] = d;
}

void ines_mapper2::write(u16 addr, u8 d) 
{
	m_prgrom0 = d;
}

u8 ines_mapper2::read(u16 addr)
{
	// switchable
	if (addr < 0xc000)
	{
		return m_prgbanks[m_prgrom0][addr - 0x8000];
	}
	// fixed
	else
	{
		return m_prgbanks[m_prgrom1][addr - 0xc000];
	}
}
