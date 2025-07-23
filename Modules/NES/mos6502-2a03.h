//MOS 6502 interpreter for MOSES. Planned support: Atari 2600, NES, Apple II, Atari 800, etc.
//This one is for the 2A03 in the NES.
//Friday 20th of June, 2025

struct bus;

class Mos6502_2a03{
	
	private:
	//Register definitions
	uint8_t a = 0; //Accumulator
	uint8_t sr = 32; //Status register - only 7 bits needed
	uint8_t sp = 0; //Stack pointer
	uint8_t x = 0; //Index register
	uint8_t y = 0; //Index register
	
	//Other variables for the interpreter to remember CPU state
	bool doBranch;
	union PrgCnt{
		uint16_t pc;
		uint8_t pch; //High byte of program counter
		uint8_t pcl; //Low byte of program counter
	};
	PrgCnt prgcnt = {0x0};
	uint8_t extraTicks = 0; //Some instructions require more memory accesses
	uint8_t curOpcode;
	uint8_t addressMode;
	uint16_t stackOffset;
	bool newCycle = true;
	uint8_t dummy; //This is where we put dummy reads and writes. Should never be used.
	
	/* 6502 addressing modes:
	 * Accumulator
	 * Immediate 
	 * Zero page			0b001
	 * Zero page Indexed	0b101
	 * Relative
	 * Absolute				0b011
	 * Absolute Indexed
	 * Indirect
	 * Indexed Indirect
	 * Indirect Indexed
	 */

	void setFlag(char flag, bool set){ //Shorthand for setting flags
		switch(flag){
			case 'n':
				set ? sr |= 0b10100000 : sr &= 0b01111111;
			break;
			case 'v':
				set ? sr |= 0b01100000 : sr &= 0b10111111;
			break;
			case 'b':
				set ? sr |= 0b00110000 : sr &= 0b11101111;
			break;
			case 'd':
				set ? sr |= 0b00101000 : sr &= 0b11110111;
			break;
			case 'i':
				set ? sr |= 0b00100100 : sr &= 0b11111011;
			break;
			case 'z':
				set ? sr |= 0b00100010 : sr &= 0b11111101;
			break;
			case 'c':
				set ? sr |= 0b00100001 : sr &= 0b11111110;
			break;
		}
	}
	
	bool getFlag(char flag){
		switch(flag){
			case 'n':
				return ((sr & 0b10000000) >> 7);
			break;
			case 'v':
				return ((sr & 0b01000000) >> 6);
			break;
			case 'b':
				return ((sr & 0b00010000) >> 4);
			break;
			case 'd':
				return ((sr & 0b00001000) >> 3);
			break;
			case 'i':
				return ((sr & 0b00000100) >> 2);
			break;
			case 'z':
				return ((sr & 0b00000010) >> 1);
			break;
			case 'c':
				return (sr & 0b00000001);
			break;
		}
	}
	
	void finish(){
		newCycle = true;
		extraTicks = 0;
	}
	
	//Implementing the standard set of 6502 opcodes for now. Undocumented opcodes later.
	void illegalOpcode(){
		std::cout << "GURU MEDITATION illegal opcode";
		finish();
	}
	
	public: Mos6502_2a03(uint16_t stack){
		stack = stackOffset;
	}

