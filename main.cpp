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
#include "vendored/SDL3-3.2.16/include/SDL3/SDL.h"
#include "vendored/json/include/nlohmann/json.hpp"
#include "vendored/flags/include/flags.h"
#include "Modules/Chip8/chip8.h"
#include "Modules/NES/nes.h"
#include "Modules/AppleII/appleii.h"
#include "Modules/CPUTest/cputest.h"
#include "Modules/XO-Chip/xochip.h"
#include "Modules/SCHIP/schip.h"

using namespace Cores;
using json = nlohmann::json;

SDL_Window* mainWindow;
SDL_Renderer* render;
SDL_Texture* frameBuffer;
SDL_AudioStream* audioOut;
SDL_AudioSpec sampleSpec;
void sdl_setup(WindowArgs *args){
	if(!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO)){
		std::cout << "GURU MEDITATION sdl init %s\n";
	}
	mainWindow = SDL_CreateWindow("MOSES", args -> getX(), args -> getY(), 0);
	render = SDL_CreateRenderer(mainWindow, NULL);
	if(SDL_SetRenderVSync(render, 1) == false ){
		std::cout << "GURU MEDITATION vsync %s\n";
	}
	frameBuffer = SDL_CreateTexture(render, SDL_PIXELFORMAT_BGRA32, SDL_TEXTUREACCESS_STREAMING, args -> getX(), args -> getY());
	if(frameBuffer == NULL){
		std::cout << "GURU MEDITATION null texture\n";
	}
	SDL_SetTextureBlendMode(frameBuffer, SDL_BLENDMODE_NONE);
	SDL_SetTextureScaleMode(frameBuffer, SDL_SCALEMODE_NEAREST);
	sampleSpec.freq = args -> getSampleFrequency();
	sampleSpec.channels = args -> getAudioChannels();
	sampleSpec.format = SDL_AUDIO_S16LE;
	audioOut = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr, nullptr, nullptr);
	SDL_SetAudioStreamFormat(audioOut, &sampleSpec, &sampleSpec);
	SDL_ResumeAudioStreamDevice(audioOut);
}

void updateDisplay(std::vector<uint32_t> *pixels, WindowArgs *args){
	int scale = args -> scaleFactor;
	int w = args -> getX();
	SDL_SetRenderScale(render, scale, scale);
	SDL_UpdateTexture(frameBuffer, nullptr, pixels -> data(), w*4);
	SDL_RenderTexture(render, frameBuffer, nullptr, nullptr);
	SDL_RenderPresent(render);
}

void scaleDisplay(WindowArgs* args, int scale){
	args -> scaleFactor = scale;
	if(!SDL_SetWindowSize(mainWindow, args -> getX()*scale, args -> getY()*scale)){
		std::cout << "GURU MEDITATION window resize\n";
	}
}

std::map<std::string, std::string> cfgParse(std::string file, std::string core){
	const int globalArgCount = 3;
	std::array<std::string, globalArgCount> validSettings = {"scale", "volume", "file"};
	std::map<std::string, std::string> ret;
	std::ifstream config(file);
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
	for(const auto& [key, value] : ret){
		std::cout << key << value << std::endl;
	}
	return ret;
}

std::map<std::string, std::string> argParse(int argc, const char** argv){
	
}

