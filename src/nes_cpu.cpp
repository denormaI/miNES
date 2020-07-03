#include "nes_cpu.h"
#include "nes_ppu.h"
//#include "nes_apu.h"
#include "nes_cart.h"
#include "nes_controllers.h"
#include "mines_sys.h"


const std::array<u8,0x100> nes_cpu::m_cycle_table =
{
/*		0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f */
/*0*/ 	7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, 
/*1*/ 	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, 
/*2*/ 	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
/*3*/ 	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/*4*/ 	6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
/*5*/ 	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/*6*/ 	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
/*7*/ 	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/*8*/ 	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
/*9*/ 	2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
/*a*/ 	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
/*b*/ 	2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
/*c*/ 	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
/*d*/ 	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
/*e*/ 	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
/*f*/ 	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};


nes_cpu::nes_cpu(mines_sys* sys) : 
		m_sys(sys)
{
}

void nes_cpu::reset()
{
	m_regs.a = m_regs.x = m_regs.y = 0;
	m_regs.pc = read16(0xfffc);
    m_regs.sp = 0xfd;
    m_regs.unpack_stat(FLG_CONSTANT | FLG_BREAK | FLG_INTERRUPT);

	m_total_instr_cycles = 0;
	m_exec_cycle = 0;
	m_stall_cycles = 0;
	m_cycle_count = 0;
	m_instr_cycles = 0;

	stall(7);
}

// TODO: Should this reset break too?
void nes_cpu::execute_nmi()
{
	push16(m_regs.pc);
	push(m_regs.pack_stat());
	m_regs.seti(1);
	m_regs.pc = read16(0xfffa);

	// Abandon current instruction. 
	// Treat the instruction at the new PC as the next instruction to execute.
	m_instr_cycles = 0;

	stall(7);
}

u8 nes_cpu::read(u16 addr)
{
	switch (addr >> 12)
	{
	// internal ram
	case 0x0: case 0x1:
		return m_ram[addr & 0x7ff];

	// ppu
	case 0x2: case 0x3:
		return m_ppu->read(addr);

	case 0x4: case 0x5:
		if (addr == 0x4016)
		{
			return m_controllers->read_state(0);	// Controller 0
		}
		else if (addr == 0x4017)
		{
			return m_controllers->read_state(1);	// Controller 1
		}
		else
		{
			//return m_apu->read(addr);
		}
		break;

	// cartridge ram
	case 0x6: case 0x7:
		return m_sram[addr & 0x1fff];

	// cartridge prg-rom
	case 0x8: case 0x9: case 0xa: case 0xb: case 0xc: case 0xd: case 0xe: case 0xf:
		return m_cart->read(addr);
	}

	return 0;
}

void nes_cpu::write(u16 addr, u8 d)
{
	switch (addr >> 12)
	{
	case 0x0: case 0x1:
		m_ram[addr & 0x7ff] = d;
		break;

	case 0x2: case 0x3:
		m_ppu->write(addr, d);
		break;

	case 0x4: case 0x5:
		// Upload 256 bytes of data from CPU page $XX00-$XXFF to the internal PPU OAM
		if (addr == 0x4014)
		{
			if (d < 0x20)
			{
				u8* src = m_ram.data() + ((d * 0x100) & 0x7ff);
				for (u16 i = 0; i < 256; i++)
					m_ppu->write(0x2004, *(src + i));
			}
			else
			{
				for (u16 i = 0; i < 256; i++)
					m_ppu->write(0x2004, read(d * 0x100 + i));
			}

			stall(513);
		}
		else if (addr == 0x4016)
		{
			m_controllers->write_strobe(d & 1);
		}
		else
		{
			//m_apu->write(addr, d);
		}
		break;

	// cartridge ram
	case 0x6: case 0x7:
	{
		m_sram[addr & 0x1fff] = d;

		if (addr == 0x6000)
		{
			if (m_sram[1] == 0xde && m_sram[2] == 0xb0 && m_sram[3] == 0x61)
			{
				if (d <= 0x7f)
				{
					//m_sys->log("test $%02x complete\n%s\n", d, (char*)(&m_sram[4]));
				}
			}
		}
	}
	break;

	// cartridge prg-rom
	case 0x8: case 0x9: case 0xa: case 0xb: case 0xc: case 0xd: case 0xe: case 0xf:
		m_cart->write(addr, d);
		break;

	}
}

