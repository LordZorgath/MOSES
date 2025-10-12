//XO-Chip module for MOSES. Ostensibly to test multi-core functionality. Unofficially because I wanted to.
//July 17th, 2025
#include "../../module.h"

namespace Cores::Xochip{
	
	struct{
	
		private: 
		uint8_t mem[65536];
	
		public:
		uint8_t flagStore[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		uint8_t audBuffer[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		bool samples[128];
	
		void patternUpdate(){
			for(int i = 0; i < 128; ++i){
				samples[i] = (audBuffer[i/8] & (1 << (8-(i % 8))));
			}
		}
		
		void loadROM(std::vector<uint8_t> rom){
			for(int i = 0; i < rom.size(); ++i){
				mem[0x200+i] = rom[i];
			}
		uint8_t pixelFont[16*15] = {
				//Five-line font
				0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
				0x20, 0x60, 0x20, 0x20, 0x70, // 1
				0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
				0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
				0x90, 0x90, 0xF0, 0x10, 0x10, // 4
				0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
				0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
				0xF0, 0x10, 0x20, 0x40, 0x40, // 7
				0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
				0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
				0xF0, 0x90, 0xF0, 0x90, 0x90, // A
				0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
				0xF0, 0x80, 0x80, 0x80, 0xF0, // C
				0xE0, 0x90, 0x90, 0x90, 0xE0, // D
				0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
				0xF0, 0x80, 0xF0, 0x80, 0x80, // F
				//Ten-line font
				0xFF, 0xFF, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, // 0
				0x18, 0x78, 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0xFF, 0xFF, // 1
				0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // 2
				0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 3
				0xC3, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, // 4
				0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 5
				0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, // 6
				0xFF, 0xFF, 0x03, 0x03, 0x06, 0x0C, 0x18, 0x18, 0x18, 0x18, // 7
				0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, // 8
				0xFF, 0xFF, 0xC3, 0xC3, 0xFF, 0xFF, 0x03, 0x03, 0xFF, 0xFF, // 9
				0x7E, 0xFF, 0xC3, 0xC3, 0xC3, 0xFF, 0xFF, 0xC3, 0xC3, 0xC3, // A
				0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, 0xC3, 0xC3, 0xFC, 0xFC, // B
				0x3C, 0xFF, 0xC3, 0xC0, 0xC0, 0xC0, 0xC0, 0xC3, 0xFF, 0x3C, // C
				0xFC, 0xFE, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xC3, 0xFE, 0xFC, // D
				0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, // E
				0xFF, 0xFF, 0xC0, 0xC0, 0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0  // F
			};
			for(int i = 0; i < (16*15); ++i){
				mem[i] = pixelFont[i];
			}
		}

		inline uint_fast8_t read(uint16_t addr){ return mem[addr]; }

		inline uint_fast16_t read16(uint16_t addr){ return (mem[addr] << 8) | mem[(addr + 1)]; }

		inline void write(uint8_t val, uint16_t addr){ mem[addr] = val; }
		
	} bus;
	
	struct{
		//XO-Chip interpreter.
		
		private:
		float pitch = 4000.0f;
		//Register definitions
		uint8_t v[16];
		uint16_t i = 0;
		uint16_t pc = 0x200; //Program counter
		uint8_t sp = 0; //Stack pointer
		uint8_t dt = 0; //Delay timer
		uint8_t st = 0; //Sound timer

		//Variables for the interpreter
		uint8_t planeSelect = 1;
		uint_fast16_t curOpcode;
		uint16_t stack[256];
		
		public:
		bool key[16];
		bool hiresMode = false;
		bool breakpointReached = false;
		uint8_t display[128][64];
		uint8_t tempKey = 16;
		uint64_t pcBreakpoint;
		uint64_t cycles = 0;
		
		inline bool getSound(){ return (st > 0); }
		
		inline float getPitch(){ return pitch; }
		
		inline uint8_t getScreenX(){ return (hiresMode ? 128 : 64); }
		
		inline uint8_t getScreenY(){ return (hiresMode ? 64 : 32); }

		std::string returnDebugInfo(){
			std::stringstream ret;
			ret << std::hex << std::endl;
			ret << "KEY ";
			for(int i = 0; i < 16; ++i){
				if(key[i]){
					ret << +i;
				}
			}
			ret << "\n";
			ret << "PC " << pc << "\n";
			ret << "OP " << curOpcode << "\n";
			ret << "SP " << +sp << "\n";
			ret << "DT " << +dt << "\n";
			ret << "I " << i << "\n";
			ret << "PLANE " << planeSelect << "\n";
			ret << "PITCH " << pitch << "\n";
			for(int i = 0; i < 16; ++i){
				ret << "V" << i << " " << +v[i] << "\n";
			}
			return ret.str();
		}
		
		void getDebugInfo(){
			std::cout << std::hex << std::endl;
			std::cout << "KEY ";
			for(int i = 0; i < 16; ++i){
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
			std::cout << "PLANE " << planeSelect << "\n";
			std::cout << "PITCH " << pitch << "\n";
			for(int i = 0; i < 16; ++i){
				std::cout << "V" << i << " " << +v[i] << "\n";
			}
		}
		
		inline void tick(uint32_t steps){
			dt = (dt == 0) ? 0 : --dt;
			st = (st == 0) ? 0 : --st;
			for(uint32_t a = 0; a < steps; ++a){
				curOpcode = bus.read16(pc);
				pc+=2;
				switch((curOpcode >> 12)){
					case 0x0:
						switch(curOpcode){
							case 0xE0: //CLS
								for(int i = 0; i < 128*64; ++i){
									display[i%128][i/128] &= ~planeSelect;
								}
								break;
							case 0xEE: //RET
								sp--;
								pc = stack[sp];
								break;
							case 0xFB: //SCR
								{
									for(int y = 0; y < getScreenY(); y++){
										for(int x = getScreenX()-1; x >= 0; x--){
											if(x < 4){
												display[x][y] &= ~planeSelect;
											}else{
												uint8_t pixelRef = (display[x][y] & ~planeSelect);
												display[x][y] = (display[((x-4) & (getScreenX()-1))][y] & planeSelect);
												display[x][y] |= pixelRef;
											}
										}
									}
								}
								break;
							case 0xFC: //SCL
								{
									for(int y = 0; y < getScreenY(); y++){
										for(int x = 0; x < getScreenX(); x++){
											if(x >= (getScreenX()-4)){
												display[x][y] &= ~planeSelect;
											}else{
												uint8_t pixelRef = (display[x][y] & ~planeSelect);
												display[x][y] = (display[((x+4) & (getScreenX()-1))][y] & planeSelect);
												display[x][y] |= pixelRef;
											}
										}
									}
								}
								break;
							case 0xFE: //LOW
								hiresMode = false;
								for(int i = 0; i < 128*64; ++i){
									display[i%128][i/128] = 0;
								}
								break;
							case 0xFF: //HIGH
								hiresMode = true;
								for(int i = 0; i < 128*64; ++i){
									display[i%128][i/128] = 0;
								}
								break;
							default:
								{
									uint8_t offset = (curOpcode & 0xF);
									uint8_t pixelRef;
									switch((curOpcode & 0xF0)){
										case 0xC0: //SCD
											for(int y = getScreenY()-1; y >= 0; y--){
												for(int x = 0; x < getScreenX(); x++){
													if(y < offset){
														display[x][y] &= ~planeSelect;
													}else{
														pixelRef = (display[x][y] & ~planeSelect);
														display[x][y] = (display[x][(y-offset) & (getScreenY()-1)] & planeSelect);
														display[x][y] |= pixelRef;
													}
												}
											}
											break;
										case 0xD0: //SCU
											for(int y = 0; y < getScreenY(); y++){
												for(int x = 0; x < getScreenX(); x++){
													if(y >= (getScreenY()-offset)){
														display[x][y] &= ~planeSelect;
													}else{
														pixelRef = (display[x][y] & ~planeSelect);
														display[x][y] = (display[x][(y+offset) & (getScreenY()-1)] & planeSelect);
														display[x][y] |= pixelRef;
													}
												}
											}
											break;
										default:[[unlikely]]
											std::cout << "GURU MEDITATION unknown opcode\n";
											getDebugInfo();
											break;
									}
								}
								break;
						}
						break;
					case 0x1: //JP
						pc = (curOpcode & 0xFFF);
						break;
					case 0x2: //CALL
						stack[sp] = pc;
						sp++;
						pc = (curOpcode & 0xFFF);
						break;
					case 0x3: //SE
						if(v[((curOpcode >> 8) & 0xF)] == (curOpcode & 0xFF)){
							pc += (bus.read16(pc) == 0xF000) ? 4 : 2;
						}
						break;
					case 0x4: //SNE
						if(v[((curOpcode >> 8) & 0xF)] != (curOpcode & 0xFF)){
							pc += (bus.read16(pc) == 0xF000) ? 4 : 2;
						}
						break;
					case 0x5:
						{
							uint8_t regX = ((curOpcode >> 8) & 0xF);
							uint8_t regY = ((curOpcode >> 4) & 0xF);
							switch(curOpcode & 0xF){
								case 0: //SE 
									if(v[regX] == v[regY]){
										pc += (bus.read16(pc) == 0xF000) ? 4 : 2;
									}
									break;
								case 2: //LD
									if(regY > regX){
										for(int a = regX; a <= regY; ++a){
											bus.write(v[a], i+(a-regX));
										}
									}else{
										for(int a = regX; a >= regY; a--){
											bus.write(v[a], i+(regX-a));
										}
									}
									break;
								case 3: //LD
									if(regY > regX){
										for(int a = regX; a <= regY; ++a){
											v[a] = bus.read(i+(a-regX));
										}
									}else{
										for(int a = regX; a >= regY; a--){
											v[a] = bus.read(i+(regX-a));
										}
									}
									break;
								default:[[unlikely]]
									std::cout << "GURU MEDITATION unknown opcode\n";
									getDebugInfo();
									break;
							}
						}
						break;
					case 0x6: //LD
						v[((curOpcode >> 8) & 0xF)] = (curOpcode & 0xFF);
						break;
					case 0x7: //ADD
						v[((curOpcode >> 8) & 0xF)] += (curOpcode & 0xFF);
						break;
					case 0x8:
						{
							auto& regX = v[((curOpcode >> 8) & 0xF)];
							auto& regY = v[((curOpcode >> 4) & 0xF)];
							switch(curOpcode & 0xF){
								case 0x0: //LD
									regX = regY;
									break;
								case 0x1: //OR
									regX |= regY;
									break;
								case 0x2: //AND
									regX &= regY;
									break;
								case 0x3: //XOR
									regX ^= regY;
									break;
								case 0x4: //ADD
									{
										auto flagRef = (regX + regY >= 256);
										regX += regY;
										v[15] = flagRef;
									}
									break;
								case 0x5: //SUB
									{
										auto flagRef = (regX >= regY);
										regX -= regY;
										v[15] = flagRef;
									}
									break;
								case 0x6: //SHR
									{
										auto flagRef = (regY & 1);
										regX = (regY >> 1);
										v[15] = flagRef;
									}
									break;
								case 0x7: //SUBN
									{
										auto flagRef = (regY >= regX);
										regX = (regY - regX);
										v[15] = flagRef;
									}
									break;
								case 0xE: //SHL
									{
										auto flagRef = (regY >> 7);
										regX = (regY << 1);
										v[15] = flagRef;
									}
									break;
								default:[[unlikely]]
									std::cout << "GURU MEDITATION unknown opcode\n";
									getDebugInfo();
									break;
							}
						}
						break;
					case 0x9: //SNE
						if((curOpcode & 0xF) == 0){
							if(v[((curOpcode >> 8) & 0xF)] != v[((curOpcode >> 4) & 0xF)]){
								pc += (bus.read16(pc) == 0xF000) ? 4 : 2;
							}
						}
						break;
					case 0xA: //LD
						i = (curOpcode & 0xFFF);
						break;
					case 0xB: //JP
						pc = ((curOpcode & 0xFFF) + v[0]);
						break;
					case 0xC: //RND
						v[((curOpcode >> 8) & 0xF)] = (rand() & (curOpcode & 0xFF));
						break;
					case 0xD: //DRW
						{
							uint8_t posX = v[((curOpcode >> 8) & 0xF)];
							uint8_t posY = v[((curOpcode >> 4) & 0xF)];
							uint8_t spriteHeight = ((curOpcode & 0xF) == 0) ? 16 : (curOpcode & 0xF);
							uint8_t planeIterator = 0;
							uint8_t pixelRef;
							int8_t line;
							v[15] = false;
							for(int plane = 0; plane < 4; plane++){
								if((planeSelect & (1 << plane))){
									for(int y = 0; y < spriteHeight; y++){
										for(int z = 0; z <= (spriteHeight == 16); z++){
											line = bus.read(i+(y*(1+(spriteHeight == 16)))+z+((spriteHeight == 16) ? planeIterator*32 : planeIterator*spriteHeight));
											for(int x = 0; x < 8; x++){
												if(((line & (0b10000000 >> x))) && (display[(posX+x+(z*8)) & (getScreenX()-1)][(posY+y) & (getScreenY()-1)] & (1 << plane))){
													v[15] = true;
												}
												pixelRef = (display[(posX+x+(z*8)) & (getScreenX()-1)][(posY+y) & (getScreenY()-1)] & ~planeSelect);
												display[(posX+x+(z*8)) & (getScreenX()-1)][(posY+y) & (getScreenY()-1)] ^= (((line & (0b10000000 >> x)) >> (7-x)) << plane);
												display[(posX+x+(z*8)) & (getScreenX()-1)][(posY+y) & (getScreenY()-1)] |= pixelRef;
											}
										}
									}
									planeIterator++;
								}
							}
						}
						break;
					case 0xE:
						switch((curOpcode & 0xFF)){
							case 0x9E: //SKP
								if(key[v[((curOpcode >> 8) & 0xF)] & 0xF]){
									pc += (bus.read16(pc) == 0xF000) ? 4 : 2;
								}
							break;
							case 0xA1: //SKNP
								if(!key[v[((curOpcode >> 8) & 0xF)] & 0xF]){
									pc += (bus.read16(pc) == 0xF000) ? 4 : 2;
								}
							break;
							default:[[unlikely]]
								std::cout << "GURU MEDITATION unknown opcode\n";
								getDebugInfo();
							break;
						}
						break;
					case 0xF:
						if(curOpcode == 0xF000){ //LONG
							i = bus.read16(pc);
							pc+=2;
							break;
						}
						switch(curOpcode & 0xFF){
							case 0x1: //DW
								planeSelect = ((curOpcode >> 8) & 0xF);
								break;
							case 0x2: //AUDIO
								for(int a = 0; a < 16; ++a){
									bus.audBuffer[a] = bus.read(i+a);
								}
								bus.patternUpdate();
								break;
							case 0x7: //LD
								v[((curOpcode >> 8) & 0xF)] = dt;
								break;
							case 0xA: //LD
								for(int i = 0; i < 16; ++i){
									if(key[i]){
										tempKey = i;
										pc-=2;
										cycles += a;
										return;
									}else if(i == 15){
										if(tempKey != 16){
											v[((curOpcode >> 8) & 0xF)] = tempKey;
											tempKey = 16;
											break;
										}else{
											pc-=2;
											cycles += a;
											return;
										}
									}
								}
								break;
							case 0x15: //LD
								dt = v[((curOpcode >> 8) & 0xF)];
								break;
							case 0x18: //LD
								st = v[((curOpcode >> 8) & 0xF)];
								break;
							case 0x1E: //ADD
								i += v[((curOpcode >> 8) & 0xF)];
								break;
							case 0x29: //LD
								i = (v[((curOpcode >> 8) & 0xF)] & 0xF) * 5;
								break;
							case 0x30:
								i = (16*5) + (v[((curOpcode >> 8) & 0xF)] & 0xF) * 10;
								break;
							case 0x33: //LD
								{
									auto& num = v[((curOpcode >> 8) & 0xF)];
									bus.write(num / 100, i);
									bus.write(num / 10 % 10, i+1);
									bus.write(num % 10, i+2);
								}
								break;
							case 0x3A: //PITCH
								pitch = 4000.0f*pow(2.0f, ((v[((curOpcode >> 8) & 0xF)]-64.0f)/48.0f));
								break;
							case 0x55: //LD
								for(int a = 0; a <= ((curOpcode >> 8) & 0xF); ++a){
									bus.write(v[a], i);
									++i;
								}
								break;
							case 0x65: //LD
								for(int a = 0; a <= ((curOpcode >> 8) & 0xF); ++a){
									v[a] = bus.read(i);
									++i;
								}
								break;
							case 0x75: //LD
								for(int a = 0; a <= ((curOpcode >> 8) & 0xF); ++a){
									bus.flagStore[a] = v[a];
								}
								break;
							case 0x85: //LD
								for(int a = 0; a <= ((curOpcode >> 8) & 0xF); ++a){
									v[a] = bus.flagStore[a];
								}
								break;
							default:[[unlikely]]
								std::cout << "GURU MEDITATION unknown opcode\n";
								getDebugInfo();
								break;
						}
						break;
						default:[[unlikely]]
							std::cout << "GURU MEDITATION unknown opcode\n";
							getDebugInfo();
							break;
				}
			}
			cycles += steps;
		}
		
		std::string loggedTick(uint32_t steps){
			std::stringstream ret;
			for(int a = 0; a < steps; ++a){
				ret << std::hex << std::setfill('0') << "[" << std::setw(8) << +cycles << "] ";
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
			}
			return ret.str();
		}
		
		void breakpointTick(uint32_t steps){
			for(int a = 0; a < steps; ++a){
				if(!breakpointReached){
					tick(1);
					if(pc == pcBreakpoint){
						std::cout << "BREAKPOINT REACHED\n";
						std::cout << "TICKS " << +cycles;
						getDebugInfo();
						breakpointReached = true;
						return;
					}
				}else{
					return;
				}
			}
		}
	} cpu;

	class System:public Module{
		private:
		uint32_t color[16] = { //Reminder to implement custom palettes!
			0xFF000000,
			0xFFFFFFFF,
			0xFFAAAAAA,
			0xFF555555,
			0xFFFF0000,
			0xFF00FF00,
			0xFF0000FF,
			0xFFFFFF00,
			0xFF880000,
			0xFF008800,
			0xFF000088,
			0xFF888800,
			0xFFFF00FF,
			0xFF00FFFF,
			0xFF880088,
			0xFF008888
		};
		uint64_t framesTicked = 0;
		
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
		
		int16_t& getAudioBuffer() override{
			uint32_t sampleFreq = getSampleFrequency();
			double targetFPS = getFPS();
			if(cpu.getSound()){
				double stepSize = (cpu.getPitch()/sampleFreq);
				audioPhase %= sampleFreq;
				for(int i = audioPhase; i < (audioPhase + (sampleFreq/targetFPS)); ++i){
					audioBuffer[(i-audioPhase)] = volume * (bus.samples[(uint32_t)(i*stepSize)&127] ? 32767 : -32767);
				}
				audioPhase += (sampleFreq/targetFPS);
			}else{
				for(uint32_t i = 0; i < (sampleFreq/targetFPS); ++i){
					audioBuffer[i] = 0;
				}
				audioPhase = 0;
			}
			return *audioBuffer.data();
		}
		
		uint32_t& getFrameBuffer() override{
			if(cpu.hiresMode){
				for(int y = 0; y < 64; y++){
					for(int x = 0; x < 128; x++){
						frameBuffer[((y*128)+x)] = color[(cpu.display[x][y] & 0xF)];
					}
				}
			}else{
				for(int y = 0; y < 64; y++){
					for(int x = 0; x < 64; x++){
						frameBuffer[2*((y*64)+x)] =  color[(cpu.display[x][y/2] & 0xF)];
						frameBuffer[2*((y*64)+x)+1] =  color[(cpu.display[x][y/2] & 0xF)];
					}
				}
			}
			return *frameBuffer.data();
		}

		void runCycle() override{
			getKey();
			cpu.tick(bclk);
		}
		
		void debugCycle() override{
			getKey();
			cpu.pcBreakpoint = pcBreakpoint;
			if(doWriteLog){
				writeLogToFile(cpu.loggedTick(debugStep));
			}else{
				if(breakpointActive){
					cpu.breakpointTick(debugStep);
					framesTicked++;
				}else{
					if(!cpu.breakpointReached){
						cpu.tick(debugStep);
						cpu.getDebugInfo();
					}
				}
			}
		}

		uint64_t getCycles() const override {
			return cpu.cycles;
		}

		System(std::map<std::string, std::string> args):Module("XO-Chip", 1000, 128, 64, 1, 48000, 60.0){
			for(auto& [key, value] : args){
				if(key == "fast"){
					bclk = 200000;
				}
				if(key == "speed"){
					bclk = std::stoi(value);
				}
			}
			bus.loadROM(readFile(args.at("file"), 65024));
		}
	};
}
