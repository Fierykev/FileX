#ifndef _FIND_Y_
#define _FIND_Y_

#include <DirectXMath.h>
#include <wrl.h>
#include <Windows.h>

#include <iostream>
#include "Helper.h"
#include "Shader.h"
#include "Window.h"
#include "d3dx12.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace std;

class Graphics;

class FindY
{
public:

	void setup();
	float FindY::findY(XMVECTOR pos);

private:

	ComPtr<ID3D12GraphicsCommandList> commandList;

	void sampleDensity();
	void searchTerrain();
	void findYRender();

	ComPtr<ID3D12PipelineState> sampleDensityPipelineState, searchTerrainPipelineState;
	ComPtr<ID3D12Resource> yposMap;
};

#endif