//Abstract class for all emulator cores.
//Friday 20th of June, 2025
#pragma once

namespace Cores{
	
	class WindowArgs{
		
		private:
		int x;
		int y;
		int numChannels;
		uint32_t sampleFreq;
		double targetFPS;
		
		public:
		int scaleFactor;
		WindowArgs(int w, int h, int scale, int channel, uint32_t freq, double fps){
			x = w;
			y = h;
			scaleFactor = scale;
			numChannels = channel;
			sampleFreq = freq;
			targetFPS = fps;
		}
		
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
	};
	
	class Module{
		
		protected:
		uint64_t pcBreakpoint; 
		uint64_t bclk;
		WindowArgs *winArgs;
		std::string name;
		std::string outFile;
		char **argv;
		int argc;
		std::vector<uint32_t> frameBuffer;
		bool init = false;
		bool fileFound = false;
		bool doWriteLog = false;
		const bool *keyCodes;
		int16_t *audioSamples;
		float volume = 0.25;
		int audioPhase = 0;
		
		std::vector<uint8_t> readFile(std::string path){
			std::vector<uint8_t> ret;
			std::ifstream file(path, std::ifstream::binary);
			if(!file.good()){
				std::cout << "GURU MEDITATION no file\n";
				return ret;
			}else{
				file.unsetf(std::ios::skipws);
				std::streampos fileSize;
				file.seekg(0, std::ios_base::end);
				fileSize = file.tellg();
				file.seekg(0, std::ios_base::beg);
				ret.reserve(fileSize);
				ret.insert(ret.begin(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
				file.close();
				fileFound = true;
				return ret;
			}
		}
		
		Module(std::string n, int f, int w, int h, int channels, int samples, double fps){
			winArgs = new WindowArgs(w, h, 1, channels, samples, fps);
			int32_t samplesPerFrame = 2*channels*samples/fps;
			audioSamples = new int16_t[samplesPerFrame];
			name = n;
			bclk = f;
			srand(69);
		};
		
		public:
		uint32_t debugStep = 1;
		bool keyRelease = false;
		bool breakpointActive = false;
		bool dbg = false;
		
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
		
		virtual int16_t* playAudio() = 0;
		
		virtual void runCycle() = 0;
		
		virtual void debugCycle() = 0;
		
		virtual void getKey() = 0;
		
		void addKey(const bool *key){
			keyCodes = key;
		}
		
		std::vector<uint32_t>& getFramebuffer(){
			return frameBuffer;
		}
		
		std::string getName(){
			return name;
		}
		
		WindowArgs* getWindowArgs(){
			return winArgs;
		}
	};
}
