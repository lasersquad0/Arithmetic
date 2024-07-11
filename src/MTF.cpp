#include "MTF.h"


void MTF::Encode(uint8_t* inBuf, uint8_t* outBuf, uint32_t size)
{
	//Modification of MTF: MTF is defined by the following modified rules: if the next
	//symbol has the current number z, then after its coding we change the number as
	//follows: if z > 1 then shift z to position 1 else shift z to position 0.

	fillAlb();

	for (uint32_t i = 0; i < size; i++)
	{
		uint8_t x = inBuf[i];
		if (alb[0] == x) // symbol is on top position, do nothing and transfer 0 to dest array
		{
			outBuf[i] = 0;
		}
		else if (alb[1] == x) // symbol is on position 1, swap it with top symbol and transfer 1
		{
			alb[1] = alb[0];
			alb[0] = x;
			outBuf[i] = 1;
		}
		else // if symbol is NOT on position 1 or 0, do standard MTF
			outBuf[i] = (uint8_t)shiftAlb(1, x);
	}
}

void MTF::Decode(uint8_t* inBuf, uint8_t* outBuf, uint32_t size)
{
	//byte[] alb = buildAlfabet(c.mtfblock);
	fillAlb();

	for (uint32_t i = 0; i < size; i++)
	{
		uint8_t x = inBuf[i];
		outBuf[i] = alb[x];
		if (x == 0)
			continue; // if x==0, no more actions required. alb stays as is.
		if (x == 1)
		{
			alb[1] = alb[0];
			alb[0] = outBuf[i];
		}
		else
			shiftAlb(1, outBuf[i]);
	}
}


int MTF::shiftAlb(int off, uint8_t x)
{
	uint8_t tmp1 = alb[off];
	alb[off] = x;

	int j = off;
	// shift alfabet one-by-one till symbol x met
	while (tmp1 != x)
	{
		j++;
		uint8_t tmp2 = tmp1;
		tmp1 = alb[j];
		alb[j] = tmp2;
	}

	return j;
}

void MTF::fillAlb()
{
	// use full (256 symbols) alphabet
	for (int i = 0; i < ALFABET_SIZE; i++)
	{
		alb[i] = (uint8_t)i;
	}

}
