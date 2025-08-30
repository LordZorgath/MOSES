//Apple ][ core for MOSES
//Friday July 11th, 2025

#include "../../module.h"
#include "../../CPUs/Interpreter/mos6502-nmos.h"

namespace Cores::Apple2{
	
	class System:public Module{
		
		public:
		
		void getKey() override{}
		
		void runCycle() override{}
		
		void debugCycle() override{}
		
	};
};

