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
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>
#include <cstdlib>
#include <cmath>
#include <cstdint>
#include <sstream>
#include "vendored/SDL3-3.2.16/include/SDL3/SDL.h"
#include "vendored/json/include/nlohmann/json.hpp"
#include "Modules/Chip8/chip8.h"
#include "Modules/NES/nes.h"
#include "Modules/AppleII/appleii.h"
#include "Modules/CPUTest/cputest.h"
#include "Modules/XO-Chip/xochip.h"

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

int main(int argc, char* argv[]){
	const bool* keysPressed = SDL_GetKeyboardState(nullptr);
	bool coreSet = false;
	std::vector<bool> keyState;
	bool debugPause = false;
	bool dbgPauseEnable = true;
	Module *sys;
	WindowArgs *winArgs;
	double targetFPS = 60.0;
	bool run = true;
	std::string *arguments = new std::string[argc];
	try{
		if(std::string(argv[1]) == "--cfg"){
			arguments = new std::string[6];
			std::string desiredCore;
			std::string confLocation(argv[2]);
			std::ifstream config(confLocation);
			json settings = json::parse(config);
			arguments[0] = "--core";
			if(argc < 4){
				std::cout << "GURU MEDITATION no core set\n";
				return 1;
			}
			desiredCore = std::string(argv[3]);
			arguments[1] = desiredCore;
			arguments[2] = "-f";
			arguments[3] = settings["cores"][desiredCore]["file"];
			arguments[4] = "-sc";
			int scale = settings["cores"][desiredCore]["scale"];
			arguments[5] = std::to_string(scale);
		}else{
			for(int i = 1; i < argc; i++){
				arguments[i-1] = std::string(argv[i]);
			}
		}
		if(arguments[0] == "--core"){
			if(argc >= 3){
				if(arguments[1] == "chip8"){
					coreSet = true;
					sys = new Cores::Chip8::System(argc, arguments);
				}
				if(arguments[1] == "nes"){
					coreSet = true;
					sys = new Cores::Nes::System(argc, arguments);
				}
				if(arguments[1] == "xochip"){
					coreSet = true;
					sys = new Cores::Xochip::System(argc, arguments, 1000);
				}
				if(arguments[1] == "xochip-fast"){
					coreSet = true;
					sys = new Cores::Xochip::System(argc, arguments, 200000);
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
			while(!arguments[b].empty()){
				if(arguments[b] == "-sc"){
					b++;
					if(std::stoi(arguments[b]) < 1){
						std::cout << "GURU MEDITATION invalid scale factor\n";
					}else{
						scaleDisplay(winArgs, std::stoi(arguments[b]));
					}
				}
				if(arguments[b] == "-vol"){
					b++;
					sys -> setVolume(std::stoi(arguments[b]));
				}
				if(arguments[b] == "--debug"){
					sys -> dbg = true;
					debugPause = true;
				}
				if(arguments[b] == "--writelog"){
					b++;
					sys -> setLogOutput(arguments[b]);
				}
				if(arguments[b] == "--dbgspeed"){
					b++;
					sys -> debugStep = stoi(arguments[b]);
				}
				if(arguments[b] == "--breakpoint"){
					b++;
					if(stoi(arguments[b]) < 1){
						std::cout << "GURU MEDITATION invalid breakpoint\n";
					}else{
						sys -> breakpointActive = true;
						sys -> setPcBreakpoint (stoi(arguments[b]));
					}
				}
				b++;
			}
		}
	}catch(json::out_of_range){
		std::cout << "GURU MEDITATION invalid argument\n";
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
							break;
						case SDLK_SPACE:
							debugPause = false;
							break;
						case SDLK_UP:
							sys -> debugStep = fmin(pow(2, 31), sys -> debugStep * 2);
							std::cout << "DBG SPEED: " << +(sys -> debugStep) << "\n";
							break;
						case SDLK_DOWN:
							sys -> debugStep = fmax(1, sys -> debugStep / 2);
							std::cout << "DBG SPEED: " << +(sys -> debugStep) << "\n";
							break;
					}
				}
				if(event.key.key == SDLK_ESCAPE){
					SDL_DestroyWindow(mainWindow);
					SDL_Quit();
					run = false;
					break;
				}
				sys -> keyRelease = false;
				break;
			case SDL_EVENT_KEY_UP:
				sys -> keyRelease = true;
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
			SDL_PutAudioStreamData(audioOut, sys -> playAudio(), 2*(winArgs -> getSampleFrequency()/targetFPS)*(winArgs -> getAudioChannels()));
		}
		/*ulong currentTime = SDL_GetTicksNS();
		while(currentTime < (time + (1000000000/targetFPS))){
			currentTime = SDL_GetTicksNS();
		}*/
		//The framerate cap code above is commented out because it is extremely slow. Need a better solution.
		updateDisplay(&sys -> getFramebuffer(), winArgs);
	}
};
