#ifndef _IMAGE_H_
#define _IMAGE_H_

#include "d3dx12.h"
#include <d3d12.h>
#include <DirectXPackedVector.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <wrl.h>
#include <IL/il.h>

#include <iostream>

using namespace DirectX;
using namespace DirectX::PackedVector;
using namespace Microsoft::WRL;
using namespace std;

struct ID3D12PipelineState;

class Graphics;

struct half4
{
	HALF x = 0, y = 0, z = 0, w = 0;

	HALF& operator[] (unsigned int i)
	{
		switch (i)
		{
		case 0:
			return x;
		case 1:
			return y;
		case 2:
			return z;
		case 3:
			return w;
		}

		return x;
	}
};

class Image
{
public:

	const static DXGI_FORMAT TEX_FORMAT = DXGI_FORMAT_R32G32B32A32_FLOAT;

	Image();

	void deleteImage();

	bool loadImage(ID3D12Device* device, const wchar_t* filename);

	void createTexture(ID3D12Device* device);

	void uploadTexture(Graphics* g, ID3D12GraphicsCommandList* commandList);

	static void initDevil();

	static void setBase(CD3DX12_CPU_DESCRIPTOR_HANDLE srvTexStartPass, UINT csuDescriptorSizePass,
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvTexStartPass, UINT rtvDescriptorSizePass);

	void setPipeline(ComPtr<ID3D12PipelineState> pipeline);

private:

	ComPtr<ID3D12PipelineState> pipelineState;

	ComPtr<ID3D12Resource> texture3D, texture2DUpload;

	static ComPtr<ID3D12Resource> texture2D;

	D3D12_VIEWPORT viewport;
	D3D12_RECT scissorRect;

	int width;
	int height;
	int depth;

	unsigned int subresourceNum;

	XMFLOAT4* data = nullptr;

	static UINT csuDescriptorSize, rtvDescriptorSize;
	static CD3DX12_CPU_DESCRIPTOR_HANDLE srvTexStart, rtvTexStart;
	static UINT numResources, numUploadedResources;
	static bool startup;
};

#endif