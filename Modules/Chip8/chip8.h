//Chip-8 module for MOSES. Mostly for testing UI, graphics, sound and the like.
//Thursday 26th June, 2025
#include "../../module.h"

namespace Cores::Chip8{
	
	struct{
	
		private: 
		uint8_t mem[4096];
	
		public:
		void loadROM(std::vector<uint8_t> rom){
			for(int i = 0; i < rom.size(); ++i){
				mem[0x200+i] = rom[i];
			}
			uint8_t pixelFont[16*5] = {
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
				0xF0, 0x80, 0xF0, 0x80, 0x80  // F
			};
			for(int i = 0; i < (16*5); ++i){
				mem[i] = pixelFont[i];
			}
		}

		inline uint_fast8_t read(uint16_t addr){ return mem[(addr % 4096)]; }
		
		inline uint_fast16_t readOpcode(uint16_t addr){ return (mem[(addr % 4096)] << 8) | mem[((addr+1) % 4096)]; }
		
		inline void write(uint8_t val, uint16_t addr){ mem[(addr % 4096)] = val; }
		
	} bus;
	
	struct{
		//Chip-8 interpreter. JIT when?
		
		private:
		//Register definitions
		uint16_t i = 0;
		uint16_t pc = 0x200; //Program counter
		uint8_t v[16]; //Registers
		uint8_t sp = 0; //Stack pointer
		uint8_t dt = 0; //Delay timer
		uint8_t st = 0; //Sound timer

		//Variables for the interpreter
		uint_fast16_t curOpcode;
		uint16_t stack[256];

		public:
		bool displayWait = true;
		bool key[16];
		uint8_t tempKey = 16;
		uint8_t display[64][32];
		uint64_t cycles = 0;

		inline bool getSound(){ return (st > 0); }
		
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
			for(int i = 0; i < 16; ++i){
				std::cout << "V" << i << " " << +v[i] << "\n";
			}
		}
		
		inline void tick(uint32_t steps){
			dt = (dt == 0) ? 0 : --dt;
			st = (st == 0) ? 0 : --st;
			for(uint32_t a = 0; a < steps; ++a){
				curOpcode = bus.readOpcode(pc);
				pc+=2;
				switch((curOpcode >> 12)){
					case 0x0:
						switch(curOpcode){
							case 0xE0: //CLS
								for(int i = 0; i < 64*32; ++i){
									display[i%64][i/64] = false;
								}
								break;
							case 0xEE: //RET
								sp--;
								pc = stack[sp];
								break;
							default:[[unlikely]]
								std::cout << "GURU MEDITATION unknown opcode\n";
								getDebugInfo();
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
							pc+=2;
						}
						break;
					case 0x4: //SNE
						if(v[((curOpcode >> 8) & 0xF)] != (curOpcode & 0xFF)){
							pc+=2;
						}
						break;
					case 0x5: //SE 
						if((curOpcode & 0xF) == 0){
							if(v[((curOpcode >> 8) & 0xF)] == v[((curOpcode >> 4) & 0xF)]){
								pc+=2;
							}
						}else{
							std::cout << "GURU MEDITATION unknown opcode\n";
							getDebugInfo();
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
									v[15] = false;
									break;
								case 0x2: //AND
									regX &= regY;
									v[15] = false;
									break;
								case 0x3: //XOR
									regX ^= regY;
									v[15] = false;
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
								pc+=2;
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
							uint8_t posX = (v[((curOpcode >> 8) & 0xF)] % 64);
							uint8_t posY = (v[((curOpcode >> 4) & 0xF)] % 32);
							v[15] = false;
							for(uint8_t h = 0; h < (curOpcode & 0xF); ++h){
								if((h + posY) > 31){
									break;
								}
								uint8_t line = bus.read(i+h);
								uint8_t curX = posX;
								draw:
								auto& disp = display[curX][(posY+h)];
								if(line & disp){
									v[15] = true;
								}
								disp ^= (line & 128);
								line <<= 1;
								++curX;
								if(line && curX < 64){
									goto draw;
								}
							}
							if(displayWait){
								cycles += a;
								return;
							}
						}
						break;
					case 0xE:
						switch((curOpcode & 0xFF)){
							case 0x9E: //SKP
								if(key[v[((curOpcode >> 8) & 0xF)] & 0xF]){
									pc+=2;
								}
							break;
							case 0xA1: //SKNP
								if(!key[v[((curOpcode >> 8) & 0xF)] & 0xF]){
									pc+=2;
								}
							break;
							default:[[unlikely]]
								std::cout << "GURU MEDITATION unknown opcode\n";
								getDebugInfo();
							break;
						}
						break;
					case 0xF:
						switch(curOpcode & 0xFF){
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
							case 0x33: //LD
								{
									auto& num = v[((curOpcode >> 8) & 0xF)];
									bus.write(num / 100, i);
									bus.write(num / 10 % 10, i+1);
									bus.write(num % 10, i+2);
								}
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
		
		inline std::string loggedTick(uint32_t steps){
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
	} cpu;

	class System:public Module{
		private:
		float freq = 440 * 2 * M_PI;
		bool lastFrame = false;
		
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
			audioPhase %= sampleFreq;
			if(cpu.getSound()){
				lastFrame = true;
				for(int i = audioPhase; i < (audioPhase + (sampleFreq/targetFPS)); ++i){
					double time = i/(double)sampleFreq;
					audioBuffer[(i-audioPhase)] = std::sin(freq*time)*(volume * 32767);
				}
				audioPhase += (sampleFreq/targetFPS);
			}else{
				if(lastFrame){
					lastFrame = false;
					bool silent = false;
					for(int i = audioPhase; i < (audioPhase + (sampleFreq/targetFPS)); ++i){
						double time = i/(double)sampleFreq;
						if(std::sin(freq*time)*(volume * 32767) <= 10.0f && std::sin(freq*time)*(volume * 32767) >= -10.0f || silent){
							silent = true;
							audioBuffer[(i-audioPhase)] = 0;
						}else if(!silent){
							audioBuffer[(i-audioPhase)] = std::sin(freq*time)*(volume * 32767);
						}
					}
				}else{
					for(int i = 0; i < (sampleFreq/targetFPS); ++i){
						audioBuffer[i] = 0;
					}
				}
				audioPhase = 0;
			}
			return *audioBuffer.data();
		}
		
		uint32_t& getFrameBuffer() override{
			for(int y = 0; y < 32; y++){
				for(int x = 0; x < 64; x++){
					if(cpu.display[x][y]){
						frameBuffer[((y*64)+x)] = 0xFFFFFFFF;
					}else{
						frameBuffer[((y*64)+x)] = 0xFF000000;
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
			if(doWriteLog){
				writeLogToFile(cpu.loggedTick(debugStep));
			}else{
				cpu.tick(debugStep);
			}
			cpu.getDebugInfo();
		}

		uint64_t getCycles() const override {
			return cpu.cycles;
		}
		
		System(std::map<std::string, std::string> args):Module("Chip-8", 15, 64, 32, 1, 48000, 60.0){
			for(auto& [key, value] : args){
				if(key == "speed"){
					bclk = std::stoi(value);
				}
				if(key == "nodisplaywait"){
					cpu.displayWait = false;
				}
			}
			bus.loadROM(readFile(args.at("file"), 3584));
		}
	};
}
