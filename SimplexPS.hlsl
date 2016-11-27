struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 worldPosition : TEXCOORD;
};

float hash(float num)
{
	return frac(sin(num) * 43758.5453);
}

// standard simplex noise
// http://stackoverflow.com/questions/15628039/simplex-noise-shader
float noise(float3 pos)
{
	float3 fl = floor(pos);
	float3 frc = frac(pos);

	frc = frc * frc * (3.f - 2.f * frc);
	float tmp = pos.x + pos.y * 57.f + 113.f * pos.z;

	return
		lerp(
			lerp(
				lerp(hash(tmp), hash(tmp + 1.f), frc.x),
				lerp(hash(tmp + 57.f), hash(tmp + 58.f), frc.x), frc.y),
			lerp(
				lerp(hash(tmp + 113.f), hash(tmp + 114.f), frc.y),
				lerp(hash(tmp + 170.f), hash(tmp + 171.f), frc.x), frc.y), frc.z);
}

uint main(PS_INPUT input) : SV_TARGET0
{
	return noise(input.worldPosition.xyz);
}