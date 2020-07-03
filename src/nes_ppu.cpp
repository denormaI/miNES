#include "nes_ppu.h"
#include "nes_cpu.h"
//#include "nes_apu.h"
#include "nes_cart.h"
#include "mines_sys.h"


std::array<unsigned char, 64 * 3> nes_ppu::sm_rgb_palette = 
{
	0x74,0x74,0x74, 0x24,0x18,0x8c, 0x00,0x00,0xa8, 0x44,0x00,0x9c,
	0x8c,0x00,0x74, 0xa8,0x00,0x10, 0xa4,0x00,0x00, 0x7c,0x08,0x00,
	0x40,0x2c,0x00, 0x00,0x44,0x00, 0x00,0x50,0x00, 0x00,0x3c,0x14,
	0x18,0x3c,0x5c, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00,
	0xbc,0xbc,0xbc, 0x00,0x70,0xec, 0x20,0x38,0xec, 0x80,0x00,0xf0,
	0xbc,0x00,0xbc, 0xe4,0x00,0x58, 0xd8,0x28,0x00, 0xc8,0x4c,0x0c,
	0x88,0x70,0x00, 0x00,0x94,0x00, 0x00,0xa8,0x00, 0x00,0x90,0x38,
	0x00,0x80,0x88, 0x00,0x00,0x00, 0x00,0x00,0x00, 0x00,0x00,0x00,
	0xfc,0xfc,0xfc, 0x3c,0xbc,0xfc, 0x5c,0x94,0xfc, 0x40,0x88,0xfc,
	0xf4,0x78,0xfc, 0xfc,0x74,0xb4, 0xfc,0x74,0x60, 0xfc,0x98,0x38,
	0xf0,0xbc,0x3c, 0x80,0xd0,0x10, 0x4c,0xd7,0x48, 0x58,0xf8,0x98,
	0x00,0xe8,0xd8, 0x78,0x78,0x78, 0x00,0x00,0x00, 0x00,0x00,0x00,
	0xfc,0xfc,0xfc, 0xa8,0xe4,0xfc, 0xc4,0xd4,0xfc, 0xd4,0xc8,0xfc,
	0xfc,0xc4,0xfc, 0xfc,0xc4,0xd8, 0xfc,0xbc,0xb0, 0xfc,0xd8,0xa8,
	0xfc,0xe4,0xa0, 0xe0,0xfc,0xa0, 0xa8,0xf0,0xbc, 0xb0,0xfc,0xcc,
	0x9c,0xfc,0xf0, 0xc4,0xc4,0xc4, 0x00,0x00,0x00, 0x00,0x00,0x00
};

nes_ppu::nes_ppu(mines_sys* sys) :
		m_sys(sys)
{
	reset();
}

void nes_ppu::attach_devices(nes_device_map_t device_map)
{
	m_cpu = std::dynamic_pointer_cast<nes_cpu>(device_map["cpu"]);
	if (!m_cpu) throw mines_exception("nes_ppu: Failed to get nes_cpu device.");

	m_cart = std::dynamic_pointer_cast<nes_cart>(device_map["cart"]);
	if (!m_cart) throw mines_exception("nes_ppu: Failed to get nes_cart device.");

	//m_apu = std::dynamic_pointer_cast<nes_apu>(device_map["apu"]);
	//if (!m_apu) throw mines_exception("nes_ppu: Failed to get nes_apu /device.");
}

void nes_ppu::register_frame_end_handler(const frame_end_func& handler)
{
	if (handler != nullptr)
		m_frame_end_handler = handler;
}

void nes_ppu::reset()
{
	m_spr_height = 8;
	m_scanline = PRE;
	m_addr_latch = 0;
	m_ctrl.r = m_mask.r = m_stat.r = 0;
	m_dot = 0;
	m_frameodd = 0;
}

