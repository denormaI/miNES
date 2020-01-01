#pragma once

#include <array>
#include <functional>
#include <memory>

#include "types.h"
#include "nes_device.h"

using namespace ppu_types;

class mines_sys;
class nes_cpu;
class nes_apu;
class nes_cart;

class nes_ppu : public nes_device 
{
public:
	nes_ppu(mines_sys*);
	~nes_ppu() {}

	typedef std::function<void(const display_bitmap_t& bitmap)> frame_end_func;

	/*
		Callback mechanism for frame updates. The handler is called by nes_ppu once 
		per frame on the last dot of the pre-render line.
	*/
	void register_frame_end_handler(const frame_end_func& handler);

	virtual void attach_devices(nes_device_map_t device_map);

	void reset();

	u8 read(u16 address);
	void write(u16 address, u8 d);

	void step(const int n = 1);

private:
	// PPUCTRL ($2000)
	union ctrl_reg
	{
		struct
		{
			unsigned nt : 2;		// Base nametable address (0 = $2000; 1 = $2400; 2 = $2800; 3 = $2C00).
			unsigned inc : 1;		// VRAM address increment per CPU read/write (0: add 1, 1: add 32)
			unsigned spr_tbl : 1;	// Sprite pattern table address for 8x8 sprites (0: $0000; 1: $1000; ignored in 8x16 mode).
			unsigned bg_tbl : 1;		// Background pattern table address (0: $0000; 1: $1000).
			unsigned spr_size : 1;	// Sprite size (0: 8x8 pixels, 1: 8x16 pixels).
			unsigned slave : 1;		// PPU master/slave select (0: read backdrop from EXT pins, 1: output color on EXT pins).
			unsigned nmi : 1;		// Generate an NMI at the start of Vblank.
		};
		u8 r;
	};

	// PPUMASK ($2001)
	union mask_reg
	{
		struct
		{
			unsigned greyscale : 1;
			unsigned left_bg : 1;		// Show background in leftmost 8 pixels of screen.
			unsigned left_spr : 1;		// Show sprites in leftmost 8 pixels of screen.
			unsigned bg : 1;			// Show background.
			unsigned spr : 1;			// Show sprites.
			unsigned red : 1;			// Emphasize red.
			unsigned green : 1;			// Emphasize green.
			unsigned blue : 1;			// Emphasize blue.
		};
		u8 r;
	};

	// PPUSTATUS ($2002)
	union status_reg
	{
		struct
		{
			unsigned bus : 5;		// Least significant bits previously written into a PPU register.
			unsigned spr_ovf : 1;	// Sprite overflow.
			unsigned spr0hit : 1;	// Sprite 0 Hit.
			unsigned vblank : 1;	// In vertical blank.
		};
		u8 r;
	};

	union vram_addr
	{
		struct
		{
			unsigned cx : 5;  // Coarse X.
			unsigned cy : 5;  // Coarse Y.
			unsigned nt : 2;  // Nametable.
			unsigned fy : 3;  // Fine Y.
		};
		struct
		{
			unsigned l : 8;
			unsigned h : 7;
		};
		unsigned addr : 14;
		unsigned r : 15;
	};

	// Sprite buffer
	struct sprite_obj
	{
		u8 index;     // Index in OAM.
		u8 x;		// X position.
		u8 y;      // Y position.
		u8 tile;   // Tile index.
		u8 attr;   // Attributes.
		u8 data_l;  // Tile data (low).
		u8 data_h;  // Tile data (high).
	};

	u8 rmem(u16 addr);
	void wmem(u16 addr, u8 d);
	u16 mirror_nt_addr(u16 addr);

	int palette_entry_lookup(u8 index);

	void vram_fetch_dot();
	bool rendering();
	u16 nt_address();
	u16 at_address();
	u16 bg_address();
	void h_increment();
	void v_increment();
	void h_update();
	void v_update();
	void shift_reload();

	void clear_oam();
	void evalulate_sprites();
	void load_sprites();

	// Memory mapped registers.
	ctrl_reg m_ctrl;
	mask_reg m_mask;
	status_reg m_stat;
	u8 m_oamaddr;
	u8 m_oamdata;
	u8 m_scroll;
	vram_addr m_addr;
	u8 m_data;
	u8 m_oamdma;

	// Internal registers.
	vram_addr m_taddr;
	u8 m_fx;
	u8 m_read_buffer;
	int m_scanline;
	u32 m_addr_latch;

	int m_dot;
	int m_frameodd;

	// Background latches
	u8 m_nt, m_at, m_bg_l, m_bg_h;

	// Background shift registers
	u8 m_at_shift[2];
	u16 m_bg_shift[2];
	bool m_at_latch[2];

	int m_spr_height;

	std::array<u8, 256> n_oam_mem;
	std::array<sprite_obj, 8> m_oam;
	std::array<sprite_obj, 8> m_sec_oam;
	std::array<u8, 0x20> m_palette_ram;
	std::array<u8, 0x800> m_nt_ram;

	std::shared_ptr<nes_cpu> m_cpu;
	std::shared_ptr<nes_apu> m_apu;
	std::shared_ptr<nes_cart> m_cart;
	mines_sys* m_sys;

	frame_end_func m_frame_end_handler;
	display_bitmap_t m_display_bitmap;

	static std::array<unsigned char, 64 * 3> sm_rgb_palette;	
};
