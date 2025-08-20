//Blank module for easily creating new ones.
//Wednesday August 20th, 2025

#include "../../module.h"

namespace Cores::ModuleName{
	
	class System:public Module{
		
		public:
		
		int16_t* playAudio() override{
			return 0;
		}
		
		void getKey() override{}
		
		void runCycle() override{}
		
		void debugCycle() override{}
		
		std::vector<uint32_t>& getFrameBuffer() override{
			return frameBuffer;
		}
		
		System(int argc, std::string* args):Module("Debug", 100, 640, 480, 2, 48000, 60.0){
			init = true;
		}
	};
};