void nes_cpu::stall(const int n)
{
	m_stall_cycles += n;
}

void nes_cpu::step_instruction(const int n)
{
	for (int i = 0; i < n; i++)
	{	
		while (m_exec_cycle == 0) step();	// step until fetch
		while (m_exec_cycle != 0) step();	// step until execution
	}
}

/*
	Implements a pseudo emulation of the instruction pipeline.

	For each instruction:
		* Fetch on the first cycle.
		* Execute on a later cycle (typically the last).
		* Some instructions may modify the number of cycles for this 
			instruction (e.g. a branch taken).
		* The pipeline is stalled if m_stall_cycles > 0, in which case 
			no instruction fetch or execution takes place. 
*/
void nes_cpu::step(const int n)
{
	for (int i = 0; i < n; i++)
	{
		if (m_stall_cycles <= 0)
		{
			if (m_instr_cycles == 0)
			{
				// First cycle: fetch instruction and init.
				m_op = read(m_regs.pc);
				m_total_instr_cycles = m_cycle_table[m_op];
				m_exec_cycle = m_total_instr_cycles - 1;
			}
			else
			{
				if (m_instr_cycles == m_exec_cycle)
				{
					// Pre-increment PC and execute instruction.
					m_regs.pc++;
					execute_op(m_op);

					// Prevent repeated executions.
					m_exec_cycle = 0;
				}

				if (m_instr_cycles == m_total_instr_cycles - 1)
				{
					// Finished this instruction.
					m_instr_cycles = -1;
				}
			}

			m_instr_cycles++;
		}
		else
		{
			m_stall_cycles--;
		}

		m_cycle_count++;
	}
}

