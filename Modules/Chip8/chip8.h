//Chip-8 module for MOSES. Mostly for testing UI, graphics, sound and the like.
//Thursday 26th June, 2025
#include "../../module.h"

namespace Cores::Chip8{
	
	struct{
	
		private: 
		uint8_t mem[4096];
	
		public:
		void loadROM(std::vector<uint8_t> rom){
			for(uint i = 0; i < rom.size(); i++){
				mem[0x200+i] = rom[i];
			}
		}
		
		void setup(){
			mem[0] = 0xF0;
			mem[1] = 0x90;
			mem[2] = 0x90;
			mem[3] = 0x90;
			mem[4] = 0xF0;
			//Sprite for 0
			mem[5] = 0x20;
			mem[6] = 0x60;
			mem[7] = 0x20;
			mem[8] = 0x20;
			mem[9] = 0x70;
			//Sprite for 1
			mem[10] = 0xF0;
			mem[11] = 0x10;
			mem[12] = 0xF0;
			mem[13] = 0x80;
			mem[14] = 0xF0;
			//Sprite for 2
			mem[15] = 0xF0;
			mem[16] = 0x10;
			mem[17] = 0xF0;
			mem[18] = 0x10;
			mem[19] = 0xF0;
			//Sprite for 3
			mem[20] = 0x90;
			mem[21] = 0x90;
			mem[22] = 0xF0;
			mem[23] = 0x10;
			mem[24] = 0x10;
			//Sprite for 4
			mem[25] = 0xF0;
			mem[26] = 0x80;
			mem[27] = 0xF0;
			mem[28] = 0x10;
			mem[29] = 0xF0;
			//Sprite for 5
			mem[30] = 0xF0;
			mem[31] = 0x80;
			mem[32] = 0xF0;
			mem[33] = 0x90;
			mem[34] = 0xF0;
			//Sprite for 6
			mem[35] = 0xF0;
			mem[36] = 0x10;
			mem[37] = 0x20;
			mem[38] = 0x40;
			mem[39] = 0x40;
			//Sprite for 7
			mem[40] = 0xF0;
			mem[41] = 0x90;
			mem[42] = 0xF0;
			mem[43] = 0x90;
			mem[44] = 0xF0;
			//Sprite for 8
			mem[45] = 0xF0;
			mem[46] = 0x90;
			mem[47] = 0xF0;
			mem[48] = 0x10;
			mem[49] = 0xF0;
			//Sprite for 9
			mem[50] = 0xF0;
			mem[51] = 0x90;
			mem[52] = 0xF0;
			mem[53] = 0x90;
			mem[54] = 0x90;
			//Sprite for A
			mem[55] = 0xE0;
			mem[56] = 0x90;
			mem[57] = 0xE0;
			mem[58] = 0x90;
			mem[59] = 0xE0;
			//Sprite for B
			mem[60] = 0xF0;
			mem[61] = 0x80;
			mem[62] = 0x80;
			mem[63] = 0x80;
			mem[64] = 0xF0;
			//Sprite for C
			mem[65] = 0xE0;
			mem[66] = 0x90;
			mem[67] = 0x90;
			mem[68] = 0x90;
			mem[69] = 0xE0;
			//Sprite for D
			mem[70] = 0xF0;
			mem[71] = 0x80;
			mem[72] = 0xF0;
			mem[73] = 0x80;
			mem[74] = 0xF0;
			//Sprite for E
			mem[75] = 0xF0;
			mem[76] = 0x80;
			mem[77] = 0xF0;
			mem[78] = 0x80;
			mem[79] = 0x80;
			//Sprite for F
		}

		uint8_t read(uint16_t addr){
			if(addr > 4095){
				std::cout << "GURU MEDITATION mem out of bounds read\n";
				return 0;
			}else{
				return mem[addr];
			}
		}
		
		uint16_t readOpcode(uint16_t addr){
			return (mem[(addr & 4095)] << 8) + mem[((addr+1) & 4095)];
		}
		
		void write(uint8_t val, uint16_t addr){
			if(addr > 4095){
				std::cout << "GURU MEDITATION mem out of bounds write\n";
			}else{
				mem[addr] = val;
			}
		}
	} bus;
	
	struct{
		//Chip-8 interpreter. JIT when?
		
		private:
		//Register definitions
		uint8_t v[16];
		uint16_t i = 0;
		uint16_t pc = 0x200; //Program counter
		uint8_t sp = 0; //Stack pointer
		uint8_t dt = 0; //Delay timer
		uint8_t st = 0; //Sound timer

		//Variables for the interpreter
		uint16_t curOpcode;
		uint16_t curOpcodeMSB;
		uint16_t stack[12];
		uint64_t loggedTicks = 0;
		
