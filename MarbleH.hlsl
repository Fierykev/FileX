#ifndef _MARBLE_H_
#define _MARBLE_H_

const static float freq = .032;

float3 genMarble(float3 origCol, float3 pos)
{
	float3 tmp = float3(0, 0, 0);
	tmp += (noise0.SampleLevel(repeatSampler, pos * freq * .324, 0).xyz * 2.f - 1) * pow(.43, -1);
	tmp += (noise1.SampleLevel(repeatSampler, pos * freq * 1.23, 0).xyz * 2.f - 1);
	tmp += (noise2.SampleLevel(repeatSampler, pos * freq * 1.83, 0).xyz * 2.f - 1) * pow(.43, 1);
	
	float3 warp = pos + tmp;

	float isMarb = pow(saturate(sin(warp.y) * 1.1), 3.f);

	isMarb = saturate(isMarb * 24.f - 22.f);
	isMarb = pow(isMarb, 4);

	float3 marble_col = 1;// float3(.6, .3, .6);
	float3 col = lerp(origCol, marble_col, isMarb);

	return col;
}

#endif