//CPU testing core
//Tuesday July 15th, 2025

#include "../../module.h"
#include "../../CPUs/Interpreter/mos6502-nmos.h"

namespace Cores::CPUTest{
	
	struct{
		uint8_t mem[65536];
	} bus;
	
	class System:public Module{
		
		public:
		
		void getKey() override{}
		
		void runCycle() override{}
		
		void debugCycle() override{}
		
		System(std::map<std::string, std::string> args):Module("Debug", 21477272, 256, 240, 2, 48000, 60.0){

		}
	};
};
