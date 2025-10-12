//MOSES multi-system emulator.
/*  Copyright (C) 2025  Justin Warner

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
//File created Thursday 19th of June, 2025

#define M_PI 3.14159265358979323846

#include <memory>
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <typeinfo>
#include <thread>
#include <chrono>
#include "vendored/SDL3-3.2.16/include/SDL3/SDL.h"
#include "vendored/json/include/nlohmann/json.hpp"
#include "vendored/CLI11/include/CLI/CLI.hpp"
#include "vendored/ghc/fpscount.hpp"
#include "Modules/Chip8/chip8.h"
#include "Modules/Chip8/schip.h"
#include "Modules/Chip8/xochip.h"
#include "Modules/NES/nes.h"
#include "Modules/AppleII/appleii.h"
#include "Modules/CPUTest/cputest.h"

using namespace Cores;
using json = nlohmann::json;

SDL_Window* mainWindow;
SDL_Renderer* render;
SDL_Texture* frameBuffer;
SDL_AudioStream* audioOut;
SDL_AudioSpec sampleSpec;

void createWindow(int w, int h, int s, int c){
	if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO)){
		std::cout << "GURU MEDITATION sdl init %s\n";
	}
	mainWindow = SDL_CreateWindow("MOSES", w, h, 0);
	render = SDL_CreateRenderer(mainWindow, NULL);
	if(SDL_SetRenderVSync(render, 1) == false ){
		std::cout << "GURU MEDITATION vsync %s\n";
	}
	frameBuffer = SDL_CreateTexture(render, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, w, h);
	if(frameBuffer == NULL){
		std::cout << "GURU MEDITATION null texture\n";
	}
	SDL_SetTextureBlendMode(frameBuffer, SDL_BLENDMODE_NONE);
	SDL_SetTextureScaleMode(frameBuffer, SDL_SCALEMODE_NEAREST);
	sampleSpec.freq = s;
	sampleSpec.channels = c;
	sampleSpec.format = SDL_AUDIO_S16LE;
	audioOut = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr, nullptr, nullptr);
	SDL_SetAudioStreamFormat(audioOut, &sampleSpec, &sampleSpec);
	SDL_ResumeAudioStreamDevice(audioOut);
}

void updateDisplay(uint32_t* pixels, int width){
	SDL_UpdateTexture(frameBuffer, nullptr, pixels, width*4);
	SDL_RenderTexture(render, frameBuffer, nullptr, nullptr);
	SDL_RenderPresent(render);
}

void scaleDisplay(int w, int h, int scale){
	SDL_SetRenderScale(render, scale, scale);
	if(!SDL_SetWindowSize(mainWindow, w*scale, h*scale)){
		std::cout << "GURU MEDITATION window resize\n";
	}
}
void cleanExit(std::string message){
	SDL_DestroyWindow(mainWindow);
	SDL_DestroyRenderer(render);
	SDL_DestroyTexture(frameBuffer);
	SDL_DestroyAudioStream(audioOut);
	SDL_Quit();
	std::cout << "GURU MEDITATION " << message << std::endl;
	exit(1);
}

void cleanExit(){
	SDL_DestroyWindow(mainWindow);
	SDL_DestroyRenderer(render);
	SDL_DestroyTexture(frameBuffer);
	SDL_DestroyAudioStream(audioOut);
	SDL_Quit();
	exit(0);
}

auto getCore(std::map<std::string, std::string> args) -> std::unique_ptr<Cores::Module>{
	std::stringstream system;
	system << args.at("core");
	std::string core;
	getline(system, core, ',');
	if(core == "nes"){ return std::make_unique<Cores::Nes::System>(args); }
	else if(core == "chip8"){ return std::make_unique<Cores::Chip8::System>(args); }
	else if(core == "schip"){ return std::make_unique<Cores::Schip::System>(args); }
	else if(core == "xochip"){ return std::make_unique<Cores::Xochip::System>(args); }
	else {
		cleanExit("unknown core: " + core);
		return 0;
	}
}

std::map<std::string, std::string> cfgParse(std::string core, std::string file){
	const int globalArgCount = 3;
	std::array<std::string, globalArgCount> validSettings = {"scale", "volume", "file"};
	std::map<std::string, std::string> ret;
	std::ifstream config(file);
	try{
		if(!config){
			throw std::invalid_argument("404 file not found");
		}
		json settings = json::parse(config);
		if(!settings.contains("cores")){
			throw std::invalid_argument("config file invalid");
		}
		if(core == "default"){
			if(!settings.at("cores").contains("default")){
				throw std::invalid_argument("no default core");
			}
			core = settings["cores"]["default"];
			ret.insert_or_assign("core", core);
		}
		if(!settings.at("cores").contains(core)){
			throw std::invalid_argument("unknown core: " + core);
		}
		for(auto& i : settings["cores"][core].items()){
			for(auto& j : validSettings){
				if(i.key() == j){
					ret.insert_or_assign(i.key(), to_string(i.value()));
					break;
				}
			}
			if(i.key() == "core specific"){
				json cmd = settings["cores"][core]["core specific"];
				for(auto& j : cmd.items()){
					ret.insert_or_assign(j.key(), to_string(j.value()));
				}
			}
		}
	}catch(std::invalid_argument &e){
		cleanExit(e.what());
	}
	for(auto& [key, value] : ret){
		value.erase(remove(value.begin(), value.end(), '\"' ),value.end());
		//std::cout << key << " " << value << std::endl;
	}
	return ret;
}

int main(int argc, char* argv[]){
	CLI::App moses{"MOSES"};
	argv = moses.ensure_utf8(argv);
	std::map<std::string, std::string> globalSettings;
	std::optional<std::string> readConfigFile;
	std::optional<std::string> writeLog;
	std::optional<std::stringstream> coreOptions;
	std::optional<uint64_t> breakPoint;
	std::optional<uint64_t> debugSpeed;
	std::string file;
	bool debug = false;
	bool measurePerf = false;
	bool uncapFPS = false;
	int scaleFactor;
	int volume;
	moses.add_flag("-D, --debug", debug, "Enable debug functions");
	moses.add_flag("-m, --measure-performance", measurePerf, "Measure performance in cycles and frames per second");
	moses.add_flag("-u, --unlimited-framerate", uncapFPS, "Run the core as fast as possible instead of limiting framerate");
	moses.add_option("-c, --core", coreOptions, "Core and core-specific options to load")
	->default_val("default");
	moses.add_option("-C, --config", readConfigFile, "Config file to load")
	->excludes("--core");
	moses.add_option("-f, --file", file, "File to be loaded")
	->excludes("--config")
	->needs("--core");
	moses.add_option("-s, --scale", scaleFactor, "Integer scaling factor")
	->default_val(1);
	moses.add_option("-d, --debugspeed", debugSpeed, "Number of ticks to run before stopping in debug mode")
	->default_val(1)
	->needs("--debug");
	moses.add_option("-b, --breakpoint", breakPoint, "Number of ticks before hitting a breakpoint in debug mode")
	->needs("--debug");
	moses.add_option("-w, --writelog", writeLog, "Log interpreter output to a file")
	->needs("--debug");
	moses.add_option("-v, --volume", volume, "Volume setting")
	->default_val(25);
	try {
		moses.parse(argc, argv);
	}catch(const CLI::ParseError &e){
		std::cout << "GURU MEDITATION CLI: ";
		return moses.exit(e);
	}
	std::string core;
	getline(coreOptions.value(), core, ',');
	if(readConfigFile.has_value()){
		globalSettings = cfgParse(core, readConfigFile.value());
	}else{
		globalSettings.insert_or_assign("core", coreOptions.value().str());
		globalSettings.insert_or_assign("file", file);
		globalSettings.insert_or_assign("volume", std::to_string(volume));
		globalSettings.insert_or_assign("scale", std::to_string(scaleFactor));
		std::stringstream coreSettings;
		coreSettings << globalSettings.at("core");
		std::string curOption;
		while(getline(coreSettings, curOption, ',')){
			std::stringstream current;
			current << curOption;
			std::string key;
			if(getline(current, key, '=')){
				std::string value;
				getline(current, value, '=');
				globalSettings.insert_or_assign(key, value);
			}else{
				globalSettings.insert_or_assign(key, nullptr);
			}
		}
	}
	std::unique_ptr<Module> sys = getCore(globalSettings);
	if(!sys -> checkInit()){
		cleanExit("core init");
	}
	sys -> addKey (SDL_GetKeyboardState(nullptr));
	sys -> setVolume(std::stoi(globalSettings.at("volume")));
	createWindow(sys -> getX(), sys -> getY(), sys -> getSampleFrequency(), sys -> getAudioChannels());
	scaleDisplay(sys -> getX(), sys -> getY(), std::stoi(globalSettings.at("scale")));
	uint32_t bufferSize = (sys -> getSampleFrequency()/sys -> getFPS())*(sys -> getAudioChannels());
	int16_t *bufferSamples = new int16_t[bufferSize];
	for(int i = 0; i < bufferSize; i++){
		bufferSamples[i] = 0;
	}
	ghc::FPSCounter fps;
	std::chrono::microseconds frameTime((uint64_t)(1000000/sys -> getFPS()));
	SDL_SetWindowTitle(mainWindow, ("MOSES: " + sys -> getName()).c_str());
	if(uncapFPS){
		if(SDL_SetRenderVSync(render, 0) == false ){
			std::cout << "GURU MEDITATION vsync %s\n";
		}
	}
	SDL_Event event;
	if(debug)[[unlikely]]{
		bool debugPause = false;
		bool dbgPauseEnable = true;
		if(debugSpeed.has_value()){
			sys -> debugStep = debugSpeed.value();
		}
		if(breakPoint.has_value()){
			sys -> breakpointActive = true;
			sys -> setPcBreakpoint(breakPoint.value());
		}
		if(writeLog.has_value()){
			sys -> setLogOutput(writeLog.value());
		}
		while(true){
			while(SDL_PollEvent(&event)){
				switch( event.type ){
					case SDL_EVENT_KEY_DOWN:
						std::cout << std::hex;
						switch(event.key.key){
							case SDLK_RSHIFT:
								dbgPauseEnable = !dbgPauseEnable;
								std::cout << (dbgPauseEnable ? "STOP\n" : "RUN\n");
								break;
							case SDLK_SPACE:
								debugPause = false;
								break;
							case SDLK_UP:
								sys -> debugStep = fmin(pow(2, 30), sys -> debugStep * 2);
								std::cout << "DBG SPEED " << +(sys -> debugStep) << "\n";
								break;
							case SDLK_DOWN:
								sys -> debugStep = fmax(1, sys -> debugStep / 2);
								std::cout << "DBG SPEED " << +(sys -> debugStep) << "\n";
								break;
						}
						if(event.key.key == SDLK_ESCAPE){
							goto finish;
						}
						break;
					case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
						goto finish;
				}
			}
			if(!debugPause && dbgPauseEnable){
				sys -> debugCycle();
				debugPause = true;
			}else if(!dbgPauseEnable){
				sys -> debugCycle();
			}
			updateDisplay(&sys -> getFrameBuffer(), sys -> getX());
		}
	}else{
		while(true){
			auto beforeTime = std::chrono::steady_clock::now();
			while(SDL_PollEvent(&event)){
				switch( event.type ){
					case SDL_EVENT_KEY_DOWN:
						if(event.key.key == SDLK_ESCAPE){
							goto finish;
						}
						break;
					case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
						goto finish;
				}
			}
			sys -> runCycle();
			SDL_PutAudioStreamData(audioOut, bufferSamples, (SDL_GetAudioStreamAvailable(audioOut) < bufferSize) ? 2*bufferSize : 0);
			SDL_PutAudioStreamData(audioOut, &sys -> getAudioBuffer(), 2*bufferSize);
			updateDisplay(&sys -> getFrameBuffer(), sys -> getX());
			if(measurePerf && fps.frameTick(sys->getCycles())){
				SDL_SetWindowTitle(mainWindow, fps.getStatsString(std::string("MOSES: " ) + sys->getName()));
			}
			if(!uncapFPS){
				auto afterTime = std::chrono::steady_clock::now();
				if((afterTime - beforeTime) < frameTime){
					std::chrono::nanoseconds sleep(((frameTime-std::chrono::milliseconds(2)) - (afterTime - beforeTime)));
					std::this_thread::sleep_for(sleep);
					while((afterTime - beforeTime) < frameTime){
						afterTime = std::chrono::steady_clock::now();
					}
				}
			}
		}
	}
	finish:
	if(measurePerf){
		fps.dumpTotalStats();
	}
	cleanExit();
};
