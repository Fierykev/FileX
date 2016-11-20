struct PS_INPUT
{
	float4 position : SV_POSITION;
};

// TMP
Texture3D<float> densityTexture : register(t0);
SamplerState nearestSample : register(s0);

float4 main(PS_INPUT input) : SV_TARGET
{
	// depth render
	float ratio = input.position.z / 100.f;

	ratio = densityTexture.SampleLevel(
		nearestSample, float3(0, 0, 0), 0);

	return (ratio != .5f) ? float4(1, 0, 0, 1) : float4(0, 1, 0, 1);//float4(ratio, ratio, ratio, 1.0f);
}