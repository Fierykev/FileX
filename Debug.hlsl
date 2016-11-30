#ifndef DEBUG_H
#define DEBUG_H

#define MAX_INT (~0)

//#define DEBUG

#ifdef DEBUG

RWStructuredBuffer<bool> debug : register(u1);

#endif

#endif