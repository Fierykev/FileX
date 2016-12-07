#ifndef IMAGE_H
#define IMAGE_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <IL/il.h>

#include <iostream>

using namespace Microsoft::WRL;
using namespace std;

class Image
{
public:

	Image();

	void deleteImage();

	bool loadImage(ID3D12Device* device, const wchar_t* filename);

	void createTexture(ID3D12Device* device);

	void uploadTexture(ID3D12GraphicsCommandList* commandList);

	ILfloat* getData();

	static void initDevil();

	static void setSRVBase(
		CD3DX12_CPU_DESCRIPTOR_HANDLE srvTexStartPass,
		UINT descriptorSizePass);

private:

	ComPtr<ID3D12Resource> texture3D, texture3DUpload;

	int width;
	int height;
	int depth;

	unsigned int prevResourceNum, subresourceNum;

	float* data = nullptr;

	static UINT descriptorSize;
	static CD3DX12_CPU_DESCRIPTOR_HANDLE srvTexStart;
	static UINT numResources;
};

#endif