void nes_cpu::execute_op(const u8 op)
{
	switch (op)
	{
	case 0x4c: // JMP nnnn: Jump Absolute. PC=nnnn
		m_regs.pc = absolute();
		break;

	case 0x6c: // JMP (nnnn): Jump Indirect. PC=[nnnn]
	{
		u16 w = read16(m_regs.pc);

		// page boundary cross quirk.
		if ((w & 0xff) == 0xff)
		{
			m_total_instr_cycles++;
			u8 pcl = read(w);
			u8 pch = read((w + 1) - 0x100);
			m_regs.pc = (pch << 8) | pcl;
		}
		else
		{
			m_regs.pc = read16(w);
		}
	}
	break;

	case 0x20: // JSR nnnn: Jump and Save Return address. [S]=PC+2, PC=nnnn
		push16(m_regs.pc + 1);
		m_regs.pc = read16(m_regs.pc);
		break;

	case 0x40: // RTI: Return from BRK/IRQ/NMI. P=[S], PC=[S]
		m_regs.unpack_stat(pop());
		m_regs.pc = pop16();
		break;

	case 0x60: // RTS: Return from Subroutine. PC=[S]+1
		m_regs.pc = pop16() + 1;
		break;

	case 0x10: // BPL disp: Branch on result plus. if N=0 PC=PC+/-nn
		if (!m_regs.issetn())
		{
			m_regs.pc += (s8)read(m_regs.pc);
			m_total_instr_cycles++;
		}
		m_regs.pc++;
		break;

	case 0x30: // BMI disp: Branch on result minus. if N=1 PC=PC+/-nn
		if (m_regs.issetn())
		{
			m_regs.pc += (s8)read(m_regs.pc);
			m_total_instr_cycles++;
		}
		m_regs.pc++;
		break;

	case 0x50: // BVC disp: Branch on overflow clear. if V=0 PC=PC+/-nn
		if (!m_regs.isseto())
		{
			m_regs.pc += (s8)read(m_regs.pc);
			m_total_instr_cycles++;
		}
		m_regs.pc++;
		break;

	case 0x70: // BVS disp: Branch on overflow set. if V=1 PC=PC+/-nn
		if (m_regs.isseto())
		{
			m_regs.pc += (s8)read(m_regs.pc);
			m_total_instr_cycles++;
		}
		m_regs.pc++;
		break;

	case 0x90: // BCC disp: Branch on carry clear. if C=0 PC=PC+/-nn
		if (!m_regs.issetc())
		{
			m_regs.pc += (s8)read(m_regs.pc);
			m_total_instr_cycles++;
		}
		m_regs.pc++;
		break;

	case 0xb0: // BCS disp: Branch on carry set. if C=1 PC=PC+/-nn
		if (m_regs.issetc())
		{
			m_regs.pc += (s8)read(m_regs.pc);
			m_total_instr_cycles++;
		}
		m_regs.pc++;
		break;

	case 0xd0: // BNE disp: Branch on result not zero. if Z=0 PC=PC+/-nn
		if (!m_regs.issetz())
		{
			m_regs.pc += (s8)read(m_regs.pc);
			m_total_instr_cycles++;
		}
		m_regs.pc++;
		break;

	case 0xf0: // BEQ disp: Branch on result zero. if Z=1 PC=PC+/-nn
		if (m_regs.issetz())
		{
			m_regs.pc += (s8)read(m_regs.pc);
			m_total_instr_cycles++;
		}
		m_regs.pc++;
		break;

	case 0xa8: // TAY: Transfer Accumulator to Y
		m_regs.y = m_regs.a;
		m_regs.setn(m_regs.a & 0x80);
		m_regs.setz(!m_regs.a);
		break;

	case 0xaa: // TAX: Transfer Accumulator to X   
		m_regs.x = m_regs.a;
		m_regs.setn(m_regs.a & 0x80);
		m_regs.setz(!m_regs.a);
		break;

	case 0xba: // TSX: Transfer Stack pointer to X 	
		m_regs.x = m_regs.sp;
		m_regs.setn(m_regs.sp & 0x80);
		m_regs.setz(!m_regs.sp);
		break;

	case 0x98: // TYA: Transfer Y to Accumulator  
		m_regs.a = m_regs.y;
		m_regs.setn(m_regs.y & 0x80);
		m_regs.setz(!m_regs.y);
		break;

	case 0x8a: // TXA: Transfer X to Accumulator 
		m_regs.a = m_regs.x;
		m_regs.setn(m_regs.x & 0x80);
		m_regs.setz(!m_regs.x);
		break;

	case 0x9a: // TXS: Transfer X to Stack pointer	
		m_regs.sp = m_regs.x;
		break;

	case 0x48: // PHA: Push accumulator on stack
		push(m_regs.a);
		break;

	case 0x08: // PHP: Push processor status on stack
		push(m_regs.pack_stat() | FLG_BREAK | FLG_CONSTANT);
		break;

	case 0x68: // PLA: Pull accumulator from stack
		m_regs.a = pop();
		m_regs.setn(m_regs.a & 0x80);
		m_regs.setz(!m_regs.a);
		break;

	case 0x28: // PLP: Pull processor status from stack
		m_regs.unpack_stat(pop() | FLG_CONSTANT);
		break;

	case 0xe8: // INX: Increment X
		m_regs.x++;
		m_regs.setn(m_regs.x & 0x80);
		m_regs.setz(!m_regs.x);
		break;

	case 0xc8: // INY: Increment Y
		m_regs.y++;
		m_regs.setn(m_regs.y & 0x80);
		m_regs.setz(!m_regs.y);
		break;

	case 0xca: // DEX: Decrement X	
		m_regs.x--;
		m_regs.setn(m_regs.x & 0x80);
		m_regs.setz(!m_regs.x);
		break;

	case 0x88: // DEY: Decrement Y	
		m_regs.y--;
		m_regs.setn(m_regs.y & 0x80);
		m_regs.setz(!m_regs.y);
		break;

	case 0x0a: // ASL A: Shift Left Accumulator
		m_regs.setc(m_regs.a & 0x80);
		m_regs.a <<= 1;
		m_regs.setn(m_regs.a & 0x80);
		m_regs.setz(!m_regs.a);
		break;

	case 0x4a: // LSR A: Shift Right Accumulator
		m_regs.setc(m_regs.a & 0x01);
		m_regs.a >>= 1;
		m_regs.setn(0);
		m_regs.setz(!m_regs.a);
		break;

	case 0x2a: // ROL A: Rotate Left Accumulator
	{
		u16 t = m_regs.a;
		t <<= 1;
		t |= m_regs.issetc();
		m_regs.setc(t > 0xff);
		t &= 0xff;
		m_regs.setn(t & 0x80);
		m_regs.setz(!t);
		m_regs.a = (u8)t;
	}
	break;

	case 0x6a: // ROR A: Rotate Right Accumulator
	{
		u16 t = m_regs.a;
		t |= (m_regs.issetc() << 8);
		m_regs.setc(t & 0x01);
		t >>= 1;
		t &= 0xff;
		m_regs.setn(t & 0x80);
		m_regs.setz(!t);
		m_regs.a = (u8)t;
	}
	break;

	case 0x00: // BRK
		m_regs.pc++;
		push16(m_regs.pc);
		push(m_regs.pack_stat() | FLG_BREAK | FLG_CONSTANT);
		m_regs.seti(1);
		m_regs.pc = read16(0xfffe);
		break;

	case 0xea: // NOP
		break;

		/* ---------------------------------------------
		Immediate. #nn
		--------------------------------------------- */
	case 0xa9: // LDA
		oplda(read(m_regs.pc));
		m_regs.pc++;
		break;

	case 0xa2: // LDX
		opldx(read(m_regs.pc));
		m_regs.pc++;
		break;

	case 0xa0: // LDY
		opldy(read(m_regs.pc));
		m_regs.pc++;
		break;

	case 0x69: // ADC
		opadc(read(m_regs.pc));
		m_regs.pc++;
		break;

	case 0xe9: // SBC
		opsbc(read(m_regs.pc));
		m_regs.pc++;
		break;

	case 0x29: // AND
		opand(read(m_regs.pc));
		m_regs.pc++;
		break;

	case 0x49: // EOR
		opeor(read(m_regs.pc));
		m_regs.pc++;
		break;

	case 0x09: // ORA
		opora(read(m_regs.pc));
		m_regs.pc++;
		break;

	case 0xc9: // CMP
		opcmp(read(m_regs.pc));
		m_regs.pc++;
		break;

	case 0xe0: // CPX
		opcpx(read(m_regs.pc));
		m_regs.pc++;
		break;

	case 0xc0: // CPY
		opcpy(read(m_regs.pc));
		m_regs.pc++;
		break;

		/* ---------------------------------------------
		Zero page. nn
		--------------------------------------------- */
	case 0xa5: // LDA
		oplda(read(zeropage()));
		m_regs.pc++;
		break;

	case 0xa6: // LDX
		opldx(read(zeropage()));
		m_regs.pc++;
		break;

	case 0xa4: // LDY
		opldy(read(zeropage()));
		m_regs.pc++;
		break;

	case 0x65: // ADC
		opadc(read(zeropage()));
		m_regs.pc++;
		break;

	case 0xe5: // SBC
		opsbc(read(zeropage()));
		m_regs.pc++;
		break;

	case 0x25: // AND
		opand(read(zeropage()));
		m_regs.pc++;
		break;

	case 0x45: // EOR
		opeor(read(zeropage()));
		m_regs.pc++;
		break;

	case 0x05: // ORA
		opora(read(zeropage()));
		m_regs.pc++;
		break;

	case 0xc5: // CMP
		opcmp(read(zeropage()));
		m_regs.pc++;
		break;

	case 0xe4: // CPX
		opcpx(read(zeropage()));
		m_regs.pc++;
		break;

	case 0xc4: // CPY
		opcpy(read(zeropage()));
		m_regs.pc++;
		break;

	case 0x85: // STA
		opsta(zeropage());
		m_regs.pc++;
		break;

	case 0x86: // STX
		opstx(zeropage());
		m_regs.pc++;
		break;

	case 0x84: // STY
		opsty(zeropage());
		m_regs.pc++;
		break;

	case 0x24: // BIT
		opbit(read(zeropage()));
		m_regs.pc++;
		break;

	case 0xe6: // INC
		opinc(zeropage());
		m_regs.pc++;
		break;

	case 0xc6: // DEC
		opdec(zeropage());
		m_regs.pc++;
		break;

	case 0x06: // ASL
		opasl(zeropage());
		m_regs.pc++;
		break;

	case 0x46: // LSR
		oplsr(zeropage());
		m_regs.pc++;
		break;

	case 0x26: // ROL
		oprol(zeropage());
		m_regs.pc++;
		break;

	case 0x66: // ROR
		opror(zeropage());
		m_regs.pc++;
		break;

		/* ---------------------------------------------
		Zero page indexed with X. nn+X
		--------------------------------------------- */
	case 0xb5: // LDA
		oplda(read(zeropagex()));
		m_regs.pc++;
		break;

	case 0xb4: // LDY
		opldy(read(zeropagex()));
		m_regs.pc++;
		break;

	case 0x75:  // ADC
		opadc(read(zeropagex()));
		m_regs.pc++;
		break;

	case 0xf5: // SBC
		opsbc(read(zeropagex()));
		m_regs.pc++;
		break;

	case 0x35: // AND
		opand(read(zeropagex()));
		m_regs.pc++;
		break;

	case 0x55: // EOR
		opeor(read(zeropagex()));
		m_regs.pc++;
		break;

	case 0x15: // ORA
		opora(read(zeropagex()));
		m_regs.pc++;
		break;

	case 0xd5: // CMP
		opcmp(read(zeropagex()));
		m_regs.pc++;
		break;

	case 0x95: // STA
		opsta(zeropagex());
		m_regs.pc++;
		break;

	case 0x94: // STY
		opsty(zeropagex());
		m_regs.pc++;
		break;

	case 0xf6: // INC
		opinc(zeropagex());
		m_regs.pc++;
		break;

	case 0xd6: // DEC
		opdec(zeropagex());
		m_regs.pc++;
		break;

	case 0x16: // ASL
		opasl(zeropagex());
		m_regs.pc++;
		break;

	case 0x56: // LSR
		oplsr(zeropagex());
		m_regs.pc++;
		break;

	case 0x36: // ROL
		oprol(zeropagex());
		m_regs.pc++;
		break;

	case 0x76: // ROR
		opror(zeropagex());
		m_regs.pc++;
		break;

		/* ---------------------------------------------
		Zero page indexed with Y. nn+Y
		--------------------------------------------- */
	case 0xb6: // LDX
		opldx(read(zeropagey()));
		m_regs.pc++;
		break;

	case 0x96: // STX
		opstx(zeropagey());
		m_regs.pc++;
		break;

		/* ---------------------------------------------
		Absolute. nnnn
		--------------------------------------------- */
	case 0xad: // LDA
		oplda(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0xae: // LDX
		opldx(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0xac: // LDY
		opldy(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0x6d: // ADC
		opadc(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0xed: // SBC
		opsbc(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0x2d: // AND
		opand(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0x4d: // EOR
		opeor(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0x0d: // ORA
		opora(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0xcd: // CMP
		opcmp(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0xec: // CPX
		opcpx(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0xcc: // CPY
		opcpy(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0x8d: // STA
		opsta(absolute());
		m_regs.pc += 2;
		break;

	case 0x8e: // STX
		opstx(absolute());
		m_regs.pc += 2;
		break;

	case 0x8C: // STY
		opsty(absolute());
		m_regs.pc += 2;
		break;

	case 0x2c: // BIT
		opbit(read(absolute()));
		m_regs.pc += 2;
		break;

	case 0xee: // INC
		opinc(absolute());
		m_regs.pc += 2;
		break;

	case 0xce:	 // DEC
		opdec(absolute());
		m_regs.pc += 2;
		break;

	case 0x0e: // ASL
		opasl(absolute());
		m_regs.pc += 2;
		break;

	case 0x4e: // LSR
		oplsr(absolute());
		m_regs.pc += 2;
		break;

	case 0x2e: // ROL
		oprol(absolute());
		m_regs.pc += 2;
		break;

	case 0x6e: // ROR
		opror(absolute());
		m_regs.pc += 2;
		break;

		/* ---------------------------------------------
		Absolute,X. nnnn+X
		--------------------------------------------- */
	case 0xbd: // LDA
		oplda(read(absolutex()));
		m_regs.pc += 2;
		break;

	case 0xbc: // LDY
		opldy(read(absolutex()));
		m_regs.pc += 2;
		break;

	case 0x7d: // ADC
		opadc(read(absolutex()));
		m_regs.pc += 2;
		break;

	case 0xfd: // SBC
		opsbc(read(absolutex()));
		m_regs.pc += 2;
		break;

	case 0x3d: // AND
		opand(read(absolutex()));
		m_regs.pc += 2;
		break;

	case 0x5d: // EOR
		opeor(read(absolutex()));
		m_regs.pc += 2;
		break;

	case 0x1d: // ORA
		opora(read(absolutex()));
		m_regs.pc += 2;
		break;

	case 0xdd: // CMP
		opcmp(read(absolutex()));
		m_regs.pc += 2;
		break;

	case 0x9d: // STA
		opsta(absolutex());
		m_regs.pc += 2;
		break;

	case 0xfe: // INC
		opinc(absolutex());
		m_regs.pc += 2;
		break;

	case 0xde: // DEC
		opdec(absolutex());
		m_regs.pc += 2;
		break;

	case 0x1e: // ASL
		opasl(absolutex());
		m_regs.pc += 2;
		break;

	case 0x5e: // LSR
		oplsr(absolutex());
		m_regs.pc += 2;
		break;

	case 0x3e: // ROL
		oprol(absolutex());
		m_regs.pc += 2;
		break;

	case 0x7e: // ROR
		opror(absolutex());
		m_regs.pc += 2;
		break;

		/* ---------------------------------------------
		Absolute,Y. nnnn+Y
		--------------------------------------------- */
	case 0xb9: // LDA 
		oplda(read(absolutey()));
		m_regs.pc += 2;
		break;

	case 0xbe: // LDX
		opldx(read(absolutey()));
		m_regs.pc += 2;
		break;

	case 0x79: // ADC
		opadc(read(absolutey()));
		m_regs.pc += 2;
		break;

	case 0xf9: // SBC
		opsbc(read(absolutey()));
		m_regs.pc += 2;
		break;

	case 0x39: // AND
		opand(read(absolutey()));
		m_regs.pc += 2;
		break;

	case 0x59: // EOR
		opeor(read(absolutey()));
		m_regs.pc += 2;
		break;

	case 0x19: // ORA
		opora(read(absolutey()));
		m_regs.pc += 2;
		break;

	case 0xd9: // CMP 
		opcmp(read(absolutey()));
		m_regs.pc += 2;
		break;

	case 0x99: // STA
		opsta(absolutey());
		m_regs.pc += 2;
		break;

		/* ---------------------------------------------
		(Indirect,X): Indexed indirect. u16[nn+X]
		--------------------------------------------- */
	case 0xa1: // LDA 
		oplda(read(idxind()));
		m_regs.pc++;
		break;

	case 0x61: // ADC 
		opadc(read(idxind()));
		m_regs.pc++;
		break;

	case 0xe1: // SBC
		opsbc(read(idxind()));
		m_regs.pc++;
		break;

	case 0x21: // AND
		opand(read(idxind()));
		m_regs.pc++;
		break;

	case 0x41: // EOR
		opeor(read(idxind()));
		m_regs.pc++;
		break;

	case 0x01: // ORA
		opora(read(idxind()));
		m_regs.pc++;
		break;

	case 0xc1: // CMP
		opcmp(read(idxind()));
		m_regs.pc++;
		break;

	case 0x81: // STA
		opsta(idxind());
		m_regs.pc++;
		break;

		/* ---------------------------------------------
		(Indirect),Y: Indirect indexed. u16[nn]+Y
		--------------------------------------------- */
	case 0xb1: // LDA
		oplda(read(indidx()));
		m_regs.pc++;
		break;

	case 0x71: // ADC
		opadc(read(indidx()));
		m_regs.pc++;
		break;

	case 0xf1: // SBC
		opsbc(read(indidx()));
		m_regs.pc++;
		break;

	case 0x31: // AND
		opand(read(indidx()));
		m_regs.pc++;
		break;

	case 0x51: // EOR	
		opeor(read(indidx()));
		m_regs.pc++;
		break;

	case 0x11: // ORA
		opora(read(indidx()));
		m_regs.pc++;
		break;

	case 0xd1: // CMP
		opcmp(read(indidx()));
		m_regs.pc++;
		break;

	case 0x91: // STA
		opsta(indidx());
		m_regs.pc++;
		break;

		/* ---------------------------------------------- */

	case 0x58: // CLI: Clear interrupt disable bit. I=0
		m_regs.seti(0);
		break;

	case 0x78: // SEI: Set interrupt disable bit. I=1
		m_regs.seti(1);
		break;

	case 0xb8: // CLV: Clear overflow flag. V=0 
		m_regs.seto(0);
		break;

	case 0x18: // CLC: Clear carry flag. C=0 
		m_regs.setc(0);
		break;

	case 0x38: // SEC: Set carry flag. C=1
		m_regs.setc(1);
		break;

	case 0xf8: // SED: Set decimal mode. D=1
		m_regs.setd(1);
		break;

	case 0xd8: // CLD: Clear decimal mode. D=0
		m_regs.setd(0);
		break;


	default:
		break;
	}
}

u16 nes_cpu::read16(u16 a)
{ 
	return (read(a+1) << 8 | read(a)); 
}

void nes_cpu::push(u8 b) 
{
	write(m_regs.sp+0x100, b);
	m_regs.sp--;
}
	
void nes_cpu::push16(u16 w)
{
	write(m_regs.sp+0x100, w >> 8);
	m_regs.sp--;
	write(m_regs.sp+0x100, w & 0xff);
	m_regs.sp--;
}
	
u8 nes_cpu::pop()
{
	m_regs.sp++;
	return read(m_regs.sp+0x100);
}
	
u16 nes_cpu::pop16() 
{
	m_regs.sp++;
	u8 lo = read(m_regs.sp+0x100);
	m_regs.sp++;
	u8 hi = read(m_regs.sp+0x100);
	return (hi << 8) | lo;
}

void nes_cpu::oplda(u8 m) 
{
	m_regs.a = m;
	m_regs.setn(m & 0x80);
	m_regs.setz(!m);
}

void nes_cpu::opldx(u8 m) 
{
	m_regs.x = m;
	m_regs.setn(m & 0x80);
	m_regs.setz(!m);
}

void nes_cpu::opldy(u8 m)
{
	m_regs.y = m;
	m_regs.setn(m & 0x80);
	m_regs.setz(!m);
}

void nes_cpu::opadc(u8 m) 
{
	u16 sum = m_regs.a + m + m_regs.issetc();
	m_regs.setz(!(sum & 0xff));
	m_regs.setn(sum & 0x80);
	m_regs.seto(
		~(m_regs.a ^ m) & (m_regs.a ^ sum) & 0x80 
		// alternatively...
		/* !((m_regs.a ^ m) & 0x80) && ((m_regs.a ^ sum) & 0x80) */
	);
	m_regs.setc(sum > 0xff);
	m_regs.a = sum & 0xff;
}

void nes_cpu::opsbc(u8 m) 
{
#if 1
	opadc(~m);	// 6502 subtraction is equivalent to ones complement addition.
#else
	u16 sum = m_regs.a - m - !m_regs.issetc();
	m_regs.setz(!(sum & 0xff));
	m_regs.setn(sum & 0x80);
	m_regs.seto(
		((m_regs.a ^ sum) & 0x80) && ((m_regs.a ^ m) & 0x80)
		);
	m_regs.setc(sum < 0x100);
	m_regs.a = sum & 0xff;
#endif
}

void nes_cpu::opand(u8 m)
{
	m_regs.a &= m;
	m_regs.setn(m_regs.a & 0x80);
	m_regs.setz(!m_regs.a);
}

void nes_cpu::opeor(u8 m)
{
	m_regs.a ^= m;
	m_regs.setn(m_regs.a & 0x80);
	m_regs.setz(!m_regs.a);
}

void nes_cpu::opora(u8 m)
{
	m_regs.a |= m;
	m_regs.setn(m_regs.a & 0x80);
	m_regs.setz(!m_regs.a);
}

void nes_cpu::opcmp(u8 m)
{
	u16 t = m_regs.a - m;
	m_regs.setc(t < 0x100);
	m_regs.setn(t & 0x80);
	m_regs.setz(!(t & 0xff));	
}

void nes_cpu::opcpx(u8 m) 
{
	u16 t = m_regs.x - m;
	m_regs.setc(t < 0x100);
	m_regs.setn(t & 0x80);
	m_regs.setz(!(t & 0xff));	
}

void nes_cpu::opcpy(u8 m) 
{
	u16 t = m_regs.y - m;
	m_regs.setc(t < 0x100);
	m_regs.setn(t & 0x80);
	m_regs.setz(!(t & 0xff));
}

void nes_cpu::opsta(u16 a)
{
	write(a,m_regs.a);
}

void nes_cpu::opstx(u16 a)
{
	write(a,m_regs.x);
}

void nes_cpu::opsty(u16 a) 
{
	write(a,m_regs.y);
}

void nes_cpu::opbit(u8 m)
{
	m_regs.setn(m & FLG_NEGATIVE);
	m_regs.seto(m & FLG_OVERFLOW);
	m_regs.setz(!(m & m_regs.a));	
}

void nes_cpu::opinc(u16 a)
{
	u8 m = read(a) + 1;
	m_regs.setn(m & 0x80);
	m_regs.setz(!m);
	write(a,m);
}

void nes_cpu::opdec(u16 a) 
{
	u8 m = read(a) - 1;
	m_regs.setn(m & 0x80);
	m_regs.setz(!m);
	write(a,m);
}

void nes_cpu::opasl(u16 a) 
{
	u8 m = read(a);
	m_regs.setc(m & 0x80);
	m <<= 1;
	m_regs.setn(m & 0x80);
	m_regs.setz(!m);	
	write(a,m);
}

void nes_cpu::oplsr(u16 a) 
{
	u8 m = read(a);
	m_regs.setc(m & 0x01);
	m >>= 1;
	m_regs.setn(0);
	m_regs.setz(!m);	
	write(a,m);
}

void nes_cpu::oprol(u16 a)
{
	u16 m = read(a);
	m <<= 1;
	m |= m_regs.issetc();
	m_regs.setc(m > 0xff);
	m &= 0xff;
	m_regs.setn(m & 0x80);
	m_regs.setz(!m);
	write(a,(u8)m);
}

void nes_cpu::opror(u16 a) 
{
	u16 m = read(a);
	m |= (m_regs.issetc() << 8);
	m_regs.setc(m & 0x01);
	m >>= 1;
	m &= 0xff;
	m_regs.setn(m & 0x80);
	m_regs.setz(!m);
	write(a,(u8)m);
}

void nes_cpu::attach_devices(nes_device_map_t device_map)
{
	m_ppu = std::dynamic_pointer_cast<nes_ppu>(device_map["ppu"]);
	if (!m_ppu) throw mines_exception("nes_cpu: Failed to get nes_ppu device.");

	m_cart = std::dynamic_pointer_cast<nes_cart>(device_map["cart"]);
	if (!m_cart) throw mines_exception("nes_cpu: Failed to get nes_cart device.");

	//m_apu = std::dynamic_pointer_cast<nes_apu>(device_map["apu"]);
	//if (!m_apu) throw mines_exception("nes_cpu: Failed to get nes_apu device.");

	m_controllers = std::dynamic_pointer_cast<nes_controllers>(device_map["controllers"]);
	if (!m_controllers) throw mines_exception("nes_cpu: Failed to get nes_controllers device.");
}