u8 nes_ppu::read(u16 address) 
{
	u8 ret = 0;
	u8 reg = address & 7;

	switch (reg) 
	{
		case 2:
			ret = m_stat.r;
			m_stat.vblank = 0;
			m_addr_latch = 0;
			break;

		case 4:
			ret = n_oam_mem[m_oamaddr];
			break;

		case 7:
			/*
				When reading VRAM in the range 0-$3EFF this read will return
				the contents of an internal read buffer. This internal buffer
				is updated only after reading PPUDATA.
			*/
			if (m_addr.addr <= 0x3eff) 
			{
				ret = m_read_buffer;
				m_read_buffer = rmem(m_addr.addr);
			}
			else 
			{
				ret = m_read_buffer = rmem(m_addr.addr);
			}
			m_addr.addr += m_ctrl.inc ? 32 : 1;
			break;
	}

	return ret;
}

void nes_ppu::write(u16 address, u8 d) 
{
	int reg = address & 7;
	switch (reg) 
	{
		case 0:
			// 2000 write: t:0000110000000000=d:00000011
			m_ctrl.r = d;
			m_taddr.nt = d & 3;

			m_spr_height = m_ctrl.spr_size ? 16 : 8;
			break;

		case 1:
			m_mask.r = d;
			break;

		case 3:
			m_oamaddr = d;
			break;

		case 4:
			n_oam_mem[m_oamaddr++] = d;
			break;

		case 5:
			if (!m_addr_latch) 
			{
				/*  2005 first write:
				 *		t:0000000000011111=d:11111000
				 *		x=d:00000111 */
				m_taddr.cx = d >> 3;
				m_fx = d & 7;
			}
			else 
			{
				/*	2005 second write:
				 *		t:0000001111100000=d:11111000
				 *		t:0111000000000000=d:00000111 */
				m_taddr.cy = d >> 3;
				m_taddr.fy = d & 7;
			}
			m_addr_latch = !m_addr_latch;
			break;

		case 6:
			if (!m_addr_latch) 
			{
				/*	2006 first write:
				 *		t:0011111100000000=d:00111111
				 *		t:1100000000000000=0 */
				m_taddr.h = d & 0x3f;
			}
			else 
			{
				/*	2006 second write:
				 *		t:0000000011111111=d:11111111
				 *		v=t */
				m_taddr.l = d;
				m_addr.r = m_taddr.r;
			}
			m_addr_latch = !m_addr_latch;
			break;
		
		case 7:
			wmem(m_addr.addr, d);
			m_addr.addr += m_ctrl.inc ? 32 : 1;
			break;
	}
	m_stat.bus = d & 0x1f;
}

u16 nes_ppu::mirror_nt_addr(u16 addr) 
{
	switch (m_cart->mirroring()) 
	{
	// horizontal
	case ppu_types::NT_MIRROR_H:
		return ((addr / 2) & 0x400) + (addr % 0x400);

	// vertical
	case ppu_types::NT_MIRROR_V:
		return addr % 0x800;

	// single screen. all mirrors of table $2000
	case ppu_types::NT_MIRROR_SS_2000:
		return addr & 0x3ff;

	// single screen. all mirrors of table $2400
	case ppu_types::NT_MIRROR_SS_2400:
		return (addr & 0x3ff) + 0x400;

	default:
		return addr - 0x2000;
	}
}

u8 nes_ppu::rmem(u16 addr) 
{
	if (addr < 0x2000) // CHRROM or RAM
	{
		return m_cart->read_chr(addr);
	} 
	else if (addr <= 0x3eff) // Nametables
	{
		return m_nt_ram[mirror_nt_addr(addr)];
	} 
	else if (addr <= 0x3fff) // Palettes
	{
		if ((addr & 0x13) == 0x10) 
			addr &= ~0x10;
		return m_palette_ram[addr & 0x1f];
	}
}

