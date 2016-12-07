#include "d3dx12.h"
#include "Image.h"
#include "Helper.h"

#include "Graphics.h"

#define TEX_FORMAT DXGI_FORMAT_R32G32B32A32_FLOAT

// declare static vars
UINT Image::descriptorSize;
CD3DX12_CPU_DESCRIPTOR_HANDLE Image::srvTexStart;
UINT Image::numResources;

Image::Image()
{

}

void Image::deleteImage()
{
	// delete the data
	if (data)
	{
		delete[] data;

		// TODO: figure out how to replace buffer
		//uploadBufferSize -= intermediateSize;

		// TODO: cleanup d3d
	}
}

bool Image::loadImage(ID3D12Device* device, const wchar_t* filename)
{
	ILuint imageID = -1;

	ilGenImages(1, &imageID);
	ilBindImage(imageID);

	if (!ilLoadImage(filename))
		return false;

	width = ilGetInteger(IL_IMAGE_WIDTH);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	depth = ilGetInteger(IL_IMAGE_DEPTH);

	// convert the image to a usable format
	data = new float[width * height * depth * 4];
	ilCopyPixels(0, 0, 0, width, height, 1, IL_RGBA,
		IL_FLOAT, data);

	// unbind the image and delete it
	ilBindImage(0);
	ilDeleteImage(imageID);

	// create the texture
	createTexture(device);

	return true;
}

void Image::createTexture(ID3D12Device* device)
{
	// create image
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = INDEX_FORMAT;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	textureDesc.DepthOrArraySize = depth;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture3D)
	));
	
	// add to upload buffer
	subresourceNum = 1;// textureDesc.DepthOrArraySize * textureDesc.MipLevels;
	cout << GetRequiredIntermediateSize(texture3D.Get(),
		0, subresourceNum) << endl;
	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(
			GetRequiredIntermediateSize(texture3D.Get(),
				0, subresourceNum)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texture3DUpload)
	));
	
	// create the srv for this texture
	D3D12_SHADER_RESOURCE_VIEW_DESC matDesc = {};
	matDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	matDesc.Format = textureDesc.Format;
	matDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	matDesc.Texture3D.MipLevels = textureDesc.MipLevels;

	// set resource location
	prevResourceNum = numResources;

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvPos(srvTexStart, prevResourceNum, descriptorSize);
	device->CreateShaderResourceView(texture3D.Get(), &matDesc, srvPos);

	// update num of resources
	numResources++;
}

void Image::uploadTexture(ID3D12GraphicsCommandList* commandList)
{
	D3D12_SUBRESOURCE_DATA textureData = {};
	textureData.pData = data;
	textureData.RowPitch = width * 16;
	textureData.SlicePitch = textureData.RowPitch * height;
	
	UpdateSubresources(commandList, texture3D.Get(),
		texture3DUpload.Get(), 0, 0, subresourceNum, &textureData);

	// transition phase of texture
	commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture3D.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));
}

ILfloat* Image::getData()
{
	return data;
}

void Image::initDevil()
{
	ilInit();
}

void Image::setSRVBase(CD3DX12_CPU_DESCRIPTOR_HANDLE srvTexStartPass, UINT descriptorSizePass)
{
	// copy over the start pos for the texture base
	srvTexStart = srvTexStartPass;
	descriptorSize = descriptorSizePass;

	// reset number of resources
	numResources = 0;
}