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

		uint_fast8_t read(uint16_t addr){
			if(addr > 4095){
				std::cout << "GURU MEDITATION mem out of bounds read\n";
				return 0;
			}else{
				return mem[addr];
			}
		}
		
		uint_fast16_t readOpcode(uint16_t addr){
			return (mem[(addr % 4096)] << 8) | mem[((addr+1) % 4096)];
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
		uint16_t stack[12];
		uint64_t loggedTicks = 0;
		
		public:
		bool key[16];
		uint8_t display[128][64];
		bool displayWait = true;
		bool hiresMode = false;
		uint8_t tempKey = 16;
		bool legacy = false;
		uint64_t cycles = 0;
		
		bool getSound(){
			return (st > 0);
		}
		
		int getScreenX(){
			return (hiresMode ? 128 : 64);
		}
		
		int getScreenY(){
			return (hiresMode ? 64 : 32);	
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
			dt = (dt == 0) ? 0 : --dt;
			st = (st == 0) ? 0 : --st;
			for(uint32_t a = 0; a < steps; a++){
				curOpcode = bus.readOpcode(pc);
				pc+=2;
				switch((curOpcode >> 12)){
					case 0x00:
						switch(curOpcode){
							case 0x00E0: //CLS
								for(int i = 0; i < 128*64; i++){
									display[i%128][i/128] = false;
								}
								break;
							case 0x00EE: //RET
								if(sp == 0){
									std::cout << "GURU MEDITATION return outside of subroutine\n";
									getDebugInfo();
								}else{
									sp--;
									pc = stack[sp];
								}
								break;
							case 0x00FB: //SCR
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
							case 0x00FC: //SCL
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
							case 0x00FE: //LOW
								hiresMode = false;
								if(!legacy){
									for(int i = 0; i < 128*64; i++){
										display[i%128][i/128] = false;
									}
								}
								break;
							case 0x00FF: //HIGH
								hiresMode = true;
								if(!legacy){
									for(int i = 0; i < 128*64; i++){
										display[i%128][i/128] = false;
									}
								}
								break;
							default:
								switch(((curOpcode & 0x00F0) >> 4)){
									case 0x0C: //SCD
										{
											uint8_t offset = (curOpcode & 0x000F);
											if(offset == 0 && legacy){
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
									default:
										std::cout << "GURU MEDITATION unknown opcode\n";
										getDebugInfo();
										break;
								}
								break;
						}
						break;
					case 0x01: //JP
						pc = (curOpcode & 0x0FFF);
						break;
					case 0x02: //CALL
						if(sp > 15){
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
							getDebugInfo();
						}
						break;
					case 0x06: //LD
						v[((curOpcode & 0x0F00) >> 8)] = (curOpcode & 0x00FF);
						break;
					case 0x07: //ADD
						v[((curOpcode & 0x0F00) >> 8)] += (curOpcode & 0x00FF);
						break;
					case 0x08: 
						{
							uint8_t flagRef;
							switch(curOpcode & 0x000F){
								case 0x0: //LD
									v[((curOpcode & 0x0F00) >> 8)] = v[((curOpcode & 0x00F0) >> 4)];
									break;
								case 0x1: //OR
									v[((curOpcode & 0x0F00) >> 8)] |= v[((curOpcode & 0x00F0) >> 4)];
									break;
								case 0x2: //AND
									v[((curOpcode & 0x0F00) >> 8)] &= v[((curOpcode & 0x00F0) >> 4)];
									break;
								case 0x3: //XOR
									v[((curOpcode & 0x0F00) >> 8)] ^= v[((curOpcode & 0x00F0) >> 4)];
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
						pc = ((curOpcode & 0x0FFF) + v[((curOpcode & 0x0F00) >> 8)]);
						break;
					case 0x0C: //RND
						v[((curOpcode & 0x0F00) >> 8)] = (rand() & (curOpcode & 0x00FF));
						break;
					case 0x0D: //DRW
						{
							uint8_t posX = v[((curOpcode & 0x0F00) >> 8)] & (getScreenX()-1);
							uint8_t posY = v[((curOpcode & 0x00F0) >> 4)] & (getScreenY()-1);
							uint8_t spriteHeight = ((curOpcode & 0x000F) == 0 ? 16 : (curOpcode & 0x000F));
							uint8_t pixels;
							v[15] = false;
							for(int h = 0; h < spriteHeight; h++){
								if((h + posY) > (getScreenY()-1)){
									break;
								}
								for(int w = 0; w <= ((legacy && !hiresMode) ? 0 : (spriteHeight == 16)); w++){
									pixels = bus.read(i+(h*(1+(spriteHeight == 16)))+w);
									for(int x = 0; x < 8; x++){
										if((posX+x+(w*8)) > (getScreenX()-1)){
											break;
										}
										auto& disp = display[(posX+x+(w*8))][(posY+h)];
										if(pixels & disp){
											v[15] = true;
										}
										disp ^= (pixels & 128);
										pixels <<= 1;
										if(pixels == 0){
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
								for(int i = 0; i < 16; i++){
									if(key[i]){
										tempKey = i;
										pc-=2;
										cycles += a;
										return;
									}else if(i == 15){
										if(tempKey != 16){
											v[((curOpcode & 0x0F00) >> 8)] = tempKey;
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
							case 0x30:
								i = (16*5) + (v[((curOpcode & 0x0F00) >> 8)] & 0x0F) * 10;
								break;
							case 0x33: //LD
								{
									uint8_t num = v[((curOpcode & 0x0F00) >> 8)];
									bus.write(num / 100, i);
									bus.write(num / 10 % 10, i+1);
									bus.write(num % 10, i+2);
								}
								break;
							case 0x55: //LD
								for(int a = 0; a <= ((curOpcode & 0x0F00) >> 8); a++){
									bus.write(v[a], i+a);
								}
								break;
							case 0x65: //LD
								for(int a = 0; a <= ((curOpcode & 0x0F00) >> 8); a++){
									v[a] = bus.read(i+a);
								}
								break;
							case 0x75: //LD
								for(int a = 0; a <= ((curOpcode & 0x0F00) >> 8); a++){
									bus.flagStore[a] = v[a];
								}
								break;
							case 0x85: //LD
								for(int a = 0; a <= ((curOpcode & 0x0F00) >> 8); a++){
									v[a] = bus.flagStore[a];
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
			cycles += steps;
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
			std::stringstream coreSettings;
			coreSettings << args.at("core");
			std::string curOption;
			while(getline(coreSettings, curOption, ',')){
				std::stringstream key;
				std::string value;
				key << curOption;
				getline(key, value, '=');
				if(value == "nodisplaywait"){
					cpu.displayWait = false;
				}
				if(value == "legacy"){
					cpu.legacy = true;
				}
				if(value == "speed"){
					getline(key, value, '=');
					if(std::stoi(value) < 1){
						std::cout << "GURU MEDITATION invalid ipf setting\n";
					}else{
						bclk = std::stoi(value);
					}
				}
			}
			bus.loadROM(readFile(args.at("file"), 3584));
		}
	};
}