void nes_ppu::wmem(u16 addr, u8 d) 
{
	if (addr < 0x2000) // CHRROM or RAM
	{
		m_cart->write_chr(addr, d);
	} 
	else if (addr <= 0x3eff) // Nametables
	{
		m_nt_ram[mirror_nt_addr(addr)] = d;
	} 
	else if (addr <= 0x3fff) // Palettes
	{
		if ((addr & 0x13) == 0x10) 
			addr &= ~0x10;
		m_palette_ram[addr & 0x1f] = d;
	}
}

#include <iostream>
int line = 0;

// Steps the PPU n cycles.
void nes_ppu::step(const int n)
{
    if (line != m_scanline)
    {
//        line = m_scanline;
//        std::cout << "line " << m_scanline << std::endl;
    }

	for (int i = 0; i < n; i++)
	{
		static u16 addr = 0;

		if (m_scanline < VISIBLE_LINES || m_scanline == PRE)
		{
			switch (m_dot)
			{
				case 257: 
					clear_oam();
					evalulate_sprites();
					load_sprites();
					break;
			}

			/*
				The first two tiles are prefetched at the end of the previous visible line 
				and pre-render line. At dot 257, after the visible dots, the horizontal bits 
				of the vram address are set to the start of the line. After this, on the 
				same line between dots 321 and 336, the data for two tiles is fetched ready 
				to be drawn at the start of the next line. 
				
				The shift registers are usually reloaded on dot 1 of the 8 dot long tile data 
				fetch process, that is, when dot % 8 equals 1, however, the first tile's data 
				is already loaded in to the shift registers from the previous line, so don't 
				reload shift registers on dot 1. Once the first tile is drawn the shift 
				reloading process continues at dot 9 with the new data fetched during the first 
				8 dots.
			*/
			if ((m_dot >= 2 && m_dot <= 255) || (m_dot >= 322 && m_dot <= 337))
			{
				vram_fetch_dot();

				switch (m_dot % 8)
				{
					/*
						Nametable byte fetch.
					*/
					case 1:
						addr = nt_address(); 
						shift_reload();
						break; 
					case 2:
						m_nt = rmem(addr);
						break;

					/*
						Attribute byte fetch.
					*/
					case 3:
						addr  = at_address(); 
						break;
					case 4: 
						m_at = rmem(addr);  
						if (m_addr.cy & 2) 
							m_at >>= 4;
						if (m_addr.cx & 2) 
							m_at >>= 2; 
						break;

					/*
						Background byte fetch (low bits).
					*/
					case 5:
						addr = bg_address(); 
						break;
					case 6:  
						m_bg_l = rmem(addr);
						break;

					/*
						Background byte fetch (high bits).
					*/
					case 7:
						addr += 8;
						break;
					case 0:
						m_bg_h = rmem(addr); 
						h_increment(); 
						break;													
				}
			} 
			else if (m_dot == 1)
			{
				// No shift reloading.
				addr = nt_address();

				if (m_scanline == PRE)
					m_stat.spr_ovf = m_stat.spr0hit = m_stat.vblank = 0;
			}
			else if (m_dot == 256)
			{
				vram_fetch_dot();
				m_bg_h = rmem(addr); 
				v_increment();
			}
			else if (m_dot == 257)
			{
				vram_fetch_dot();
				shift_reload();
				h_update();
			}
			else if (m_dot == 321 || m_dot == 339)
			{
				addr = nt_address();
			}
			else if (m_dot == 338 || m_dot == 340)
			{
				m_nt = rmem(addr); 
			}	
				
			if (m_scanline == PRE)
			{
				if (m_dot >= 280 && m_dot <= 304)
				{
					v_update();
				}

				if (m_dot >= SCANLINE_DOTS-1)
				{
					if (m_frame_end_handler != nullptr)
						m_frame_end_handler(m_display_bitmap);

					m_dot = (m_frameodd ? 0 : -1);
					m_scanline = 0;
					m_frameodd ^= 1;
				}
			}
			else
			{
				if (m_dot >= SCANLINE_DOTS-1)
				{
					m_scanline++;
					m_dot = -1;
				}
			}
		}
		else if (m_scanline == POST)
		{
			if (m_dot >= SCANLINE_DOTS-1)
			{
				m_scanline++;
				m_dot = -1;
			}
		}
		else if (m_scanline > POST && m_scanline < PRE) // Vblank.
		{
			if (m_dot == 1 && m_scanline == 241)
			{
				m_stat.vblank = 1;

				if (m_ctrl.nmi)
					m_cpu->execute_nmi();
			}

			if (m_dot >= SCANLINE_DOTS-1)
			{
				m_scanline++;
				m_dot = -1;
			}
		}

		m_dot++;
	}
}

