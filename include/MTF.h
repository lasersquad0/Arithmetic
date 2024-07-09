#pragma once
#include <cstdint>


class MTF
{
public:
	void Encode(uint8_t* inBuf, uint8_t* outBuf, uint32_t size);
	void Decode(uint8_t* inBuf, uint8_t* outBuf, uint32_t size);
private:
	static const int ALFABET_SIZE = 256;
	uint8_t alb[ALFABET_SIZE];
	void fillAlb();
	int shiftAlb(int off, uint8_t x);
};

