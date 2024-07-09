#pragma once

#include "CommonFunctions.h" // need for uchar

class Context
{
private:
    uchar ch[4];
public:
    Context()
    {
        ch[0] = ch[1] = ch[2] = ch[3] = 0;
    }

    inline void add(uchar c)
    {       
       // memmove(ch + 1, ch, 3);
        ch[3] = ch[2];
        ch[2] = ch[1];
        ch[1] = ch[0];        
        ch[0] = c;
    }

    inline uchar* getCtx()
    {
        return ch;
    }

};