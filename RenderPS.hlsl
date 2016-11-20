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
	float ratio;

	ratio = densityTexture.Sample(
		nearestSample, float3(input.position.xy / 800.f, 0));
	//(ratio < 0.f) ? float4(1, 0, 0, 1) : float4(0, 1, 0, 1);

	//ratio = ratio == 0.f ? 1.f : 0.f;

	return float4(ratio, ratio, ratio, 1.0f);
}