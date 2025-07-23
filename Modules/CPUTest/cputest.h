//CPU testing core
//Tuesday July 15th, 2025

#include "../../module.h"
#include "mos6502-nmos.h"

namespace Cores::CPUTest{
	
	class System:public Module{
		
		public:
		
		int16_t* playAudio() override{
			return 0;
		}
		
		void getKey() override{}
		
		void runCycle() override{}
		
		void debugCycle() override{}
		
		System(int argc, std::string* args):Module("Debug", 21477272, 256, 240, 2, 48000, 60.0){
			init = true;
		}
	};
};
