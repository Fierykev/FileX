Texture2D altCol0 : register(t10);
Texture2D bump0 : register(t11);

SamplerState repeatSampler : register(s1);

const static float3 distort = float3(1, 12, 1);
const static float scale = .003f;

Texture3D<float4> noise0 : register(t4);
Texture3D<float4> noise1 : register(t5);
Texture3D<float4> noise2 : register(t6);

float3 genBaseCol_(float3 pos, float3 normal)
{
	float4 noise = noise0.SampleLevel(
		repeatSampler, pos * .00047 * distort, 0);

	// calculate the uv coords
	float2 uv;
	uv.x = noise.x * 1.2 - .1;
	uv.y = pos.y *.03 + .5;

	// get the color based on altitude
	float3 col = altCol0.SampleLevel(
		repeatSampler,
		float2(saturate(uv.x), uv.y), 0
	).xyz;

	// sample a second time
	uv.x += noise.z * .13 - .23 * normal.y;
	uv.y *= -7.45;

	float3 col2 = altCol0.SampleLevel(
		repeatSampler,
		float2(saturate(uv.x), uv.y), 0
	).xyz;

	col = lerp(col, col2, .326);

	return col;
}

float3 genBaseCol(float3 pos, inout float3 normal)
{
	float3 bw = abs(normal);
	bw = (bw - .2) * 7;
	bw = max(bw, 0);
	bw /= (bw.x + bw.y + bw.z).xxx;

	// get uv
	float2 c0 = pos.yz * scale;
	float2 c1 = pos.zx * scale;
	float2 c2 = pos.xy * scale;

	// color
	float4 col0 = altCol0.Sample(repeatSampler, c0);
	float4 col1 = altCol0.Sample(repeatSampler, c1);
	float4 col2 = altCol0.Sample(repeatSampler, c2);

	// factor in bump map
	float2 b0 = bump0.Sample(repeatSampler, c0).xy - .5;
	float2 b1 = bump0.Sample(repeatSampler, c1).xy - .5;
	float2 b2 = bump0.Sample(repeatSampler, c2).xy - .5;

	float3 bump0 = float3(0, b0.xy);
	float3 bump1 = float3(b1.y, 0, b1.x);
	float3 bump2 = float3(b2.xy, 0);

	// blend colors
	float4 blendCol =
		col0 * bw.xxxx +
		col1 * bw.yyyy +
		col2 * bw.zzzz;
	float3 bumpVec =
		bump0 * bw.xxx +
		bump1 * bw.yyy +
		bump2 * bw.zzz;

	// combine col with vec

	// add in lighting
	normal = normalize(normal + bumpVec);

	return blendCol.xyz;
}