#include "ines_mapper3.h"
#include "mines_sys.h"


ines_mapper3::ines_mapper3(mines_sys* sys, const std::vector<u8>& filebuf) : 
		ines_mapper(sys, filebuf),
		m_bankSel(0)
{
	switch (filebuf[4]) 
	{
		case 1: // 16K PRG-ROM
			std::memcpy(&m_prgRom[0], &filebuf[m_prg_start], PRGROM_BANK_SZ);
			std::memcpy(&m_prgRom[PRGROM_BANK_SZ], &filebuf[m_prg_start], PRGROM_BANK_SZ);
			m_chr_start = m_prg_start + PRGROM_BANK_SZ;
			break;

		case 2: // 32K PRG-ROM
			std::memcpy(&m_prgRom[0], &filebuf[m_prg_start], 2 * PRGROM_BANK_SZ);
			m_chr_start = m_prg_start + PRGROM_BANK_SZ * 2;
			break;

		default:
			throw mines_exception("Unexpected PRG-ROM size.");
	}

	//if (filebuf[5] != 4)
	//	throw mines_exception("Unexpected CHR-ROM size.");

	std::memcpy(&m_chrrom[0], &filebuf[m_chr_start], 4 * CHRROM_BANK_SZ);
}

u8 ines_mapper3::read_chr(u16 addr)
{
	return m_chrrom[m_bankSel * CHRROM_BANK_SZ + (addr & 0x1fff)];
}

void ines_mapper3::write(u16 addr, u8 d) 
{
	m_bankSel = d & 3;
}

u8 ines_mapper3::read(u16 addr)
{
	return m_prgRom[addr & 0x7fff];
}

