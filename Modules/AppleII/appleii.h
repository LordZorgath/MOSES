//Apple ][ core for MOSES
//Friday July 11th, 2025

#include "../../module.h"
#include "../../CPUs/Interpreter/mos6502-nmos.h"

namespace Cores::Apple2{
	
	class System:public Module{
		
		public:
		
		int16_t* playAudio() override{
			return 0;
		}
		
		void getKey() override{}
		
		void runCycle() override{}
		
		void debugCycle() override{}
		
	};
};

