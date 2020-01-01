#pragma once

#include <array>

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef char s8;
typedef short s16;
typedef int s32;
typedef float f32;
typedef double f64;
typedef long long s64;

namespace ppu_types
{
	static const int NT_MIRROR_H = 0;	// horizontal
	static const int NT_MIRROR_V = 1;	// vertical
	static const int NT_MIRROR_SS_2000 = 2;	// single screen; table at $2000
	static const int NT_MIRROR_SS_2400 = 3;	// single screen; table at $2400

	static const int VISIBLE_DOTS = 256;
	static const int VISIBLE_LINES = 240;
	static const int POST = 240;
	static const int PRE = 261;

	static const int SCANLINE_DOTS = 341;

	static const int TILES_PER_ROW = 32;
	static const int TILES_PER_COLUMN = 30;

	typedef std::array<unsigned char, VISIBLE_DOTS * VISIBLE_LINES * 4> display_bitmap_t;
}