		public:
		bool key[16];
		uint8_t tempKey = 16;
		bool release = true;
		bool display[64][32];
		bool displayWait = true;
		
		bool getSound(){
			return (st > 0);
		}
		
		void decTimers(){
			if(dt > 0){
				dt--;
			}
			if(st > 0){
				st--;
			}
		}
		
		void getDebugInfo(){
			std::cout << std::hex << std::endl;
			std::cout << "KEY ";
			for(int i = 0; i < 16; i++){
				if(key[i]){
					std::cout << +i;
				}
			}
			std::cout << "\n";
			std::cout << "PC " << pc << "\n";
			std::cout << "OP " << curOpcode << "\n";
			std::cout << "SP " << +sp << "\n";
			std::cout << "DT " << +dt << "\n";
			std::cout << "I " << i << "\n";
			for(int i = 0; i < 16; i++){
				std::cout << "V" << i << " " << +v[i] << "\n";
			}
		}
		
		inline void tick(uint32_t steps){
			uint8_t refA;
			uint8_t refB;
			uint8_t flagRef;
			for(uint32_t a = 0; a < steps; a++){
				curOpcode = bus.readOpcode(pc);
				curOpcodeMSB = (curOpcode & 0xF000) >> 12;
				pc+=2;
				switch(curOpcodeMSB){
					case 0x00:
						switch(curOpcode){
							case 0x00E0: //CLS
								for(int i = 0; i < 64*32; i++){
									display[i%64][i/64] = false;
								}
								break;
							case 0x00EE: //RET
								if(sp == 0){
									std::cout << "GURU MEDITATION return outside of subroutine\n";
								}else{
									sp--;
									pc = stack[sp];
								}
								break;
							default:
								std::cout << "GURU MEDITATION unknown opcode\n";
								getDebugInfo();
								break;
						}
						break;
					case 0x01: //JP
						pc = (curOpcode & 0x0FFF);
						break;
					case 0x02: //CALL
						if(sp > 11){
							std::cout << "GURU MEDITATION too many nested subroutines\n";
						}else{
							stack[sp] = pc;
							sp++;
							pc = (curOpcode & 0x0FFF);
						}
						break;
					case 0x03: //SE
						if(v[((curOpcode & 0x0F00) >> 8)] == (curOpcode & 0x00FF)){
							pc+=2;
						}
						break;
					case 0x04: //SNE
						if(v[((curOpcode & 0x0F00) >> 8)] != (curOpcode & 0x00FF)){
							pc+=2;
						}
						break;
					case 0x05: //SE 
						if((curOpcode & 0x000F) == 0){
							if(v[((curOpcode & 0x0F00) >> 8)] == v[((curOpcode & 0x00F0) >> 4)]){
								pc+=2;
							}
						}else{
							std::cout << "GURU MEDITATION unknown opcode\n";
						}
						break;
					case 0x06: //LD
						v[((curOpcode & 0x0F00) >> 8)] = (curOpcode & 0x00FF);
						break;
					case 0x07: //ADD
						v[((curOpcode & 0x0F00) >> 8)] += (curOpcode & 0x00FF);
						break;
					case 0x08: 
						switch(curOpcode & 0x000F){
							case 0x0: //LD
								v[((curOpcode & 0x0F00) >> 8)] = v[((curOpcode & 0x00F0) >> 4)];
								break;
							case 0x1: //OR
								v[((curOpcode & 0x0F00) >> 8)] |= v[((curOpcode & 0x00F0) >> 4)];
								v[15] = false;
								break;
							case 0x2: //AND
								v[((curOpcode & 0x0F00) >> 8)] &= v[((curOpcode & 0x00F0) >> 4)];
								v[15] = false;
								break;
							case 0x3: //XOR
								v[((curOpcode & 0x0F00) >> 8)] ^= v[((curOpcode & 0x00F0) >> 4)];
								v[15] = false;
								break;
							case 0x4: //ADD
								flagRef = (v[((curOpcode & 0x0F00) >> 8)] + v[((curOpcode & 0x00F0) >> 4)] >= 256);
								v[((curOpcode & 0x0F00) >> 8)] += v[((curOpcode & 0x00F0) >> 4)];
								v[15] = flagRef;
								break;
							case 0x5: //SUB
								flagRef = (v[((curOpcode & 0x0F00) >> 8)] >= v[((curOpcode & 0x00F0) >> 4)]);
								v[((curOpcode & 0x0F00) >> 8)] -= v[((curOpcode & 0x00F0) >> 4)];
								v[15] = flagRef;
								break;
							case 0x6: //SHR
								flagRef = (v[((curOpcode & 0x00F0) >> 4)] & 0b00000001);
								v[((curOpcode & 0x0F00) >> 8)] = (v[((curOpcode & 0x00F0) >> 4)] >> 1);
								v[15] = flagRef;
								break;
							case 0x7: //SUBN
								flagRef = (v[((curOpcode & 0x00F0) >> 4)] >= v[((curOpcode & 0x0F00) >> 8)]);
								v[((curOpcode & 0x0F00) >> 8)] = (v[((curOpcode & 0x00F0) >> 4)] - v[((curOpcode & 0x0F00) >> 8)]);
								v[15] = flagRef;
								break;
							case 0xE: //SHL
								flagRef = (v[((curOpcode & 0x00F0) >> 4)] >> 7);
								v[((curOpcode & 0x0F00) >> 8)] = (v[((curOpcode & 0x00F0) >> 4)] << 1);
								v[15] = flagRef;
								break;
							default:
								std::cout << "GURU MEDITATION unknown opcode\n";
								getDebugInfo();
								break;
						}
						break;
					case 0x09: //SNE
						if((curOpcode & 0x000F) == 0){
							if(v[((curOpcode & 0x0F00) >> 8)] != v[((curOpcode & 0x00F0) >> 4)]){
								pc+=2;
							}
						}
						break;
					case 0x0A: //LD
						i = (curOpcode & 0x0FFF);
						break;
					case 0x0B: //JP
						pc = ((curOpcode & 0x0FFF) + v[0]);
						break;
					case 0x0C: //RND
						v[((curOpcode & 0x0F00) >> 8)] = (rand() & (curOpcode & 0x00FF));
						break;
					case 0x0D: //DRW
						refA = v[((curOpcode & 0x0F00) >> 8)] & 63;
						refB = v[((curOpcode & 0x00F0) >> 4)] & 31;
						v[15] = false;
						for(int y = 0; y < (curOpcode & 0x000F); y++){
							if((y + refB) > 31){
								break;
							}
							flagRef = bus.read(i+y);
							for(int x = 0; x < 8; x++){
								if((x + refA) > 63){
									break;
								}
								if(flagRef & (0b10000000 >> x) && display[(refA+x)][(refB+y)]){
									v[15] = true;
								}
								display[(refA+x)][(refB+y)] ^= (flagRef & (0b10000000 >> x)) >> (7-x);
							}
						}
						if(displayWait){
							return;
						}
						break;
					case 0x0E:
						switch((curOpcode & 0x00FF)){
							case 0x9E: //SKP
								if(key[v[((curOpcode & 0x0F00) >> 8)] & 0x0F]){
									pc+=2;
								}
							break;
							case 0xA1: //SKNP
								if(!key[v[((curOpcode & 0x0F00) >> 8)] & 0x0F]){
									pc+=2;
								}
							break;
							default:
								std::cout << "GURU MEDITATION unknown opcode\n";
								getDebugInfo();
							break;
						}
						break;
					case 0x0F:
						switch(curOpcode & 0x00FF){
							case 0x07: //LD
								v[(curOpcode & 0x0F00) >> 8] = dt;
								break;
							case 0x0A: //LD
								if(!release){
									for(int i = 0; i < 16; i++){
										if(key[i]){
											tempKey = i;
											break;
										}
									}
									pc-=2;
								}else{
									if(tempKey == 16){
										pc-=2;
									}else{
										v[((curOpcode & 0x0F00) >> 8)] = tempKey;
										tempKey = 16;
									}
								}
								break;
							case 0x15: //LD
								dt = v[((curOpcode & 0x0F00) >> 8)];
								break;
							case 0x18: //LD
								st = v[((curOpcode & 0x0F00) >> 8)];
								break;
							case 0x1E: //ADD
								i += v[((curOpcode & 0x0F00) >> 8)];
								break;
							case 0x29: //LD
								i = (v[((curOpcode & 0x0F00) >> 8)] & 0x0F) * 5;
								break;
							case 0x33: //LD
								refA = v[((curOpcode & 0x0F00) >> 8)];
								bus.write(refA / 100, i);
								bus.write(refA / 10 % 10, i+1);
								bus.write(refA % 10, i+2);
								break;
							case 0x55: //LD
								for(int a = 0; a <= ((curOpcode & 0x0F00) >> 8); a++){
									bus.write(v[a], i);
									i++;
								}
								break;
							case 0x65: //LD
								for(int a = 0; a <= ((curOpcode & 0x0F00) >> 8); a++){
									v[a] = bus.read(i);
									i++;
								}
								break;
							default:
								std::cout << "GURU MEDITATION unknown opcode\n";
								getDebugInfo();
								break;
						}
						break;
						default:
							std::cout << "GURU MEDITATION unknown opcode\n";
							getDebugInfo();
							break;
				}
			}
		}
		
