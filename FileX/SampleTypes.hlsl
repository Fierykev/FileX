#ifndef _SAMPLE_TYPES_H_
#define _SAMPLE_TYPES_H_

static const float TEX_SIZE = 16.f;
static const float2 INV_TEX_SIZE = float2(1.f / TEX_SIZE, 0);

SamplerState repeatSampler : register(s1);

// user function must have this
float4 evalTexID(uint texID, float3 uvw);

// low

float4 lowUnsigned(uint texID, float3 uvw)
{
	return evalTexID(texID, uvw);
}

float4 lowSigned(uint texID, float3 uvw)
{
	return lowUnsigned(texID, uvw) * 2.f - 1.f;
}

// medium

float4 mediumUnsigned(uint texID, float3 uvw)
{
	// smooth input
	float3 tmp1 = frac(uvw * TEX_SIZE + .5);
	float3 delta = (3 - 2 * tmp1) * tmp1 * tmp1;
	float3 sampleLoc = uvw + (delta - tmp1) / TEX_SIZE;

	return lowUnsigned(texID, uvw);
}

float4 mediumSigned(uint texID, float3 uvw)
{
	return mediumUnsigned(texID, uvw) * 2.f - 1.f;
}

// high

float highUnsigned(uint texID, float3 uvw, float soften = 1)
{
	float3 tmp1 = floor(uvw * TEX_SIZE) * INV_TEX_SIZE.x;
	float3 delta = (uvw - tmp1) * TEX_SIZE;
	delta = lerp(delta, delta * delta * (3.f - 2.f * delta), soften);

	// 2 sample style
	float4 sample1 = lowUnsigned(texID, tmp1).zxyw;
	float4 sample2 = lowUnsigned(texID, tmp1 + INV_TEX_SIZE.xyy).zxyw;

	float4 lerp1 = lerp(sample1, sample2, delta.xxxx);
	float2 lerp2 = lerp(lerp1.xy, lerp1.zw, delta.yy);
	float lerp3 = lerp(lerp2.x, lerp2.y, delta.z);

	return lerp3;
}

float highSigned(uint texID, float3 uvw, float soften = 1)
{
	return highUnsigned(texID, uvw, soften) * 2.f - 1.f;
}

// misc

float snap(float a, float b)
{
	float tmp = (.5 < a) ? 1 : 0;
	float tmp2 = 1.f - tmp * 2.f;

	return tmp * tmp2 * pow((tmp + tmp2 * a) * 2, b) * .5;
}

#endif