int main(int argc, char* argv[]){
	const bool* keysPressed = SDL_GetKeyboardState(nullptr);
	std::map<std::string, std::string> coreSettings;
	bool coreSet = false;
	std::vector<bool> keyState;
	bool debugPause = false;
	bool dbgPauseEnable = true;
	std::unique_ptr<Module> sys;
	WindowArgs *winArgs;
	double targetFPS = 60.0;
	bool run = true;
	constexpr bool perfTimer = false;
	int tickedFrames = 0;
	int tickLimit = 3600;
	const flags::args arguments(argc, argv);
	auto configFileOpt = arguments.get<std::string>("cfg");
	auto coreOpt = arguments.get<std::string>("core");
	try{
		if(configFileOpt.has_value()){
			std::string core = coreOpt.value_or("default");
			std::string configFile = configFileOpt.value();
			std::map<std::string, std::string> cfg = cfgParse(configFile, core);
		}else{
			
		}
	}catch(const std::exception& e){
		std::string what = e.what();
		std::cout << "GURU MEDITATION " << ((what == "std::exception") ? "unknown" : what) << "\n";
	}
	std::string *argument = new std::string[argc];
	try{
		/*if(std::string(argv[1]) == "--cfg"){
			argument = new std::string[6];
			std::string desiredCore;
			std::string confLocation(argv[2]);
			std::ifstream config(confLocation);
			json settings = json::parse(config);
			argument[0] = "--core";
			if(argc < 4){
				std::cout << "GURU MEDITATION no core set\n";
				return 1;
			}
			desiredCore = std::string(argv[3]);
			argument[1] = desiredCore;
			argument[2] = "-f";
			argument[3] = settings["cores"][desiredCore]["file"];
			argument[4] = "-sc";
			int scale = settings["cores"][desiredCore]["scale"];
			argument[5] = std::to_string(scale);
		}else{*/
			for(int i = 1; i < argc; i++){
				argument[i-1] = std::string(argv[i]);
			}
	//	}
		if(argument[0] == "--core"){
			if(argc >= 3){
				if(argument[1] == "chip8"){
					coreSet = true;
					sys = std::make_unique<Cores::Chip8::System>(argc, argument);
				}
				if(argument[1] == "schip"){
					coreSet = true;
					sys = std::make_unique<Cores::Schip::System>(argc, argument);
				}
				if(argument[1] == "nes"){
					coreSet = true;
					sys = std::make_unique<Cores::Nes::System>(argc, argument);
				}
				if(argument[1] == "xochip"){
					coreSet = true;
					sys = std::make_unique<Cores::Xochip::System>(argc, argument, 1000);
				}
				if(argument[1] == "xochip-fast"){
					coreSet = true;
					sys = std::make_unique<Cores::Xochip::System>(argc, argument, 200000);
				}
				if(!(sys -> checkInit())){
					SDL_Quit();
					run = false;
				}else{
					sys -> addKey(keysPressed);
					winArgs = sys -> getWindowArgs();
					sdl_setup(winArgs);
					SDL_SetWindowTitle(mainWindow, ("MOSES: " + sys -> getName()).c_str());
				}
			}
		}
		if(!coreSet){
			std::cout << "GURU MEDITATION no core set\n";
			SDL_Quit();
			run = false;
		}else{
			int b = 0;
			while(!argument[b].empty()){
				if(argument[b] == "-sc"){
					b++;
					if(std::stoi(argument[b]) < 1){
						std::cout << "GURU MEDITATION invalid scale factor\n";
					}else{
						scaleDisplay(winArgs, std::stoi(argument[b]));
					}
				}
				if(argument[b] == "-vol"){
					b++;
					sys -> setVolume(std::stoi(argument[b]));
				}
				if(argument[b] == "--debug"){
					sys -> dbg = true;
					debugPause = true;
				}
				if(argument[b] == "--writelog"){
					b++;
					sys -> setLogOutput(argument[b]);
				}
				if(argument[b] == "--dbgspeed"){
					b++;
					sys -> debugStep = stoi(argument[b]);
				}
				if(argument[b] == "--breakpoint"){
					b++;
					if(stoi(argument[b]) < 1){
						std::cout << "GURU MEDITATION invalid breakpoint\n";
					}else{
						sys -> breakpointActive = true;
						sys -> setPcBreakpoint (stoi(argument[b]));
					}
				}
				b++;
			}
		}
	}catch(json::out_of_range){
		std::cout << "GURU MEDITATION invalid argument\n";
	}
	uint32_t bufferSize = (winArgs -> getSampleFrequency()/targetFPS)*(winArgs -> getAudioChannels());
	int16_t *bufferSamples = new int16_t[bufferSize];
	for(int i = 0; i < bufferSize; i++){
		bufferSamples[i] = 0;
	}
	while(run){
		//ulong time = SDL_GetTicksNS();
		SDL_Event event;
		while(SDL_PollEvent(&event)){
			switch( event.type ){
			case SDL_EVENT_KEY_DOWN:
				if(sys -> dbg){
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
							sys -> debugStep = fmin(pow(2, 31), sys -> debugStep * 2);
							std::cout << "DBG SPEED " << +(sys -> debugStep) << "\n";
							break;
						case SDLK_DOWN:
							sys -> debugStep = fmax(1, sys -> debugStep / 2);
							std::cout << "DBG SPEED " << +(sys -> debugStep) << "\n";
							break;
					}
				}
				if(event.key.key == SDLK_ESCAPE){
					SDL_DestroyWindow(mainWindow);
					SDL_Quit();
					run = false;
					break;
				}
				break;
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				SDL_DestroyWindow(mainWindow);
				SDL_Quit();
				run = false;
				break;
			default:
				break;
			}
		}
		if(!run){
			break;
		}
		if(sys -> dbg){
			if(!debugPause && dbgPauseEnable){
				sys -> debugCycle();
				debugPause = true;
			}else if(!dbgPauseEnable){
				sys -> debugCycle();
			}
		}else{
			sys -> runCycle();
			SDL_PutAudioStreamData(audioOut, bufferSamples, (SDL_GetAudioStreamAvailable(audioOut) < bufferSize) ? 2*bufferSize : 0);
			SDL_PutAudioStreamData(audioOut, sys -> playAudio(), 2*(winArgs -> getSampleFrequency()/targetFPS)*(winArgs -> getAudioChannels()));
		}
		/*ulong currentTime = SDL_GetTicksNS();
		while(currentTime < (time + (1000000000/targetFPS))){
			currentTime = SDL_GetTicksNS();
		}*/
		//The framerate cap code above is commented out because it is extremely slow. Need a better solution.
		updateDisplay(&sys -> getFrameBuffer(), winArgs);
		if(perfTimer){
			tickedFrames++;
			if(tickedFrames > tickLimit){
				SDL_DestroyWindow(mainWindow);
				SDL_Quit();
				run = false;
			}
		}
	}
};