		inline std::string loggedTick(uint32_t steps){
			std::stringstream ret;
			for(int a = 0; a < steps; a++){
				ret << std::hex << std::setfill('0') << "[" << std::setw(8) << +loggedTicks << "] ";
				for(int b = 0; b < 16; b++){
					ret << std::setw(1);
					switch(b){ //This is really dumb. Whatever.
						case 10:
							ret << "VA";
							break;
						case 11:
							ret << "VB";
							break;
						case 12: 
							ret << "VC";
							break;
						case 13:
							ret << "VD";
							break;
						case 14:
							ret << "VE";
							break;
						case 15:
							ret << "VF";
							break;
						default:
							ret << "V" << +b;
						break;
					}
					ret << ":" << std::setw(2) << +v[b] << " ";
				}
				ret << "I:" << std::setw(4) << +i << " " << std::setw(1);
				ret << "SP:" << +sp << " ";
				ret << "PC:" << std::setw(4) << +pc << " ";
				tick(1);
				ret << "O:" << std::setw(4) << +curOpcode << "\n";
				loggedTicks++;
			}
			return ret.str();
		}
	} cpu;

	class System:public Module{
		private:
		float freq = 440 * 2 * M_PI;
		
		void drawFrame(){
			for(int y = 0; y < 32; y++){
				for(int x = 0; x < 64; x++){
					if(cpu.display[x][y]){
						frameBuffer[((y*64)+x)] = 0xFFFFFFFF;
					}else{
						frameBuffer[((y*64)+x)] = 0xFF000000;
					}
				}
			}
		}
		
