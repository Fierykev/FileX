struct PS_INPUT
{
	float4 position : SV_POSITION;
	float4 worldPosition : TEXCOORD;
};

float main(PS_INPUT input) : SV_TARGET0
{
	// eval the density function
	// TODO: add better density function
	return .5f;// -input.worldPosition.y;
}