/*
	Retrieve an entry from the palette as an index in to the RGB palette.
	Format for indices:
	43210
	|||||
	|||++- Pixel value from tile data
	|++--- Palette number from attribute table or OAM
	+----- Background/Sprite select
*/
int nes_ppu::palette_entry_lookup(u8 index)
{
	if ((index & 3) == 0)
		return (rmem(0x3f00) & 63) * 3;
	else
		return (rmem((0x3f00 | index)) & 63) * 3;
}

void nes_ppu::vram_fetch_dot()
{
	int x = m_dot - 2;
	int y = m_scanline;

	auto bit_n = [](u16 d, int n) { return (d >> n) & 1; };

	u8 obj_palette = 0, bg_palette = 0;
	if (m_scanline < 240 && x >= 0 && x < 256)
	{
		if (m_mask.bg && !(!m_mask.left_bg && x < 8))
		{
			bg_palette = (bit_n(m_bg_shift[1], 15 - m_fx) << 1) | bit_n(m_bg_shift[0], 15 - m_fx);

			if (bg_palette)
				bg_palette |= ((bit_n(m_at_shift[1], 7 - m_fx) << 1) | bit_n(m_at_shift[0], 7 - m_fx)) << 2;
		}

		bool obj_priority = 0;
		if (m_mask.spr && !(!m_mask.left_spr && x < 8))
		{
			for (int i = 7; i >= 0; i--)
			{
				if (m_oam[i].index == 64)
					continue;

				u32 sprx = x - m_oam[i].x;
				if (sprx >= 8)
					continue;

				// Horizontal flip.
				if (m_oam[i].attr & 0x40)
					sprx ^= 7;

				u8 spr_palette = (bit_n(m_oam[i].data_h, 7 - sprx) << 1) | bit_n(m_oam[i].data_l, 7 - sprx);

				// Transparent pixel.
				if (spr_palette == 0)
					continue;

				if (m_oam[i].index == 0 && bg_palette && x != 255)
					m_stat.spr0hit = true;

				spr_palette |= (m_oam[i].attr & 3) << 2;
				obj_palette = spr_palette + 16;
				obj_priority = m_oam[i].attr & 0x20;
			}
		}

		int rgb_offset;

		// Evaluate priority
		if (obj_palette && (!bg_palette || !obj_priority))
			rgb_offset = palette_entry_lookup(obj_palette);
		else
			rgb_offset = palette_entry_lookup(bg_palette);

		// Offset of the current pixel being drawn in to the display bitmap.
		int colour_offset = (VISIBLE_DOTS * y * 4) + (x * 4);

		m_display_bitmap[colour_offset + 0] = sm_rgb_palette[rgb_offset + 0];
		m_display_bitmap[colour_offset + 1] = sm_rgb_palette[rgb_offset + 1];
		m_display_bitmap[colour_offset + 2] = sm_rgb_palette[rgb_offset + 2];
		m_display_bitmap[colour_offset + 3] = 255;
	}

	// Shift background registers.
	m_bg_shift[0] <<= 1;
	m_bg_shift[1] <<= 1;
	m_at_shift[0] = (m_at_shift[0] << 1) | m_at_latch[0];
	m_at_shift[1] = (m_at_shift[1] << 1) | m_at_latch[1];
}

bool nes_ppu::rendering()
{
	return m_mask.bg || m_mask.spr;
}

