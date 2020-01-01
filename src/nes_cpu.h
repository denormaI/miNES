#pragma once

#include <array>
#include <vector>
#include <locale>
#include <string>
#include <memory>

#include "types.h"
#include "nes_device.h"
#include "nes_ppu.h"

class mines_sys;
class nes_ppu;
class nes_cart;
class nes_controllers;
class nes_apu;

class nes_cpu : public nes_device
{
public:
	nes_cpu(mines_sys*);
	~nes_cpu() {}

	void reset();
	void execute_nmi();

	void step_instruction(const int n = 1);
	void step(const int n = 1);
	void stall(const int n);

	virtual void attach_devices(nes_device_map_t device_map);

	u8 read(u16 address);
	void write(u16 address, u8 d);

private:
	struct cpu_register
	{
		u8 a, x, y, sp;
		u16 pc;

	private:
		struct
		{
			u8 n;	// negative
			u8 o;	// overflow
			u8 r;	// constant/reserved
			u8 b;	// break
			u8 d;	// decimal
			u8 i;	// interrupt
			u8 z;	// zero
			u8 c;	// carry
		} stat;

	public:
		void setn(u8 v) { stat.n = (v ? 1 : 0); }
		void seto(u8 v) { stat.o = (v ? 1 : 0); }
		void setr(u8 v) { stat.r = (v ? 1 : 0); }
		void setb(u8 v) { stat.b = (v ? 1 : 0); }
		void setd(u8 v) { stat.d = (v ? 1 : 0); }
		void seti(u8 v) { stat.i = (v ? 1 : 0); }
		void setz(u8 v) { stat.z = (v ? 1 : 0); }
		void setc(u8 v) { stat.c = (v ? 1 : 0); }

		u8 issetn() { return stat.n; }
		u8 isseto() { return stat.o; }
		u8 issetr() { return stat.r; }
		u8 issetb() { return stat.b; }
		u8 issetd() { return stat.d; }
		u8 isseti() { return stat.i; }
		u8 issetz() { return stat.z; }
		u8 issetc() { return stat.c; }

		u8 pack_stat()
		{
			return (
				(stat.c) |
				(stat.z << 1) |
				(stat.i << 2) |
				(stat.d << 3) |
				(stat.b << 4) |
				(stat.r << 5) |
				(stat.o << 6) |
				(stat.n << 7));
		}

		void unpack_stat(u8 p)
		{
			stat.c = (p & FLG_CARRY ? 1 : 0);
			stat.z = (p & FLG_ZERO ? 1 : 0);
			stat.i = (p & FLG_INTERRUPT ? 1 : 0);
			stat.d = (p & FLG_DECIMAL ? 1 : 0);
			stat.b = (p & FLG_BREAK ? 1 : 0);
			stat.r = (p & FLG_CONSTANT ? 1 : 0);
			stat.o = (p & FLG_OVERFLOW ? 1 : 0);
			stat.n = (p & FLG_NEGATIVE ? 1 : 0);
		}
	};

	void execute_op(const u8 op);

	static const u8 FLG_NEGATIVE = 0x80;
	static const u8 FLG_OVERFLOW = 0x40;
	static const u8 FLG_CONSTANT = 0x20;
	static const u8 FLG_BREAK = 0x10;
	static const u8 FLG_DECIMAL = 0x08;
	static const u8 FLG_INTERRUPT = 0x04;
	static const u8 FLG_ZERO = 0x02;
	static const u8 FLG_CARRY = 0x01;

	u16 read16(u16 a);
	
	inline void push(u8 b);
	inline void push16(u16 w);
	inline u8 pop();
	inline u16 pop16();
	
	inline void opbit(u8 m);
	inline void oplda(u8 m);
	inline void opldx(u8 m);
	inline void opldy(u8 m);	
	inline void opadc(u8 m);
	inline void opsbc(u8 m);
	inline void opand(u8 m);
	inline void opeor(u8 m);
	inline void opora(u8 m);
	inline void opcmp(u8 m);
	inline void opcpx(u8 m);
	inline void opcpy(u8 m);
	inline void opsta(u16 a);
	inline void opstx(u16 a);
	inline void opsty(u16 a);
	inline void opinc(u16 a);
	inline void opdec(u16 a);
	inline void opasl(u16 a);
	inline void oplsr(u16 a);
	inline void oprol(u16 a);
	inline void opror(u16 a);
	
	/*
	 * Addressing modes.
	 */

	// nn
	inline u8 zeropage()
	{ 		
		return read(m_regs.pc); 
	}

	// nn+X
	inline u8 zeropagex()
	{
		return read(m_regs.pc) + m_regs.x; 
	}

	// nn+Y
	inline u8 zeropagey() 
	{ 
		return read(m_regs.pc) + m_regs.y; 
	}

	// nnnn
	inline u16 absolute()
	{ 
		return read16(m_regs.pc); 
	}

	// nnnn+X
	inline u16 absolutex()
	{ 
		return read16(m_regs.pc) + m_regs.x; 
	}

	// nnnn+Y
	inline u16 absolutey()
	{ 
		return read16(m_regs.pc) + m_regs.y; 
	}

	// u16[nn+X]	indexed indirect
	inline u16 idxind()
	{
			u8 al = read(m_regs.pc) + m_regs.x;
			if (al == 0xff)	// page cross
			{
				m_total_instr_cycles++;
				return (read(0) << 8) | al;
			}
			else
				return read16(al);		
	}	
	
	// u16[nn]+Y	indirect indexed
	inline u16 indidx()
	{
			u8 al = read(m_regs.pc);
			if (al == 0xff)	// page cross
			{
				m_total_instr_cycles++;
				return ((read(0) << 8) | al) + m_regs.y;
			}
			else
				return ((read16(al) + m_regs.y));
	}	

	int m_total_instr_cycles;		// Number of cycles to execute for this instruction (may be modified by instruction).
	int m_exec_cycle;			// Cycle of this instruction to complete the instruction on (typically the last).
	int m_stall_cycles;			// Number of cycles to delay instruction execution for.
	int m_instr_cycles;			// The current cycle of this instruction.
	u8 m_op;
	u64 m_cycle_count;

	static const std::array<u8,0x100> m_cycle_table;
	
	std::array<u8,0x800> m_ram;		// 2K RAM 		
	std::array<u8,0x2000> m_sram;	// 8K battery backed prg ram
	
	cpu_register m_regs;

	std::shared_ptr<nes_ppu> m_ppu;
	std::shared_ptr<nes_apu> m_apu;
	std::shared_ptr<nes_cart> m_cart;
	std::shared_ptr<nes_controllers> m_controllers;
	mines_sys* m_sys;
};