		void getKey() override{
			cpu.key[0] = keyCodes[SDL_SCANCODE_X];
			cpu.key[1] = keyCodes[SDL_SCANCODE_1];
			cpu.key[2] = keyCodes[SDL_SCANCODE_2];
			cpu.key[3] = keyCodes[SDL_SCANCODE_3];
			cpu.key[4] = keyCodes[SDL_SCANCODE_Q];
			cpu.key[5] = keyCodes[SDL_SCANCODE_W];
			cpu.key[6] = keyCodes[SDL_SCANCODE_E];
			cpu.key[7] = keyCodes[SDL_SCANCODE_A];
			cpu.key[8] = keyCodes[SDL_SCANCODE_S];
			cpu.key[9] = keyCodes[SDL_SCANCODE_D];
			cpu.key[10] = keyCodes[SDL_SCANCODE_Z];
			cpu.key[11] = keyCodes[SDL_SCANCODE_C];
			cpu.key[12] = keyCodes[SDL_SCANCODE_4];
			cpu.key[13] = keyCodes[SDL_SCANCODE_R];
			cpu.key[14] = keyCodes[SDL_SCANCODE_F];
			cpu.key[15] = keyCodes[SDL_SCANCODE_V];
		}
		
		public:
		
		int16_t* playAudio() override{
			uint32_t sampleFreq = winArgs -> getSampleFrequency();
			double targetFPS = winArgs -> getFPS();
			audioPhase %= sampleFreq;
			if(cpu.getSound()){
				for(int i = audioPhase; i < (audioPhase + (sampleFreq/targetFPS)); i++){
					double time = i/(double)sampleFreq;
					audioSamples[(i-audioPhase)] = std::sin(freq*time)*(volume * 32767);
				}
				audioPhase += (sampleFreq/targetFPS);
			}else{
				for(int i = 0; i < (sampleFreq/targetFPS); i++){
					audioSamples[i] = 0;
				}
				audioPhase = 0;
			}
			return audioSamples;
		}
		
		void runCycle() override{
			getKey();
			cpu.release = keyRelease;
			cpu.tick(bclk);
			drawFrame();
			cpu.decTimers();
		}
		
		void debugCycle() override{
			getKey();
			cpu.release = keyRelease;
			if(doWriteLog){
				writeLogToFile(cpu.loggedTick(debugStep));
			}else{
				cpu.tick(debugStep);
			}
			drawFrame();
			cpu.decTimers();
			cpu.getDebugInfo();
		}
		
		System(int argc, std::string* args):Module("Chip-8", 16, 64, 32, 1, 1800, 60.0){
			frameBuffer.resize(64*32);
			bool fileArg = false;
			for(int i = 0; i < argc; i++){
				if(args[i] == "-f"){
					fileArg = true;
					bus.loadROM(readFile(args[i+1]));
				}
				if(args[i] == "-sp"){
					if(std::stoi(args[i+1]) < 1){
						std::cout << "GURU MEDITATION invalid ipf setting\n";
					}else{
						bclk = std::stoi(args[i+1]);
					}
				}
				if(args[i] == "--nodisplaywait"){
					cpu.displayWait = false;
				}
			}
			if(!fileArg){
				std::cout << "GURU MEDITATION no file argument\n";
			}
			if(fileFound){
				init = true;
				bus.setup();
			}
		}
	};
}