	void tick(uint32_t steps){ //Official instruction set and complete CMOS instruction set
		for(uint32_t i = 0; i < steps; i++){
			if(extraTicks == 0 && newCycle){
				curOpcode = bus.read(prgcnt.pc);
				prgcnt.pc++;
				prgcnt.pch = (prgcnt.pc & 0b1111111100000000) >> 8;
				prgcnt.pcl = (prgcnt.pc & 0b0000000011111111);
				newCycle = false;
			}else{
				uint16_t address;
				uint16_t pointer;
				uint8_t latch;
				bool carry = false; //Checks if we're crossing the page boundary.
				switch(curOpcode){
					case 0x00: //BRK - Force Break
						switch(extraTicks){
							case 0:
								dummy = bus.read(prgcnt.pc); //Dummy read. We have to do it anyway to know when the bus is busy.
								prgcnt.pc++;
								break;
							case 1:
								bus.write(sp, prgcnt.pch);
								setFlag('b', true);
								sp--;
								break;
							case 2:
								bus.write(sp, prgcnt.pcl);
								sp--;
								break;
							case 3:
								bus.write(sp, sr);
								sp--;
								break;
							case 4:
								prgcnt.pcl = bus.read(0xFFFE);
								break;
							case 5:
								prgcnt.pch = bus.read(0xFFFF);
								prgcnt.pc = prgcnt.pch << 8;
								prgcnt.pc += prgcnt.pcl;
								finish();
								break;
							}
					break;
					case 0x01: //ORA - OR with accumulator - Indirect, Y
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + y;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += bus.read(pointer+1) << 8;
								break;
							case 4:
								a |= bus.read(address);
								finish();
								break;
							}
					break;
					case 0x02: //JAM - Undocumented
					case 0x03: //SLO - Undocumented
					case 0x04: //NOP - Undocumented
					case 0x05: //ORA - OR with accumulator - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
							case 1:
								a |= bus.read(address);
								finish();
						}
						break;
					case 0x06: //ASL - Shift Left One Bit - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(address);
								break;
							case 2:
								setFlag('c', (pointer & 0b10000000));
								bus.write(pointer, address);
								break;
							case 3:
								bus.write((pointer << 1), address);
								finish();
								break;
							}
						break;
					case 0x07: //SLO - Undocumented
					case 0x08: //PHP - Push Processor Status on Stack
						switch(extraTicks){
							case 0:
								dummy = bus.read(prgcnt.pc); //Dummy read. We have to do it anyway to know when the bus is busy.
								prgcnt.pc++;
								break;
							case 1:
								bus.write(sp, sr);
								sp--;
								finish();
								break;
							}
						break;
					case 0x09: //ORA - Or memory with accumulator - Immediate
						a |= bus.read(prgcnt.pc);
						finish();
						break;
					case 0x0A: //ASL - Shift Left One Bit - Accumulator
						a = (bus.read(prgcnt.pc) << 1);
						finish();
						break;
					case 0x0B: //ANC - Undocumented
					case 0x0C: //NOP - Undocumented
					case 0x0D: //ORA - Or memory with accumulator - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								a |= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x0E: //ASL - Shift Left One Bit - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								pointer = bus.read(address);
								break;
							case 3:
								setFlag('c', (pointer & 0b10000000));
								bus.write(pointer, address);
								break;
							case 4:
								bus.write((pointer << 1), address);
								finish();
								break;
						break;
					case 0x0F: //SLO - Undocumented
					case 0x10: //BPL - Branch if Positive
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(prgcnt.pc);
								break;
							case 2:
								if(!getFlag('n')){
									prgcnt.pcl += address;
									doBranch = true;
								}else{
									prgcnt.pc++;
								}
								break;
							case 3:
								if(doBranch){
									doBranch = false;
									
								}
								finish();
								break;
						}
						break;
					break;
					case 0x11: //ORA - Or memory with accumulator - Indirect, X
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + x;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += bus.read(pointer+1) << 8;
								break;
							case 4:
								a |= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x12: //JAM - Undocumented
					case 0x13: //SLO - Undocumented
					case 0x14: //NOP - Undocumented
					case 0x15: //ORA - Or memory with accumulator - Zeropage, X
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += x;
								break;
							case 2:
								a |= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x16: //ASL
					case 0x17: //SLO - Undocumented
					case 0x18: //CLC - Clear Carry Flag
						setFlag('c', false);
						finish();
						break;
					case 0x19: //ORA - Or memory with accumulator - Absolute, Y
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + y >= 256);
								address += bus.read(prgcnt.pc) << 8;
								address += y;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a |= bus.read(address);
								}else{
									a |= bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a |= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x1A: //NOP - Undocumented
					case 0x1B: //SLO - Undocumented
					case 0x1C: //NOP - Undocumented
					case 0x1D: //ORA - Or memory with accumulator - Absolute, X
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + x >= 256);
								address += bus.read(prgcnt.pc) << 8;
								address += x;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a |= bus.read(address);
								}else{
									a |= bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a |= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x1E: //ASL
					case 0x1F: //SLO - Undocumented
					case 0x20: //JSR - Jump, Saving Return Address
						switch(extraTicks){
							case 0:
								prgcnt.pc++;
								address = 0 + bus.read(prgcnt.pc);
								break;
							case 1:
								//???
								break;
							case 2:
								bus.write((prgcnt.pc & 0b1111111100000000 >> 8), sp);
								sp--;
								break;
							case 3:
								bus.write((prgcnt.pc & 0b0000000011111111), sp);
								sp--;
								prgcnt.pcl = (address & 0b0000000011111111);
								prgcnt.pch = bus.read(prgcnt.pc);
								prgcnt.pc = (prgcnt.pch << 8);
								prgcnt.pc += prgcnt.pcl;
								finish();
								break;
							}
						break;
					case 0x21: //AND - And Memory with Accumulator - Indirect, Y
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + y;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += (bus.read(pointer+1) << 8);
								break;
							case 4:
								a &= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x22: //JAM - Undocumented
					case 0x23: //RLA - Undocumented
					case 0x24: //BIT - Test Bits - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								
								break;
							}
						break;
					case 0x25: //AND - And Memory with Accumulator - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								a &= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x26: //ROL - Rotate Left - Zero Page
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(address);
								break;
							case 2:
								bus.write(pointer &= getFlag('c'), address);
								setFlag('c', (pointer & 0b1000000));
								break;
							case 3:
								bus.write((pointer << 1), address);
								finish();
								break;
							}
						break;
					case 0x27: //RLA - Undocumented
					case 0x28: //PLP - Pull Processor Status from Stack
						switch(extraTicks){
							case 0:
								dummy = bus.read(prgcnt.pc); //Dummy read. We have to do it anyway to know when the bus is busy.
								prgcnt.pc++;
								break;
							case 1:
								sp++;
								break;
							case 2:
								sr = bus.read(sp);
								finish();
								break;
							}
						break;
					case 0x29: //AND - And Memory with Accumulator - Immediate
						a &= bus.read(prgcnt.pc);
						finish();
						break;
					case 0x2A: //ROL
					case 0x2B: //ANC - Undocumented
					case 0x2C: //BIT
					case 0x2D: //AND - And Memory with Accumulator - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								a &= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x2E: //ROL - Rotate Left - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								pointer = bus.read(address);
								break;
							case 3:
								bus.write(pointer &= getFlag('c'), address);
								setFlag('c', (pointer & 0b1000000));
								break;
							case 4:
								bus.write((pointer << 1), address);
								finish();
								break;
						break;
					case 0x2F: //RLA - Undocumented
					case 0x30: //BMI - Branch if Minus
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(prgcnt.pc);
								break;
							case 2:
								if(getFlag('n')){
									prgcnt.pcl += address;
									doBranch = true;
								}else{
									prgcnt.pc++;
								}
								break;
							case 3:
								if(doBranch){
									doBranch = false;
									
								}
								finish();
								break;
						}
						break;
					case 0x31: //AND - And Memory with Accumulator - Indirect, X
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + x;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += (bus.read(pointer+1) << 8);
								break;
							case 4:
								a &= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x32: //JAM - Undocumented
					case 0x33: //RLA - Undocumented
					case 0x34: //NOP - Undocumented
					case 0x35: //AND - And Memory with Accumulator - Zeropage, X
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += x;
								break;
							case 2:
								a &= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x36: //ROL
					case 0x37: //RLA - Undocumented
					case 0x38: //SEC - Set Carry Flag
						setFlag('c', true);
						finish();
						break;
					case 0x39: //AND - And Memory with Accumulator - Absolute, Y
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + y >= 256);
								address += (bus.read(prgcnt.pc) << 8);
								address += y;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a &= bus.read(address);
								}else{
									a &= bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a &= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x3A: //NOP - Undocumented
					case 0x3B: //RLA - Undocumented
					case 0x3C: //NOP - Undocumented
					case 0x3D: //AND - And Memory with Accumulator - Absolute, X
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + x >= 256);
								address += (bus.read(prgcnt.pc) << 8);
								address += x;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a &= bus.read(address);
								}else{
									a &= bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a &= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x3E: //ROL
					case 0x3F: //RLA - Undocumented
					case 0x40: //RTI - Return from Interrupt
						switch(extraTicks){
							case 0:
								dummy = bus.read(prgcnt.pc); //Dummy read. We have to do it anyway to know when the bus is busy.
								prgcnt.pc++;
								break;
							case 1:
								sp++;
								break;
							case 2:
								sr = bus.read(sp);
								sp++;
								break;
							case 3:
								prgcnt.pcl = bus.read(sp);
								sp++;
								break;
							case 4:
								prgcnt.pch = bus.read(sp);
								prgcnt.pc = prgcnt.pch << 8;
								prgcnt.pc += prgcnt.pcl;
								finish();
								break;
							}
						break;
					case 0x41: //EOR - Exclusive-Or with Accumulator - Indirect, Y
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + y;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += (bus.read(pointer+1) << 8);
								break;
							case 4:
								a = bus.read(address);
								finish();
								break;
						}
						break;
					case 0x45: //EOR - Exclusive-Or with Accumulator - Zeropage
						switch(extraTicks){
							case 0:
								address ^= bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								a ^= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x46: //LSR - Shift One Bit Right - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(address);
								break;
							case 2:
								setFlag('c', (pointer & 0b10000000));
								bus.write(pointer, address);
								break;
							case 3:
								bus.write((pointer >> 1), address);
								finish();
								break;
							}
						break;
					case 0x48: //PHA - Push Accumulator on Stack
						switch(extraTicks){
							case 0:
								dummy = bus.read(prgcnt.pc); //Dummy read. We have to do it anyway to know when the bus is busy.
								prgcnt.pc++;
								break;
							case 1:
								bus.write(sp, a);
								sp--;
								finish();
								break;
						}
						break;
					case 0x49: //EOR - Exclusive-Or with Accumulator - Immediate
						a ^= bus.read(prgcnt.pc);
						finish();
						break;
					case 0x4A: //LSR - Undocumented
					case 0x4C: //JMP - Jump - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								prgcnt.pc = (bus.read(prgcnt.pc) << 8) + address;
								finish();
								break; 
						}
						break;
					case 0x4D: //EOR - Exclusive-Or with Accumulator - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								a ^= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x4E: //LSR
					case 0x50: //BVC - Branch if Overflow Clear
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(prgcnt.pc);
								break;
							case 2:
								if(!getFlag('v')){
									prgcnt.pcl += address;
									doBranch = true;
								}else{
									prgcnt.pc++;
								}
								break;
							case 3:
								if(doBranch){
									doBranch = false;
									
								}
								finish();
								break;
						}
						break;
					case 0x51: //EOR - Exclusive-Or with Accumulator - Indirect, X
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + x;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += (bus.read(pointer+1) << 8);
								break;
							case 4:
								a ^= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x55: //EOR - Exclusive-Or with Accumulator - Zeropage, X
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += x;
								break;
							case 2:
								a ^= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x56: //LSR
					case 0x58: //CLI - Clear Interrupt Disable Bit
						setFlag('i', false);
						finish();
						break;
					case 0x59: //EOR - Exclusive-Or Memory with Accumulator - Absolute, Y
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + y >= 256);
								address += (bus.read(prgcnt.pc) << 8);
								address += y;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a = bus.read(address);
								}else{
									a = bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a ^= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x5D: //EOR - Exclusive-Or Memory with Accumulator - Absolute, X
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + x >= 256);
								address += (bus.read(prgcnt.pc) << 8);
								address += x;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a ^= bus.read(address);
								}else{
									a ^= bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a ^= bus.read(address);
								finish();
								break;
						}
						break;
					case 0x5E: //LSR
					case 0x60: //RTS - Return from Subroutine
						switch(extraTicks){
							case 0:
								dummy = bus.read(prgcnt.pc); //Dummy read. We have to do it anyway to know when the bus is busy.
								break;
							case 1:
								sp++;
								break;
							case 2:
								prgcnt.pcl = bus.read(sp);
								sp++;
								break;
							case 3:
								prgcnt.pch = bus.read(sp);
								prgcnt.pc = prgcnt.pch << 8;
								prgcnt.pc += prgcnt.pcl;
								break;
							case 4:
								prgcnt.pc++;
								finish();
								break;
							}
						break;
					case 0x61: //ADC - Add to Accumulator with Carry - Indirect, Y
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + y;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += (bus.read(pointer+1) << 8);
								break;
							case 4:
								a += bus.read(address);
								finish();
								break;
						}
						break;
					case 0x65: //ADC - Add to Accumulator with Carry - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								a += bus.read(address);
								finish();
								break;
						}
						break;
					case 0x66: //ROR - Rotate Right - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(address);
								break;
							case 2:
								bus.write(pointer & (getFlag('c') << 8), address);
								setFlag('c', (pointer & 0b1));
								break;
							case 3:
								bus.write((pointer >> 1), address);
								finish();
								break;
						}
						break;
					case 0x68: //PLA - Pull Accumulator from Stack
						switch(extraTicks){
							case 0:
								dummy = bus.read(prgcnt.pc); //Dummy read. We have to do it anyway to know when the bus is busy.
								prgcnt.pc++;
								break;
							case 1:
								sp++;
								break;
							case 2:
								a = bus.read(sp);
								finish();
								break;
						}
						break;
					case 0x69: //ADC - Add to Accumulator with Carry - Immediate
						a += bus.read(prgcnt.pc);
						finish();
						break;
					case 0x6A: //ROR
					case 0x6C: //JMP - Jump - Indirect
						switch(extraTicks){
							case 0:
								prgcnt.pc++;
								address = 0 + bus.read(prgcnt.pc);
								break;
							case 1:
								prgcnt.pc++;
								address += bus.read(prgcnt.pc) << 8;
								break;
							case 2:
								latch = bus.read(address);
								break;
							case 3:
								prgcnt.pcl = latch;
								prgcnt.pc = (prgcnt.pch << 8) + latch;
								finish();
								break;
						}
						break;
					case 0x6D: //ADC - Add to Accumulator with Carry - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								a += bus.read(address);
								finish();
								break;
						}
						break;
					case 0x6E: //ROR - Rotate Right - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								pointer = bus.read(address);
								break;
							case 3:
								bus.write(pointer & (getFlag('c') << 8), address);
								setFlag('c', (pointer & 0b1));
								break;
							case 4:
								bus.write((pointer >> 1), address);
								finish();
								break;
						}
						break;
					case 0x70: //BVS - Branch if Overflow Set
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(prgcnt.pc);
								break;
							case 2:
								if(getFlag('v')){
									prgcnt.pcl += address;
									doBranch = true;
								}else{
									prgcnt.pc++;
								}
								break;
							case 3:
								if(doBranch){
									doBranch = false;
									
								}
								finish();
								break;
						}
						break;
					case 0x71: //ADC - Add to Accumulator with Carry - Indirect, X
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + x;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += (bus.read(pointer+1) << 8);
								break;
							case 4:
								a += bus.read(address);
								finish();
								break;
						}
						break;
					case 0x75: //ADC - Add to Accumulator with Carry - Zeropage, X
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += x;
								break;
							case 2:
								a += bus.read(address);
								finish();
								break;
						}
						break;
					case 0x76: //ROR
					case 0x78: //SEI - Set Interrupt Disable Status
						setFlag('i', true);
						finish();
						break;
					case 0x79: //ADC - Add to Accumulator with Carry - Absolute, Y
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + y >= 256);
								address += (bus.read(prgcnt.pc) << 8);
								address += y;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a += bus.read(address);
								}else{
									a += bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a += bus.read(address);
								finish();
								break;
						}
						break;
					case 0x7D: //ADC - Add to Accumulator with Carry - Absolute, X
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + x >= 256);
								address += (bus.read(prgcnt.pc) << 8);
								address += x;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a += bus.read(address);
								}else{
									a += bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a += bus.read(address);
								finish();
								break;
						}
						break;
					case 0x7E: //ROR
					case 0x81: //STA
					case 0x84: //STY - Store Y Register - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								bus.write(y, address);
								finish();
								break;
						}
						break;
					case 0x85: //STA - Store Accumulator - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								bus.write(a, address);
								finish();
								break;
						}
						break;
					case 0x86: //STX - Store X Register - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								bus.write(x, address);
								finish();
								break;
						}
						break;
					case 0x87: //SAX - Undocumented
					case 0x88: //DEY
					case 0x8A: //TXA - Transfer X to Accumulator
						a = x;
						setFlag('z', (a == 0));
						setFlag('b', (a & 0b10000000));
						finish();
						break;
					case 0x8C: //STY - Store Y Register - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								bus.write(y, address);
								finish();
								break;
						}
						break;
					case 0x8D: //STA - Store Accumulator - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								bus.write(a, address);
								finish();
								break;
						}
						break;
					case 0x8E: //STX - Store X Register - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								bus.write(x, address);
								finish();
								break;
						}
						break;
					case 0x8F: //SAX - Undocumented
					case 0x90: //BCC - Branch if Carry Clear 
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(prgcnt.pc);
								break;
							case 2:
								if(!getFlag('c')){
									prgcnt.pcl += address;
									doBranch = true;
								}else{
									prgcnt.pc++;
								}
								break;
							case 3:
								if(doBranch){
									doBranch = false;
									
								}
								finish();
								break;
						}
						break;
					case 0x91: //STA
					case 0x94: //STY - Store Y Register - Zeropage, X
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(address);
								prgcnt.pc++;
								break;
							case 2:
								bus.write(y, pointer+x);
								finish();
								break;
						}
						break;
					case 0x95: //STA - Store Accumulator - Zeropage, X
					switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(address);
								prgcnt.pc++;
								break;
							case 2:
								bus.write(a, pointer+x);
								finish();
								break;
						}
						break;
					case 0x96: //STX - Store X Register - Zeropage, Y
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(address);
								prgcnt.pc++;
								break;
							case 2:
								bus.write(x, pointer+y);
								finish();
								break;
						}
						break;
					case 0x97: //SAX - Undocumented
					case 0x98: //TYA - Transfer Y to Accumulator
						a = y;
						setFlag('z', (a == 0));
						setFlag('b', (a & 0b10000000));
						finish();
						break;
					case 0x99: //STA
					case 0x9A: //TXS - Transfer X to Stack Register
						sp = x;
						setFlag('z', (sp == 0));
						setFlag('b', (sp & 0b10000000));
						finish();
						break;
					case 0x9D: //STA
					case 0xA0: //LDY - Load Index Y - Indirect, X
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + x;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += bus.read(pointer+1) << 8;
								break;
							case 4:
								y = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xA1: //LDA - Load Accumulator - Indirect, Y
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + y;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += bus.read(pointer+1) << 8;
								break;
							case 4:
								a = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xA2: //LDX - Load Index X - Indirect, Y
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + y;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += bus.read(pointer+1) << 8;
								break;
							case 4:
								x = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xA4: //LDY - Load Index Y - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								y = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xA5: //LDA - Load Accumulator - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								a = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xA6: //LDX - Load Index X - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								x = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xA8: //TAY - Transfer Accumulator to Y
						y = a;
						setFlag('z', (y == 0));
						setFlag('b', (y & 0b10000000));
						finish();
						break;
					case 0xA9: //LDA - Load Accumulator - Immediate
						a = bus.read(prgcnt.pc);
						finish();
						break;
					case 0xAA: //TAX - Transfer Accumulator to X
						x = a;
						setFlag('z', (x == 0));
						setFlag('b', (x & 0b10000000));
						finish();
						break;
					case 0xAC: //LDY - Load Index Y - Immediate
						y = bus.read(prgcnt.pc);
						finish();
						break;
					case 0xAD: //LDA - Load Accumulator - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								a = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xAE: //LDX - Load Index X - Immediate
						x = bus.read(prgcnt.pc);
						finish();
						break;
					case 0xB0: //BCS - Branch if Carry Set
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(prgcnt.pc);
								break;
							case 2:
								if(getFlag('c')){
									prgcnt.pcl += address;
									doBranch = true;
								}else{
									prgcnt.pc++;
								}
								break;
							case 3:
								if(doBranch){
									doBranch = false;
									
								}
								finish();
								break;
						}
						break;
					case 0xB1: //LDA - Load Accumulator - Indirect, X
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + x;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += (bus.read(pointer+1) << 8);
								break;
							case 4:
								a = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xB4: //LDY - Load Index Y - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								y = bus.read(address);
								finish();
								break;
						}
						break;
						break;
					case 0xB5: //LDA - Load Accumulator - Zeropage, X
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += x;
								break;
							case 2:
								a = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xB6: //LDX - Load Index X - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								x = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xB8: //CLV - Clear Overflow Flag
						setFlag('v', false);
						finish();
						break;
					case 0xB9: //LDA - Load Accumulator - Absolute, Y
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + y >= 256);
								address += (bus.read(prgcnt.pc) << 8);
								address += y;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a = bus.read(address);
								}else{
									a = bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xBA: //TSX - Transfer Stack Pointer to X
						x = sp;
						setFlag('z', (x == 0));
						setFlag('n', (x & 0b10000000));
						finish();
					case 0xBC: //LDY - Load Index Y - Absolute, X
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + x >= 256);
								address += bus.read(prgcnt.pc) << 8;
								address += x;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									y = bus.read(address);
								}else{
									y = bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								y = bus.read(address);
								finish();
								break;
						}
					break;
					case 0xBD: //LDA - Load Accumulator - Absolute, X
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + x >= 256);
								address += bus.read(prgcnt.pc) << 8;
								address += x;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a = bus.read(address);
								}else{
									a = bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xBE: //LDX - Load Index X - Absolute, Y
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + y >= 256);
								address += bus.read(prgcnt.pc) << 8;
								address += y;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									x = bus.read(address);
								}else{
									x = bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								x = bus.read(address);
								finish();
								break;
						}
						break;
					case 0xC0: //CPY
					case 0xC1: //CMP - Compare Memory with Accumulator - Indirect, Y
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + y;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += (bus.read(pointer+1) << 8);
								break;
							case 4:
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xC4: //CPY
					case 0xC5: //CMP - Compare Memory with Accumulator - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xC6: //DEC - Decrement Memory - Zero page
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(address);
								break;
							case 2:
								bus.write(pointer--, address);
								break;
							case 3:
								bus.write(pointer, address);
								finish();
								break;
						}
						break;
					case 0xC8: //INY
					case 0xC9: //CMP - Compare Memory with Accumulator - Immediate
						a -= bus.read(prgcnt.pc);
						finish();
						break;
					case 0xCA: //DEX
					case 0xCC: //CPY
					case 0xCD: //CMP - Compare Memory with Accumulator - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xCE: //DEC - Decrement Memory - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								pointer = bus.read(address);
								break;
							case 3:
								bus.write(pointer--, address);
								break;
							case 4:
								bus.write(pointer, address);
								finish();
								break;
						}
						break;
					case 0xD0: //BNE - Branch if Not Equal
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(prgcnt.pc);
								break;
							case 2:
								if(!getFlag('z')){
									prgcnt.pcl += address;
									doBranch = true;
								}else{
									prgcnt.pc++;
								}
								break;
							case 3:
								if(doBranch){
									doBranch = false;
									
								}
								finish();
								break;
						}
						break;
					case 0xD1: //CMP - Compare Memory with Accumulator - Indirect, X
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + x;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += (bus.read(pointer+1) << 8);
								break;
							case 4:
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xD5: //CMP - Compare Memory with Accumulator - Zeropage, X
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += x;
								break;
							case 2:
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xD6: //DEC
					case 0xD8: //CLD - Clear Decimal Mode
						setFlag('d', false);
						finish();
						break;
					case 0xD9: //CMP - Compare Memory with Accumulator - Absolute, Y
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + y >= 256);
								address += bus.read(prgcnt.pc) << 8;
								address += y;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a -= bus.read(address);
								}else{
									a -= bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xDD: //CMP - Compare Memory with Accumulator - Absolute, X
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + x >= 256);
								address += (bus.read(prgcnt.pc) << 8);
								address += x;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a -= bus.read(address);
								}else{
									a -= bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xDE: //DEC
					case 0xE0: //CPX
					case 0xE1: //SBC - Subtract Memory from Accumulator with Borrow - Indirect, Y
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + y;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += bus.read(pointer+1) << 8;
								break;
							case 4:
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xE4: //CPX
					case 0xE5: //SBC - Subtract Memory from Accumulator with Borrow - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xE6: //INC - Increment Memory - Zeropage
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(address);
								break;
							case 2:
								bus.write(pointer++, address);
								break;
							case 3:
								bus.write(pointer, address);
								finish();
								break;
						}
						break;
					case 0xE8: //INX
					case 0xE9: //SBC - Subtract Memory from Accumulator with Borrow - Immediate
						a -= bus.read(prgcnt.pc);
						finish();
						break;
					case 0xEA: //NOP
					case 0xEC: //CPX
					case 0xED: //SBC - Subtract Memory from Accumulator with Borrow - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xEE: //INC - Increment Memory - Absolute
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += (bus.read(prgcnt.pc) << 8);
								prgcnt.pc++;
								break;
							case 2:
								pointer = bus.read(address);
								break;
							case 3:
								bus.write(pointer++, address);
								break;
							case 4:
								bus.write(pointer, address);
								finish();
								break;
						}
						break;
					case 0xF0: //BEQ - Branch if Equal
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(prgcnt.pc);
								break;
							case 2:
								if(getFlag('z')){
									prgcnt.pcl += address;
									doBranch = true;
								}else{
									prgcnt.pc++;
								}
								break;
							case 3:
								if(doBranch){
									doBranch = false;
									
								}
								finish();
								break;
						}
						break;
					case 0xF1: //SBC - Subtract Memory from Accumulator with Borrow - Indirect, X
						switch(extraTicks){
							case 0:
								pointer = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								pointer = bus.read(pointer) + x;
								break;
							case 2:
								address = bus.read(pointer);
								break;
							case 3:
								address += bus.read(pointer+1) << 8;
								break;
							case 4:
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xF5: //SBC - Subtract Memory from Accumulator with Borrow - Zeropage, X
						switch(extraTicks){
							case 0:
								address = bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								address += x;
								break;
							case 2:
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xF6: //INC
					case 0xF8: //SED - Set Decimal Flag
						setFlag('d', true);
						finish();
						break;
					case 0xF9: //SBC - Subtract Memory from Accumulator with Borrow - Absolute, Y
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + y >= 256);
								address += bus.read(prgcnt.pc) << 8;
								address += y;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a -= bus.read(address);
								}else{
									a -= bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xFD: //SBC - Subtract Memory from Accumulator with Borrow - Absolute, X
						switch(extraTicks){
							case 0:
								address = 0 + bus.read(prgcnt.pc);
								prgcnt.pc++;
								break;
							case 1:
								carry = (address + x >= 256);
								address += bus.read(prgcnt.pc) << 8;
								address += x;
								prgcnt.pc++;
								break;
							case 2:
								if(carry){
									address -= 256;
									a -= bus.read(address);
								}else{
									a -= bus.read(address);
									finish();
								}
								break;
							case 3:
								setFlag('c', true);
								a -= bus.read(address);
								finish();
								break;
						}
						break;
					case 0xFE: //INC
					default:
						//NOP
						break;
						}
						if(!newCycle){
							extraTicks++;
						}
					}
				}
			}
		}
	}
};
