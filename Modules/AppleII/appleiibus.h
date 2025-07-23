#pragma once

struct{
	uint8_t mem[49152];
	
	uint8_t read(uint16_t addr){
		return mem[addr];
	}
	
	void write(int8_t val, uint16_t addr){
		mem[addr] = val;
	}
} appleiibus;
