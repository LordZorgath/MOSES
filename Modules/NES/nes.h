//NES core for MOSES
//Friday July 11th, 2025

#include "../../module.h"
#include "bus.h"
#include "../../CPUs/Interpreter/mos6502-2a03.h"

namespace Cores::Nes{
	
	class System:public Module{
		
		public:
		
		void getKey() override{}
		
		void runCycle() override{}
		
		void debugCycle() override{}
		
		System(std::map<std::string, std::string> args):Module("Nintendo Entertainment System", 21477272, 256, 240, 2, 48000, 60.0){
			init = true;
		}
	};
};
