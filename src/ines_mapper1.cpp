#include "ines_mapper1.h"
#include "mines_sys.h"


ines_mapper1::ines_mapper1(mines_sys* sys, const std::vector<u8>& filebuf) :
		ines_mapper(sys, filebuf),
		m_writes(0)
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

	auto chrsz = filebuf[5];
	if (chrsz)
	{
		throw mines_exception("ines_mapper1: TODO");
	}
}

u8 ines_mapper1::read_chr(u16 addr)
{
	return m_chrrom[addr & 0x1fff];
}

void ines_mapper1::write_chr(u16 addr, u8 d)
{
	m_chrrom[addr & 0x1fff] = d;
}

void ines_mapper1::apply()
{
	if (m_regs[0] & 0x8)
	{
		if (m_regs[0] & 0x4)
		{
			m_prgrom0 = m_regs[3] & 0xf;
			m_prgrom1 = m_prgbanks.size() - 1;
		}
		else
		{
			m_prgrom0 = m_prgbanks.size() - 1;
			m_prgrom1 = m_regs[3] & 0xf;
		}
	}
	else
	{
		throw mines_exception("ines_mapper1 32KB PRG");
	}

	// 4KB CHR
	//if (m_regs[0] & 0x10)
	//{
	//	throw mines_exception("4KB CHR");
	//}
	// 8KB CHR
	//else
	//{

	//}

	//switch (m_regs[0] & 0x3)
	//{
	//case 2: m_mirroring = ppu_types::NT_MIRROR_V; break;
	//case 3: m_mirroring = ppu_types::NT_MIRROR_V; break;
	//}
}

void ines_mapper1::write(u16 addr, u8 d) 
{
	if (addr < 0x8000)
	{
		m_prgram[addr - 0x6000] = d;
	}
	else if (addr & 0x8000)
	{
		/*
			7  bit  0
			---- ----
			Rxxx xxxD
			| |
			| +-Data bit to be shifted into shift register, LSB first
			+ -------- - 1: Reset shift register and write Control with(Control OR $0C),
							locking PRG ROM at $C000 - $FFFF to the last bank.
		*/
		if (d & 0x80)
		{
			m_writes = 0;
			m_tmpreg = 0;
			m_regs[0] |= 0x0c;
			apply();
		}
		else
		{
			m_tmpreg = ((d & 1) << 4) | (m_tmpreg >> 1);

			if (++m_writes == 5)
			{
				m_regs[(addr >> 13) & 3] = m_tmpreg;
				m_writes = 0;
				m_tmpreg = 0;
				apply();
			}
		}
	}
}

u8 ines_mapper1::read(u16 addr)
{
	if (addr < 0xc000)
	{
		return m_prgbanks[m_prgrom0][addr - 0x8000];
	}
	else
	{
		return m_prgbanks[m_prgrom1][addr - 0xc000];
	}
}
