//SCHIP module for MOSES
//Thursday 26th June, 2025
#include "../../module.h"

namespace Cores::Schip{
	
	struct{
	
		private: 
		uint8_t mem[4096];
	
		public:
		uint8_t flagStore[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		
		void loadROM(std::vector<uint8_t> rom){
			for(uint i = 0; i < rom.size(); i++){
				mem[0x200+i] = rom[i];
			}
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
		
		void populateFont(){
			for(int i = 0; i < (16*15); i++){
				mem[i] = pixelFont[i];
			}
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
		uint16_t curOpcode;
		uint16_t curOpcodeMSB;
		uint16_t stack[12];
		uint64_t loggedTicks = 0;
		
		public:
		bool key[16];
		bool display[128][64];
		bool displayWait = true;
		bool hiresMode = false;
		bool release = true;
		uint8_t tempKey = 16;
		bool legacy = false;
		/*uint8_t variant = 2; //0 = SCHIP 1.0, 1 = SCHIP 1.1, 2 = SCHIP-MODERN, 3 = SCHIPC
		
		#define SCHIP1 0
		#define SCHIP11 1
		#define SCHIPM 2
		#define SCHIPC 3
		*/
		bool getSound(){
			return (st > 0);
		}
		
		int getScreenX(){
			return (hiresMode ? 128 : 64);
		}
		
		int getScreenY(){
			return (hiresMode ? 64 : 32);	
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
			uint8_t refC;
			uint8_t flagRef;
			for(uint32_t a = 0; a < steps; a++){
				curOpcode = bus.readOpcode(pc);
				curOpcodeMSB = (curOpcode & 0xF000) >> 12;
				pc+=2;
				switch(curOpcodeMSB){
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
								refA = (curOpcode & 0x000F);
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
								refA = (curOpcode & 0x000F);
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
										refA = (curOpcode & 0x000F);
										if(refA == 0 && legacy){
											std::cout << "GURU MEDITATION invalid 0 pixel scroll\n";
											getDebugInfo();
											break;
										}
										for(int y = getScreenY()-1; y >= 0; y--){
											for(int x = 0; x < getScreenX(); x++){
												if(y < refA){
													display[x][y] = false;
												}else{
													display[x][y] = display[x][(y-refA) & (getScreenY()-1)];
												}
											}
										}
										break;
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
								flagRef = (v[((curOpcode & 0x0F00) >> 8)] & 0b00000001);
								v[((curOpcode & 0x0F00) >> 8)] >>= 1;
								v[15] = flagRef;
								break;
							case 0x7: //SUBN
								flagRef = (v[((curOpcode & 0x00F0) >> 4)] >= v[((curOpcode & 0x0F00) >> 8)]);
								v[((curOpcode & 0x0F00) >> 8)] = (v[((curOpcode & 0x00F0) >> 4)] - v[((curOpcode & 0x0F00) >> 8)]);
								v[15] = flagRef;
								break;
							case 0xE: //SHL
								flagRef = ((v[((curOpcode & 0x0F00) >> 8)] & 0b1000000) >> 7);
								v[((curOpcode & 0x0F00) >> 8)] <<= 1;
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
						refA = ((curOpcode & 0x0F00) >> 8);
						pc = ((curOpcode & 0x0FFF) + v[refA]);
						break;
					case 0x0C: //RND
						v[((curOpcode & 0x0F00) >> 8)] = (rand() & (curOpcode & 0x00FF));
						break;
					case 0x0D: //DRW
						refA = v[((curOpcode & 0x0F00) >> 8)] & (getScreenX()-1);
						refB = v[((curOpcode & 0x00F0) >> 4)] & (getScreenY()-1);
						refC = ((curOpcode & 0x000F) == 0 ? 16 : (curOpcode & 0x000F));
						v[15] = false;
						for(int y = 0; y < refC; y++){
							if((y + refB) > (getScreenY()-1)){
								break;
							}
							for(int w = 0; w <= ((legacy && !hiresMode) ? 0 : (refC/16)); w++){
								flagRef = bus.read(i+(y*(1+(refC/16)))+w);
								for(int x = 0; x < 8; x++){
									if((refA+x+(w*8)) > (getScreenX()-1)){
										break;
									}
									if(flagRef & (0b10000000 >> x) && display[(refA+x+(w*8))][(refB+y)]){
										v[15] = true;
									}
									display[(refA+x+(w*8))][(refB+y)] ^= (flagRef & (0b10000000 >> x)) >> (7-x);
								}
							}
						}
						if(displayWait && !hiresMode && legacy){
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
		
		System(int argc, std::string* args):Module("SCHIP", 32, 128, 64, 1, 1800, 60.0){
			frameBuffer.resize(128*64);
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
				if(args[i] == "--legacy"){
					cpu.legacy = true;
				}
			}
			if(!fileArg){
				std::cout << "GURU MEDITATION no file argument\n";
			}
			if(fileFound){
				init = true;
				bus.populateFont();
			}
		}
	};
}
