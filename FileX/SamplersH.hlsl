#ifndef _SAMPLERS_H_
#define _SAMPLERS_H_

SamplerState nearestClampSample : register(s0);
SamplerState linearClampSample : register(s1);
SamplerState linearRepeatSample : register(s2);
SamplerState nearestRepeatSample : register(s3);

#endif