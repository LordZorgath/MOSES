//Abstract class for all emulator cores.
//Friday 20th of June, 2025
#pragma once

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Cores{
	
	class Module{
		
		private:
		int x;
		int y;
		int numChannels;
		uint32_t sampleFreq;
		double targetFPS;
		
		protected:
		uint64_t pcBreakpoint; 
		uint64_t bclk;
		std::string name;
		std::string outFile;
		char **argv;
		int argc;
		std::vector<uint32_t> frameBuffer;
		std::vector<int16_t> audioBuffer;
		bool init = true;
		bool fileFound = false;
		bool doWriteLog = false;
		const bool *keyCodes;
		float volume = 0.25;
		int audioPhase = 0;
		
		virtual void getKey() = 0;
		
		std::vector<uint8_t> readFile(std::string path, int maxFilesize){
			std::vector<uint8_t> ret;
			std::ifstream file(path, std::ifstream::binary);
			if(!file.good()){
				std::cout << "GURU MEDITATION no file\n";
				init = false;
				return ret;
			}else{
				file.unsetf(std::ios::skipws);
				std::streampos fileSize;
				file.seekg(0, std::ios_base::end);
				fileSize = file.tellg();
				if(fileSize > maxFilesize){
					std::cout << "GURU MEDITATION file too large\n";
					init = false;
					return ret;
				}
				file.seekg(0, std::ios_base::beg);
				ret.reserve(fileSize);
				ret.insert(ret.begin(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
				file.close();
				fileFound = true;
				return ret;
			}
		}
		
		Module(std::string n, int f, int w, int h, int channels, int samples, double fps){
			x = w;
			y = h;
			numChannels = channels;
			sampleFreq = samples;
			targetFPS = fps;
			frameBuffer.resize(w*h);
			audioBuffer.resize(2*channels*samples/fps);
			name = n;
			bclk = f;
			srand(0x69);
		};
		
		public:

		int getX(){
			return x;
		}
		
		int getY(){
			return y;
		}
		
		uint32_t getSampleFrequency(){
			return sampleFreq;
		}
		
		int getAudioChannels(){
			return numChannels;
		}
		
		double getFPS(){
			return targetFPS;
		}
		
		uint32_t debugStep = 1;
		bool breakpointActive = false;
		bool dbg = false;

		virtual ~Module() = default;

		void setPcBreakpoint(uint64_t i){
			pcBreakpoint = i;
		}
		
		void setLogOutput(std::string fileName){
			outFile = fileName;
			doWriteLog = true;
		}
		
		void writeLogToFile(std::string contents){
			std::ofstream out;
			out.open(outFile, std::ios_base::app);
			out << contents;
		}
		
		void setVolume(int vol){
			if(vol >= 0 && vol <= 100){
				volume = vol/(float)100;
			}else{
				std::cout << "GURU MEDITATION invalid volume setting\n";
			}
		}
		
		bool checkInit(){
			return init;
		}
		
		virtual void runCycle() = 0;
		
		virtual void debugCycle() = 0;

		virtual uint64_t getCycles() const{ return 0; }
		
		virtual int16_t& getAudioBuffer(){ return *audioBuffer.data(); }
		
		virtual uint32_t& getFrameBuffer(){ return *frameBuffer.data(); }
		
		void addKey(const bool *key){
			keyCodes = key;
		}
		
		std::string getName(){
			return name;
		}
	};
}
