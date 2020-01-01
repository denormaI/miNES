#include "ines_mapper0.h"
#include "mines_sys.h"

ines_mapper0::ines_mapper0(mines_sys* sys, const std::vector<u8>& filebuf) : 
		ines_mapper(sys, filebuf)
{
	switch (filebuf[4])
	{
		case 1: // 16K PRG-ROM
			std::memcpy(&m_prgrom[0], &filebuf[m_prg_start], PRGROM_BANK_SZ);
			std::memcpy(&m_prgrom[PRGROM_BANK_SZ], &filebuf[m_prg_start], PRGROM_BANK_SZ);
			m_chr_start = m_prg_start + PRGROM_BANK_SZ;
			break;

		case 2: // 32K PRG-ROM
			std::memcpy(&m_prgrom[0], &filebuf[m_prg_start], 2 * PRGROM_BANK_SZ);
			m_chr_start = m_prg_start + PRGROM_BANK_SZ * 2;
			break;

		default:
			throw mines_exception("Unexpected PRG-ROM size.");		
	}

	// If the game has CHR-ROM, it gets loaded into the pattern table
	if (filebuf[5] == 1)
		std::memcpy(&m_chrrom[0], &filebuf[m_chr_start], CHRROM_BANK_SZ);
	else if (filebuf[5] > 1)
		throw mines_exception("NROM doesn't support more than 8KB of CHR-ROM");
}

u8 ines_mapper0::read_chr(u16 addr)
{
	return m_chrrom[addr & 0x1fff];
}

void ines_mapper0::write(u16, u8) { }

u8 ines_mapper0::read(u16 address) 
{
	return m_prgrom[address & 0x7fff];
}