u16 nes_ppu::nt_address()
{
	return 0x2000 | (m_addr.r & 0xfff);
}

u16 nes_ppu::at_address()
{
	return 0x23c0 | (m_addr.nt << 10) | ((m_addr.cy / 4) << 3) | (m_addr.cx / 4);
}

u16 nes_ppu::bg_address()
{
	return (m_ctrl.bg_tbl * 0x1000) + (m_nt * 16) + m_addr.fy;
}

void nes_ppu::h_increment()
{
	if (!rendering())
		return;

	if (m_addr.cx == 31)
		m_addr.r ^= 0x41f;
	else
		m_addr.cx++;
}

void nes_ppu::v_increment()
{
	if (!rendering())
		return;
	if (m_addr.fy < 7)
		m_addr.fy++;
	else
	{
		m_addr.fy = 0;
		if (m_addr.cy == 31)
			m_addr.cy = 0;
		else if (m_addr.cy == 29)
		{
			m_addr.cy = 0;
			m_addr.nt ^= 2;
		}
		else
			m_addr.cy++;
	}
}

void nes_ppu::h_update()
{
	if (!rendering())
		return;

	m_addr.r = (m_addr.r & ~0x041f) | (m_taddr.r & 0x041f);
}

void nes_ppu::v_update()
{
	if (!rendering())
		return;

	m_addr.r = (m_addr.r & ~0x7be0) | (m_taddr.r & 0x7be0);
}

void nes_ppu::shift_reload()
{
	m_bg_shift[0] = (m_bg_shift[0] & 0xff00) | m_bg_l;
	m_bg_shift[1] = (m_bg_shift[1] & 0xff00) | m_bg_h;

	m_at_latch[0] = (m_at & 1);
	m_at_latch[1] = (m_at & 2);
}

void nes_ppu::clear_oam()
{
	for (auto& sec_oam : m_sec_oam)
	{
		sec_oam.index = 64;
		sec_oam.y = 0xff;
		sec_oam.tile = 0xff;
		sec_oam.attr = 0xff;
		sec_oam.x = 0xff;
		sec_oam.data_l = 0;
		sec_oam.data_h = 0;
	}
}

void nes_ppu::evalulate_sprites()
{
	int in_range_n = 0;
	for (int i = 0; i < 64; i++)
	{
		if (m_scanline == PRE)
			continue;

		int line = m_scanline - n_oam_mem[i * 4];

		if (line >= 0 && line < m_spr_height)
		{
			if (in_range_n < 8)
			{
				m_sec_oam[in_range_n].index = i;
				m_sec_oam[in_range_n].y = n_oam_mem[i * 4 + 0];
				m_sec_oam[in_range_n].tile = n_oam_mem[i * 4 + 1];
				m_sec_oam[in_range_n].attr = n_oam_mem[i * 4 + 2];
				m_sec_oam[in_range_n].x = n_oam_mem[i * 4 + 3];
			}
			else
			{
				m_stat.spr_ovf = true;
				return;
			}
			in_range_n++;
		}
	}
}

void nes_ppu::load_sprites()
{
	for (int i = 0; i < 8; i++)
	{
		m_oam[i] = m_sec_oam[i];

		u32 line = (m_scanline - m_oam[i].y) % m_spr_height;

		// Vertical flip.
		if (m_oam[i].attr & 0x80)
			line ^= m_spr_height - 1;

		u16 addr;
		switch (m_spr_height)
		{
		case 16: addr = ((m_oam[i].tile & 1) * 0x1000) + ((m_oam[i].tile & ~1) * 16); break;
		case 8: addr = (m_ctrl.spr_tbl * 0x1000) + (m_oam[i].tile * 16); break;
		default: break;
		}

		// Select the second tile if on 8x16.
		addr += line + (line & 8);

		m_oam[i].data_l = rmem(addr + 0);
		m_oam[i].data_h = rmem(addr + 8);
	}
}
