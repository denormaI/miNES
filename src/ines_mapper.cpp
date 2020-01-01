#include "ines_mapper.h"


ines_mapper::ines_mapper(mines_sys* sys, const std::vector<u8>& filebuf) : 
        m_filebuf(filebuf), 
        m_sys(sys),
        m_prg_start(16),
		m_mirroring(filebuf[6] & 1)
{
}

ines_mapper::ines_mapper(mines_sys* sys) : 
        m_sys(sys)
{
}