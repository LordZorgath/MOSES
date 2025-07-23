//MOS 6502 interpreter for MOSES. Planned support: Atari 2600, NES, Apple II, Atari 800, etc.
//This one is the NMOS 6502, the most common variant (probably)
//Friday 20th of June, 2025
#include "appleiibus.h"


class Mos6502_nmos{
	
	private:
	//Register definitions
	uint8_t a = 0; //Accumulator
	uint8_t sr = 32; //Status register - only 7 bits needed
	uint16_t pc = 0; //16 bit program counter
	uint8_t sp = 0; //Stack pointer
	uint8_t x = 0; //Index register
	uint8_t y = 0; //Index register
	
	//Other variables for the interpreter to remember CPU state
	uint8_t pch; //High byte of program counter
	uint8_t pcl; //Low byte of program counter
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
	
	void finish(){
		newCycle = true;
		extraTicks = 0;
	}
	
	//Implementing the standard set of 6502 opcodes for now. Undocumented opcodes later.
	void illegalOpcode(){
		std::cout << "GURU MEDITATION illegal opcode";
		finish();
	}
	
	void ADC(){ //Add Memory to Accumulator with Carry
		uint16_t address;
		uint16_t pointer;
		bool carry = false; //Checks if we're crossing the page boundary.
		switch(addressMode){
			case 0b000: //Indirect, Y
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + y;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a += bus.read(address);
						finish();
				}
			case 0b001: //Zeropage
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						a += bus.read(address);
						finish();
				}
			case 0b010: //Immediate
				switch(extraTicks){
					case 0:
						a += bus.read(pc);
						finish();
				}
			case 0b011: //Absolute
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += bus.read(pc) << 8;
					case 2:
						a += bus.read(address);
						finish();
				}
			case 0b100: //Indirect, X
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + x;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a += bus.read(address);
						finish();
				}
			case 0b101: //Zeropage, X
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += x;
					case 2:
						a += bus.read(address);
						finish();
				}
			case 0b110: //Absolute, Y
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + y >= 256);
						address += bus.read(pc) << 8;
						address += y;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a += bus.read(address);
						}else{
							a += bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a += bus.read(address);
						finish();
				}
			case 0b111: //Absolute, X
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + x >= 256);
						address += bus.read(pc) << 8;
						address += x;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a += bus.read(address);
						}else{
							a += bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a += bus.read(address);
						finish();
				}
			}
	}
	void AND(){ //AND Memory with Accumulator
		uint16_t address;
		uint16_t pointer;
		bool carry = false; //Checks if we're crossing the page boundary.
		switch(addressMode){
			case 0b000: //Indirect, Y
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + y;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a &= bus.read(address);
						finish();
				}
			case 0b001: //Zeropage
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						a &= bus.read(address);
						finish();
				}
			case 0b010: //Immediate
				switch(extraTicks){
					case 0:
						a &= bus.read(pc);
						finish();
				}
			case 0b011: //Absolute
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += bus.read(pc) << 8;
					case 2:
						a &= bus.read(address);
						finish();
				}
			case 0b100: //Indirect, X
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + x;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a &= bus.read(address);
						finish();
				}
			case 0b101: //Zeropage, X
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += x;
					case 2:
						a &= bus.read(address);
						finish();
						
				}
			case 0b110: //Absolute, Y
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + y >= 256);
						address += bus.read(pc) << 8;
						address += y;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a &= bus.read(address);
						}else{
							a &= bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a &= bus.read(address);
						finish();
				}
			case 0b111: //Absolute, X
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + x >= 256);
						address += bus.read(pc) << 8;
						address += x;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a &= bus.read(address);
						}else{
							a &= bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a &= bus.read(address);
						finish();
				}
			}
	}
	void ASL(){
	
	}
	void BCC(){
		
	}
	void BCS(){
		
	}
	void BEQ(){
		
	}
	void BIT(){
		
	}
	void BMI(){
		
	}
	void BNE(){
		
	}
	void BPL(){
		
	}
	void BRK(){ //Force Break
		switch(extraTicks){
			case 0:
				dummy = bus.read(pc); //Dummy read. We have to do it anyway to know when the bus is busy.
				pc++;
			case 1:
				bus.write(sp, pch);
				setFlag('b', true);
				sp--;
			case 2:
				bus.write(sp, pcl);
				sp--;
			case 3:
				bus.write(sp, sr);
				sp--;
			case 4:
				pcl = bus.read(0xFFFE);
			case 5:
				pch = bus.read(0xFFFF);
				pc = pch << 8;
				pc += pcl;
				finish();
			}
	}
	void BVC(){
		
	}
	void BVS(){
		
	}
	void CLC(){ //Clear Carry Flag
		setFlag('c', false);
		finish();
	}
	void CLD(){ //Clear Decimal Mode
		setFlag('d', false);
		finish();
	}
	void CLI(){ //Clear Interrupt Disable Bit
		setFlag('i', false);
		finish();
	}
	void CLV(){ //Clear Overflow Flag
		setFlag('v', false);
		finish();
	}
	void CMP(){ //Compare Memory with Accumulator
		uint16_t address;
		uint16_t pointer;
		bool carry = false; //Checks if we're crossing the page boundary.
		switch(addressMode){
			case 0b000: //Indirect, Y
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + y;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a -= bus.read(address);
						finish();
				}
			case 0b001: //Zeropage
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						a -= bus.read(address);
						finish();
				}
			case 0b010: //Immediate
				switch(extraTicks){
					case 0:
						a -= bus.read(pc);
						finish();
				}
			case 0b011: //Absolute
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += bus.read(pc) << 8;
					case 2:
						a -= bus.read(address);
						finish();
				}
			case 0b100: //Indirect, X
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + x;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a -= bus.read(address);
						finish();
				}
			case 0b101: //Zeropage, X
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += x;
					case 2:
						a -= bus.read(address);
						finish();
				}
			case 0b110: //Absolute, Y
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + y >= 256);
						address += bus.read(pc) << 8;
						address += y;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a -= bus.read(address);
						}else{
							a -= bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a -= bus.read(address);
						finish();
				}
			case 0b111: //Absolute, X
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + x >= 256);
						address += bus.read(pc) << 8;
						address += x;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a -= bus.read(address);
						}else{
							a -= bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a -= bus.read(address);
						finish();
				}
			}
	}
	void CPX(){
		
	}
	void CPY(){
		
	}
	void DEC(){
		
	}
	void DEX(){
		
	}
	void DEY(){
		
	}
	void EOR(){ //Exclusive-OR Memory with Accumulator
		uint16_t address;
		uint16_t pointer;
		bool carry = false; //Checks if we're crossing the page boundary.
		switch(addressMode){
			case 0b000: //Indirect, Y
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + y;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a = bus.read(address);
						finish();
				}
			case 0b001: //Zeropage
				switch(extraTicks){
					case 0:
						address ^= bus.read(pc);
						pc++;
					case 1:
						a ^= bus.read(address);
						finish();
				}
			case 0b010: //Immediate
				switch(extraTicks){
					case 0:
						a ^= bus.read(pc);
						finish();
				}
			case 0b011: //Absolute
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += bus.read(pc) << 8;
					case 2:
						a ^= bus.read(address);
						finish();
				}
			case 0b100: //Indirect, X
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + x;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a ^= bus.read(address);
						finish();
				}
			case 0b101: //Zeropage, X
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += x;
					case 2:
						a ^= bus.read(address);
						finish();
						
				}
			case 0b110: //Absolute, Y
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + y >= 256);
						address += bus.read(pc) << 8;
						address += y;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a = bus.read(address);
						}else{
							a = bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a ^= bus.read(address);
						finish();
				}
			case 0b111: //Absolute, X
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + x >= 256);
						address += bus.read(pc) << 8;
						address += x;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a ^= bus.read(address);
						}else{
							a ^= bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a ^= bus.read(address);
						finish();
				}
			}
	}
	void INC(){
		
	}
	void INX(){
		
	}
	void INY(){
		
	}
	void JMP(){ //Jump to New Location
		//This one is special, because it disambiguates addressing modes by being called like an entirely different instruction. Sad!
		uint16_t address;
		if(curOpcode == 0x4C){ //Absolute mode
			switch(extraTicks){
				case 0:
					pc++;
					address = 0 + bus.read(pc);
				case 1:
					//??? What happens here is not well documented.
					break;
				case 2:
					bus.write(pc && 0b1111111100000000 >> 8, sp);
					sp--;
				case 3:
					bus.write(pc && 0b0000000011111111, sp);
					sp--;
					pcl = address && 0b0000000011111111;
					pch = bus.read(pc);
					pc = pch << 8;
					pc += pcl;
					finish();	
			}
		}else{ //Indirect mode
			uint8_t latch;
			switch(extraTicks){
				case 0:
					pc++;
					address = 0 + bus.read(pc);
				case 1:
					pc++;
					address += bus.read(pc) << 8;
				case 2:
					latch = bus.read(address);
				case 3:
					pcl = latch;
					pc = (pch << 8) + latch;
					finish();
			}
		}
	}
	void JSR(){ //Jump, Saving Return Address
		uint16_t address;
		switch(extraTicks){
			case 0:
				pc++;
				address = 0 + bus.read(pc);
			case 1:
				//???
				break;
			case 2:
				bus.write(pc && 0b1111111100000000 >> 8, sp);
				sp--;
			case 3:
				bus.write(pc && 0b0000000011111111, sp);
				sp--;
				pcl = address && 0b0000000011111111;
				pch = bus.read(pc);
				pc = pch << 8;
				pc += pcl;
				finish();	
			}
	}
	void LDA(){ //Load Accumulator with Memory
		uint16_t address;
		uint16_t pointer;
		bool carry = false; //Checks if we're crossing the page boundary.
		switch(addressMode){
			case 0b000: //Indirect, Y
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + y;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a = bus.read(address);
						finish();
				}
			case 0b001: //Zeropage
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						a = bus.read(address);
						finish();
				}
			case 0b010: //Immediate
				switch(extraTicks){
					case 0:
						a = bus.read(pc);
						finish();
				}
			case 0b011: //Absolute
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += bus.read(pc) << 8;
					case 2:
						a = bus.read(address);
						finish();
				}
			case 0b100: //Indirect, X
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + x;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a = bus.read(address);
						finish();
				}
			case 0b101: //Zeropage, X
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += x;
					case 2:
						a = bus.read(address);
						finish();
						
				}
			case 0b110: //Absolute, Y
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + y >= 256);
						address += bus.read(pc) << 8;
						address += y;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a = bus.read(address);
						}else{
							a = bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a = bus.read(address);
						finish();
				}
			case 0b111: //Absolute, X
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + x >= 256);
						address += bus.read(pc) << 8;
						address += x;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a = bus.read(address);
						}else{
							a = bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a = bus.read(address);
						finish();
				}
			}
	}
	void LDX(){ //Load Index X with Memory
		uint16_t address;
		uint16_t pointer;
		bool carry = false; //Checks if we're crossing the page boundary.
		switch(addressMode){
			case 0b000: //Indirect, Y
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + y;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						x = bus.read(address);
						finish();
				}
			case 0b001: //Zeropage
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						x = bus.read(address);
						finish();
				}
			case 0b010: //Immediate
				switch(extraTicks){
					case 0:
						x = bus.read(pc);
						finish();
				}
			case 0b011: //Absolute
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += bus.read(pc) << 8;
					case 2:
						x = bus.read(address);
						finish();
				}
			case 0b110: //Absolute, Y
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + y >= 256);
						address += bus.read(pc) << 8;
						address += y;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							x = bus.read(address);
						}else{
							x = bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						x = bus.read(address);
						finish();
				}
			}
	}
	void LDY(){ //Load Index Y with Memory
		uint16_t address;
		uint16_t pointer;
		bool carry = false; //Checks if we're crossing the page boundary.
		switch(addressMode){
			case 0b000: //Indirect, X
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + x;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						y = bus.read(address);
						finish();
				}
			case 0b001: //Zeropage
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						y = bus.read(address);
						finish();
				}
			case 0b010: //Immediate
				switch(extraTicks){
					case 0:
						y = bus.read(pc);
						finish();
				}
			case 0b011: //Absolute
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += bus.read(pc) << 8;
					case 2:
						y = bus.read(address);
						finish();
				}
			case 0b110: //Absolute, X
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + x >= 256);
						address += bus.read(pc) << 8;
						address += x;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							y = bus.read(address);
						}else{
							y = bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						y = bus.read(address);
						finish();
				}
			}
	}
	void LSR(){
		
	}
	void NOP(){
		
	}
	void ORA(){ //OR Memory with Accumulator
		uint16_t address;
		uint16_t pointer;
		bool carry = false; //Checks if we're crossing the page boundary.
		switch(addressMode){
			case 0b000: //Indirect, Y
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + y;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a |= bus.read(address);
						finish();
				}
			case 0b001: //Zeropage
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						a |= bus.read(address);
						finish();
				}
			case 0b010: //Immediate
				switch(extraTicks){
					case 0:
						a |= bus.read(pc);
						finish();
				}
			case 0b011: //Absolute
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += bus.read(pc) << 8;
					case 2:
						a |= bus.read(address);
						finish();
				}
			case 0b100: //Indirect, X
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + x;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a |= bus.read(address);
						finish();
				}
			case 0b101: //Zeropage, X
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += x;
					case 2:
						a |= bus.read(address);
						finish();
				}
			case 0b110: //Absolute, Y
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + y >= 256);
						address += bus.read(pc) << 8;
						address += y;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a |= bus.read(address);
						}else{
							a |= bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a |= bus.read(address);
						finish();
				}
			case 0b111: //Absolute, X
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + x >= 256);
						address += bus.read(pc) << 8;
						address += x;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a |= bus.read(address);
						}else{
							a |= bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a |= bus.read(address);
						finish();
				}
			}
	}
	void PHA(){ //Push Accumulator on Stack
		switch(extraTicks){
			case 0:
				dummy = bus.read(pc); //Dummy read. We have to do it anyway to know when the bus is busy.
				pc++;
			case 1:
				bus.write(sp, a);
				sp--;
				finish();
			}
	}
	void PHP(){ //Push Processor Status on Stack
		switch(extraTicks){
			case 0:
				dummy = bus.read(pc); //Dummy read. We have to do it anyway to know when the bus is busy.
				pc++;
			case 1:
				bus.write(sp, sr);
				sp--;
				finish();
			}
	}
	void PLA(){ //Pull Accumulator from Stack
		switch(extraTicks){
			case 0:
				dummy = bus.read(pc); //Dummy read. We have to do it anyway to know when the bus is busy.
				pc++;
			case 1:
				sp++;
			case 2:
				a = bus.read(sp);
				finish();
			}
	}
	void PLP(){ //Pull Processor Status from Stack
		switch(extraTicks){
			case 0:
				dummy = bus.read(pc); //Dummy read. We have to do it anyway to know when the bus is busy.
				pc++;
			case 1:
				sp++;
			case 2:
				sr = bus.read(sp);
				finish();
			}
	}
	void ROL(){
		
	}
	void ROR(){
		
	}
	void RTI(){ //ReTurn from Interrupt
		switch(extraTicks){
			case 0:
				dummy = bus.read(pc); //Dummy read. We have to do it anyway to know when the bus is busy.
				pc++;
			case 1:
				sp++;
			case 2:
				sr = bus.read(sp);
				sp++;
			case 3:
				pcl = bus.read(sp);
				sp++;
			case 4:
				pch = bus.read(sp);
				pc = pch << 8;
				pc += pcl;
				finish();
			}
	}
	void RTS(){ //ReTurn from Subroutine
		switch(extraTicks){
			case 0:
				dummy = bus.read(pc); //Dummy read. We have to do it anyway to know when the bus is busy.
			case 1:
				sp++;
			case 2:
				pcl = bus.read(sp);
				sp++;
			case 3:
				pch = bus.read(sp);
				pc = pch << 8;
				pc += pcl;
			case 4:
				pc++;
				finish();
			}
		
	}
	void SBC(){ //Subtract Memory from Accumulator with Borrow
		uint16_t address;
		uint16_t pointer;
		bool carry = false; //Checks if we're crossing the page boundary.
		switch(addressMode){
			case 0b000: //Indirect, Y
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + y;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a -= bus.read(address);
						finish();
				}
			case 0b001: //Zeropage
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						a -= bus.read(address);
						finish();
				}
			case 0b010: //Immediate
				switch(extraTicks){
					case 0:
						a -= bus.read(pc);
						finish();
				}
			case 0b011: //Absolute
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += bus.read(pc) << 8;
					case 2:
						a -= bus.read(address);
						finish();
				}
			case 0b100: //Indirect, X
				switch(extraTicks){
					case 0:
						pointer = bus.read(pc);
						pc++;
					case 1:
						pointer = bus.read(pointer) + x;
					case 2:
						address = bus.read(pointer);
					case 3:
						address += bus.read(pointer+1) << 8;
					case 4:
						a -= bus.read(address);
						finish();
				}
			case 0b101: //Zeropage, X
				switch(extraTicks){
					case 0:
						address = bus.read(pc);
						pc++;
					case 1:
						address += x;
					case 2:
						a -= bus.read(address);
						finish();
				}
			case 0b110: //Absolute, Y
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + y >= 256);
						address += bus.read(pc) << 8;
						address += y;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a -= bus.read(address);
						}else{
							a -= bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a -= bus.read(address);
						finish();
				}
			case 0b111: //Absolute, X
				switch(extraTicks){
					case 0:
						address = 0 + bus.read(pc);
						pc++;
					case 1:
						carry = (address + x >= 256);
						address += bus.read(pc) << 8;
						address += x;
						pc++;
					case 2:
						if(carry){
							address -= 256;
							a -= bus.read(address);
						}else{
							a -= bus.read(address);
							finish();
						}
					case 3:
						setFlag('c', true);
						a -= bus.read(address);
						finish();
				}
			}
	}
	void SEC(){ //Set Carry Flag
		setFlag('c', true);
		finish();
	}
	void SED(){ //Set Decimal Flag
		setFlag('d', true);
		finish();
	}
	void SEI(){ //Set Interrupt Disable Status
		setFlag('i', true);
		finish();
	}
	void STA(){
	
	}
	void STX(){
		
	}
	void STY(){
		
	}
	void TAX(){
		
	}
	void TAY(){
		
	}
	void TSX(){
		
	}
	void TXA(){
		
	}
	void TXS(){
		
	}
	void TYA(){
		
	}

	bool debugMode = false; //Prints CPU state after every instruction.
	
	void fetch(){
		curOpcode = bus.read(pc);
		addressMode = (curOpcode && 0b00011100) >> 2;
		pch = pc && 0b1111111100000000 >> 8;
		pcl = pc && 0b0000000011111111;
		if(debugMode){
	
		}
	}
	
	public: Mos6502_nmos(uint16_t stack, bool debug){
		stack = stackOffset;
		debugMode = debug;
	}

	void tick(){
		if(extraTicks == 0 && newCycle){
			pc++;
			fetch();
			newCycle = false;
		}else{
			switch(curOpcode){ //Official instruction set and complete CMOS instruction set
				case 0x00: BRK(); break;
				case 0x01: ORA(); break;
				case 0x05: ORA(); break;
				case 0x06: ASL(); break;
				case 0x08: PHP(); break;
				case 0x09: ORA(); break;
				case 0x0A: ASL(); break;
				case 0x0D: ORA(); break;
				case 0x0E: ASL(); break;
				case 0x10: BPL(); break;
				case 0x11: ORA(); break;
				case 0x15: ORA(); break;
				case 0x16: ASL(); break;
				case 0x18: CLC(); break;
				case 0x19: ORA(); break;
				case 0x1D: ORA(); break;
				case 0x1E: ASL(); break;
				case 0x20: JSR(); break;
				case 0x21: AND(); break;
				case 0x24: BIT(); break;
				case 0x25: AND(); break;
				case 0x26: ROL(); break;
				case 0x28: PLP(); break;
				case 0x29: AND(); break;
				case 0x2A: ROL(); break;
				case 0x2C: BIT(); break;
				case 0x2D: AND(); break;
				case 0x2E: ROL(); break;
				case 0x30: BMI(); break;
				case 0x31: AND(); break;
				case 0x35: AND(); break;
				case 0x36: ROL(); break;
				case 0x38: SEC(); break;
				case 0x39: AND(); break;
				case 0x3D: AND(); break;
				case 0x3E: ROL(); break;
				case 0x40: RTI(); break;
				case 0x41: EOR(); break;
				case 0x45: EOR(); break;
				case 0x46: LSR(); break;
				case 0x48: PHA(); break;
				case 0x49: EOR(); break;
				case 0x4A: LSR(); break;
				case 0x4C: JMP(); break;
				case 0x4D: EOR(); break;
				case 0x4E: LSR(); break;
				case 0x50: BVC(); break;
				case 0x51: EOR(); break;
				case 0x55: EOR(); break;
				case 0x56: LSR(); break;
				case 0x58: CLI(); break;
				case 0x59: EOR(); break;
				case 0x5C: JMP(); break;
				case 0x5D: EOR(); break;
				case 0x5E: LSR(); break;
				case 0x60: RTS(); break;
				case 0x61: ADC(); break;
				case 0x65: ADC(); break;
				case 0x66: ROR(); break;
				case 0x68: PLA(); break;
				case 0x69: ADC(); break; //nice
				case 0x6A: ROR(); break;
				case 0x6C: JMP(); break;
				case 0x6D: ADC(); break;
				case 0x6E: ROR(); break;
				case 0x70: BVS(); break;
				case 0x71: ADC(); break;
				case 0x75: ADC(); break;
				case 0x76: ROR(); break;
				case 0x78: SEI(); break;
				case 0x79: ADC(); break;
				case 0x7D: ADC(); break;
				case 0x7E: ROR(); break;
				case 0x81: STA(); break;
				case 0x84: STY(); break;
				case 0x85: STA(); break;
				case 0x86: STX(); break;
				case 0x88: DEY(); break;
				case 0x8A: TXA(); break;
				case 0x8C: STY(); break;
				case 0x8D: STA(); break;
				case 0x8E: STX(); break;
				case 0x90: BCC(); break;
				case 0x91: STA(); break;
				case 0x94: STY(); break;
				case 0x95: STA(); break;
				case 0x96: STX(); break;
				case 0x98: TYA(); break;
				case 0x99: STA(); break;
				case 0x9A: TXS(); break;
				case 0x9D: STA(); break;
				case 0xA0: LDY(); break;
				case 0xA1: LDA(); break;
				case 0xA2: LDX(); break;
				case 0xA4: LDY(); break;
				case 0xA5: LDA(); break;
				case 0xA6: LDX(); break;
				case 0xA8: TAY(); break;
				case 0xA9: LDA(); break;
				case 0xAA: TAX(); break;
				case 0xAC: LDY(); break;
				case 0xAD: LDA(); break;
				case 0xAE: LDX(); break;
				case 0xB0: BCS(); break;
				case 0xB1: LDA(); break;
				case 0xB4: LDY(); break;
				case 0xB5: LDA(); break;
				case 0xB6: LDX(); break;
				case 0xB8: CLV(); break;
				case 0xB9: LDA(); break;
				case 0xBA: TSX(); break;
				case 0xBC: LDY(); break;
				case 0xBD: LDA(); break;
				case 0xBE: LDX(); break;
				case 0xC0: CPY(); break;
				case 0xC1: CMP(); break;
				case 0xC4: CPY(); break;
				case 0xC5: CMP(); break;
				case 0xC6: DEC(); break;
				case 0xC8: INY(); break;
				case 0xC9: CMP(); break;
				case 0xCA: DEX(); break;
				case 0xCC: CPY(); break;
				case 0xCD: CMP(); break;
				case 0xCE: DEC(); break;
				case 0xD0: BNE(); break;
				case 0xD1: CMP(); break;
				case 0xD5: CMP(); break;
				case 0xD6: DEC(); break;
				case 0xD8: CLD(); break;
				case 0xD9: CMP(); break;
				case 0xDD: CMP(); break;
				case 0xDE: DEC(); break;
				case 0xE0: CPX(); break;
				case 0xE1: SBC(); break;
				case 0xE4: CPX(); break;
				case 0xE5: SBC(); break;
				case 0xE6: INC(); break;
				case 0xE8: INX(); break;
				case 0xE9: SBC(); break;
				case 0xEA: NOP(); break;
				case 0xEC: CPX(); break;
				case 0xED: SBC(); break;
				case 0xEE: INC(); break;
				case 0xF0: BEQ(); break;
				case 0xF1: SBC(); break;
				case 0xF5: SBC(); break;
				case 0xF6: INC(); break;
				case 0xF8: SED(); break;
				case 0xF9: SBC(); break;
				case 0xFD: SBC(); break;
				case 0xFE: INC(); break;
				default:
					NOP();
					break;
			}
			if(!newCycle){
				extraTicks++;
			}
		}
	}
};
