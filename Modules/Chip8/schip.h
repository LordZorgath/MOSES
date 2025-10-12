//SCHIP module for MOSES
//July 26th, 2025
#include "../../module.h"

namespace Cores::Schip{
	
	struct{
	
		private: 
		uint8_t mem[4096];
	
		public:
		uint8_t flagStore[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		
		void loadROM(std::vector<uint8_t> rom){
			for(int i = 0; i < rom.size(); i++){
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
			for(int i = 0; i < (16*15); i++){
				mem[i] = pixelFont[i];
			}
		}

		inline uint_fast8_t read(uint16_t addr){ return mem[(addr % 4096)]; }
		
		inline uint_fast16_t readOpcode(uint16_t addr){ return (mem[(addr % 4096)] << 8) | mem[((addr+1) % 4096)]; }
		
		inline void write(uint8_t val, uint16_t addr){ mem[(addr % 4096)] = val; }
		
	} bus;
	
	struct{
		//SCHIP interpreter.
		
		private:
		//Register definitions
		uint8_t v[16];
		uint16_t i = 0;
		uint16_t pc = 0x200; //Program counter
		uint8_t sp = 0; //Stack pointer
		uint8_t dt = 0; //Delay timer
		uint8_t st = 0; //Sound timer

		//Variables for the interpreter
		uint_fast16_t curOpcode;
		uint16_t stack[256];
		
		public:
		bool key[16];
		uint8_t display[128][64];
		bool displayWait = true;
		bool hiresMode = false;
		uint8_t tempKey = 16;
		bool legacy = false;
		uint64_t cycles = 0;
		
		inline bool getSound(){ return (st > 0); }
		
		inline uint8_t getScreenX(){ return (hiresMode ? 128 : 64); }
		
		inline uint8_t getScreenY(){ return (hiresMode ? 64 : 32); }
		
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
			dt = (dt == 0) ? 0 : --dt;
			st = (st == 0) ? 0 : --st;
			for(uint32_t a = 0; a < steps; a++){
				curOpcode = bus.readOpcode(pc);
				pc+=2;
				switch((curOpcode >> 12)){
					case 0x0:
						switch(curOpcode){
							case 0xE0: //CLS
								for(int i = 0; i < 128*64; i++){
									display[i%128][i/128] = false;
								}
								break;
							case 0xEE: //RET
								sp--;
								pc = stack[sp];
								break;
							case 0xFB: //SCR
								for(int y = 0; y < getScreenY(); y++){
									for(int x = getScreenX()-1; x >= 0; x--){
										if(x < 4){
											display[x][y] = false;
										}else{
											display[x][y] = display[((x-4) & (getScreenX()-1))][y];
										}
									}
								}
								break;
							case 0xFC: //SCL
								for(int y = 0; y < getScreenY(); y++){
									for(int x = 0; x < getScreenX(); x++){
										if(x >= (getScreenX()-4)){
											display[x][y] = 0;
										}else{
											display[x][y] = display[((x+4) & (getScreenX()-1))][y];
										}
									}
								}
								break;
							case 0xFE: //LOW
								hiresMode = false;
								if(!legacy){
									for(int i = 0; i < 128*64; i++){
										display[i%128][i/128] = false;
									}
								}
								break;
							case 0xFF: //HIGH
								hiresMode = true;
								if(!legacy){
									for(int i = 0; i < 128*64; i++){
										display[i%128][i/128] = false;
									}
								}
								break;
							default:[[unlikely]]
								switch(((curOpcode >> 4) & 0xF)){
									case 0xC: //SCD
										{
											uint8_t offset = (curOpcode & 0xF);
											if(legacy && offset == 0){
												std::cout << "GURU MEDITATION invalid 0 pixel scroll\n";
												getDebugInfo();
												break;
											}
											for(int y = getScreenY()-1; y >= 0; y--){
												for(int x = 0; x < getScreenX(); x++){
													if(y < offset){
														display[x][y] = false;
													}else{
														display[x][y] = display[x][(y-offset) & (getScreenY()-1)];
													}
												}
											}
										}
										break;
									default:[[unlikely]]
										std::cout << "GURU MEDITATION unknown opcode\n";
										getDebugInfo();
										break;
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
										regX >>= 1;
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
										regX <<= 1;
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
						pc = ((curOpcode & 0xFFF) + v[((curOpcode >> 8) & 0xF)]);
						break;
					case 0xC: //RND
						v[((curOpcode >> 8) & 0xF)] = (rand() & (curOpcode & 0xFF));
						break;
					case 0xD: //DRW
						{
							uint8_t posX = v[((curOpcode >> 8) & 0xF)] & (getScreenX()-1);
							uint8_t posY = v[((curOpcode >> 4) & 0xF)] & (getScreenY()-1);
							uint8_t spriteHeight = ((curOpcode & 0xF) == 0 ? 16 : (curOpcode & 0xF));
							v[15] = false;
							for(int h = 0; h < spriteHeight; h++){
								if((h + posY) > (getScreenY()-1)){
									if(legacy && hiresMode){
										++v[15];
										continue;
									}else{
										break;
									}
								}
								for(int w = 0; w <= ((legacy && !hiresMode) ? 0 : (spriteHeight == 16)); w++){
									auto line = bus.read(i+(h*(1+(spriteHeight == 16)))+w);
									for(int x = 0; x < 8; x++){
										if((posX+x+(w*8)) > (getScreenX()-1)){
											break;
										}
										auto& disp = display[(posX+x+(w*8))][(posY+h)];
										if(line & disp){
											if(legacy && hiresMode){
												++v[15];
											}else{
												v[15] = true;
											}
										}
										disp ^= (line & 128);
										line <<= 1;
										if(line == 0){
											break;
										}
									}
								}
							}
							if(displayWait && !hiresMode && legacy){
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
								v[(curOpcode & 0xF00) >> 8] = dt;
								break;
							case 0xA: //LD
								for(int i = 0; i < 16; i++){
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
							case 0x55: //LD
								for(int a = 0; a <= ((curOpcode >> 8) & 0xF); a++){
									bus.write(v[a], i+a);
								}
								break;
							case 0x65: //LD
								for(int a = 0; a <= ((curOpcode >> 8) & 0xF); a++){
									v[a] = bus.read(i+a);
								}
								break;
							case 0x75: //LD
								for(int a = 0; a <= ((curOpcode >> 8) & 0xF); a++){
									bus.flagStore[a] = v[a];
								}
								break;
							case 0x85: //LD
								for(int a = 0; a <= ((curOpcode >> 8) & 0xF); a++){
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
		
		inline std::string loggedTick(uint32_t steps){
			std::stringstream ret;
			for(int a = 0; a < steps; a++){
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
				for(int i = audioPhase; i < (audioPhase + (sampleFreq/targetFPS)); i++){
					double time = i/(double)sampleFreq;
					audioBuffer[(i-audioPhase)] = std::sin(freq*time)*(volume * 32767);
				}
				audioPhase += (sampleFreq/targetFPS);
			}else{
				if(lastFrame){
					lastFrame = false;
					bool silent = false;
					for(int i = audioPhase; i < (audioPhase + (sampleFreq/targetFPS)); i++){
						double time = i/(double)sampleFreq;
						if(std::sin(freq*time)*(volume * 32767) <= 10.0f && std::sin(freq*time)*(volume * 32767) >= -10.0f || silent){
							silent = true;
							audioBuffer[(i-audioPhase)] = 0;
						}else if(!silent){
							audioBuffer[(i-audioPhase)] = std::sin(freq*time)*(volume * 32767);
						}
					}
				}else{
					for(int i = 0; i < (sampleFreq/targetFPS); i++){
						audioBuffer[i] = 0;
					}
				}
				audioPhase = 0;
			}
			return *audioBuffer.data();
		}
		
		uint32_t& getFrameBuffer() override{
			if(cpu.hiresMode){
				for(int y = 0; y < 64; y++){
					for(int x = 0; x < 128; x++){
						if(cpu.display[x][y]){
							frameBuffer[((y*128)+x)] = 0xFFFFFFFF;
						}else{
							frameBuffer[((y*128)+x)] = 0xFF000000;
						}
					}
				}
			}else{
				for(int y = 0; y < 64; y++){
					for(int x = 0; x < 64; x++){
						if(cpu.display[x][y/2]){
							frameBuffer[2*((y*64)+x)] =  0xFFFFFFFF;
							frameBuffer[2*((y*64)+x)+1] =  0xFFFFFFFF;
						}else{
							frameBuffer[2*((y*64)+x)] =  0xFF000000;
							frameBuffer[2*((y*64)+x)+1] =  0xFF000000;
						}
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
		
		System(std::map<std::string, std::string> args):Module("SCHIP", 30, 128, 64, 1, 48000, 60.0){
			for(auto& [key, value] : args){
				if(key == "speed"){
					bclk = std::stoi(value);
				}
				if(key == "nodisplaywait"){
					cpu.displayWait = false;
				}
				if(key == "legacy"){
					cpu.legacy = true;
				}
			}
			bus.loadROM(readFile(args.at("file"), 3584));
		}
	};
}
