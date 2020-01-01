#pragma once

#include <cstdint>
#include <cstring>
#include <array>
#include <vector>
#include <memory>
#include <fstream>
#include <iterator>

#include "types.h"

class mines_sys;

class ines_mapper
{
public:
	ines_mapper(mines_sys* sys);
	ines_mapper(mines_sys* sys, const std::vector<u8>& filebuf);

	static const u32 PRGROM_BANK_SZ = 0x4000;	// 16KB PRG-ROM page
	static const u32 CHRROM_BANK_SZ = 0x2000;	// 8KB CHR-ROM page

	virtual ~ines_mapper() {}

	int mirroring() const { return m_mirroring; }

	virtual u8 read(u16 /*address*/) = 0;
	virtual void write(u16 /*address*/, u8 /*data*/) = 0;

	virtual u8 read_chr(u16 /*address*/) = 0;
	virtual void write_chr(u16 /*address*/, u8 /*data*/) = 0;

protected:
	int m_mirroring;
	int m_chr_start;
	int m_prg_start;
	std::vector<u8> m_filebuf;
	mines_sys* m_sys;
};

/*
	Used when no mapper is assigned.
*/
class ines_dummy_mapper : public ines_mapper
{
public:
	ines_dummy_mapper(mines_sys* sys) : 
			ines_mapper(sys) 
	{
	}

	virtual int mirroring() { return 0; }
	virtual u8 read(u16 /*address*/) { return 0; }
	virtual void write(u16 /*address*/, u8 /*data*/) { }
	virtual u8 read_chr(u16 /*address*/) { return 0; }
	virtual void write_chr(u16 /*address*/, u8 /*data*/) {}
};
