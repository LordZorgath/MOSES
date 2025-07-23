//NES core for MOSES
//Friday July 11th, 2025

#include "../../module.h"
#include "bus.h"
#include "mos6502-2a03.h"

namespace Cores::Nes{
	
	class System:public Module{
		
		public:
		
		int16_t* playAudio() override{
			return 0;
		}
		
		void getKey() override{}
		
		void runCycle() override{}
		
		void debugCycle() override{}
		
		System(int argc, std::string* args):Module("Nintendo Entertainment System", 21477272, 256, 240, 2, 48000, 60.0){
			init = true;
		}
	};
};
