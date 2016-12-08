#include "d3dx12.h"
#include "Image.h"
#include "Helper.h"

#include "Graphics.h"

#define STANDARD_SIZE 16

// declare static vars
UINT Image::csuDescriptorSize;
UINT Image::rtvDescriptorSize;
CD3DX12_CPU_DESCRIPTOR_HANDLE Image::srvTexStart;
CD3DX12_CPU_DESCRIPTOR_HANDLE Image::rtvTexStart;
UINT Image::numResources, Image::numUploadedResources;
bool Image::startup = false;

ComPtr<ID3D12Resource> Image::texture2D, Image::instanceBuffer;

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
	
	if (!ilLoad(IL_RAW, filename))
		return false;

	width = ilGetInteger(IL_IMAGE_WIDTH);
	height = ilGetInteger(IL_IMAGE_HEIGHT);
	depth = ilGetInteger(IL_IMAGE_DEPTH);

	// convert the image to a usable format
	half4* tmpData = new half4[width * height * depth];
	bool pass = ilCopyPixels(0, 0, 0, width, height, depth,
		IL_RGBA,
		ilGetInteger(IL_IMAGE_TYPE), tmpData);

	if (!pass)
		cout << "Could not copy" << endl;
	
	// unbind the image and delete it
	ilBindImage(0);
	ilDeleteImage(imageID);

	data = new XMFLOAT4[width * height * depth];

	// transfer the halfs to full floats
	for (int i = 0; i < width * height * depth; i++)
	{
		data[i].x = XMConvertHalfToFloat(tmpData[i][0]);
		data[i].y = XMConvertHalfToFloat(tmpData[i][1]);
		data[i].z = XMConvertHalfToFloat(tmpData[i][2]);
		data[i].w = XMConvertHalfToFloat(tmpData[i][3]);
	}

	delete[] tmpData;

	// create the texture
	createTexture(device);

	return true;
}

void Image::createTexture(ID3D12Device* device)
{
	if (!startup)
	{
		D3D12_RESOURCE_DESC texture2DDesc = {};

		// startup happened
		startup = true;

		// setup the upload image
		// create the texture2D for upload reasons
		texture2DDesc.MipLevels = 1;
		texture2DDesc.Format = TEX_FORMAT;
		texture2DDesc.Width = STANDARD_SIZE;
		texture2DDesc.Height = STANDARD_SIZE;
		texture2DDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		texture2DDesc.DepthOrArraySize = 1;
		texture2DDesc.SampleDesc.Count = 1;
		texture2DDesc.SampleDesc.Quality = 0;
		texture2DDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
			D3D12_HEAP_FLAG_NONE,
			&texture2DDesc,
			D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr,
			IID_PPV_ARGS(&texture2D)
		));

		D3D12_SHADER_RESOURCE_VIEW_DESC mat2DDesc = {};
		mat2DDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		mat2DDesc.Format = texture2DDesc.Format;
		mat2DDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		mat2DDesc.Texture2D.MipLevels = texture2DDesc.MipLevels;

		CD3DX12_CPU_DESCRIPTOR_HANDLE srvPos(srvTexStart, uploadOffset, csuDescriptorSize);
		device->CreateShaderResourceView(texture2D.Get(), &mat2DDesc, srvPos);

		// create upload instance buffer
		ThrowIfFailed(device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(sizeof(Graphics::VOXEL_POS) * STANDARD_SIZE),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&instanceBuffer)));

		// fill the buffer
		UINT* v;
		instanceBuffer->Map(0, nullptr, (void**)&v);
		
		for (UINT i = 0; i < STANDARD_SIZE; i++)
		{
			v[i] = i;
		}

		instanceBuffer->Unmap(0, nullptr);
	}

	// set subresource num
	subresourceNum = 1;

	// create image
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.MipLevels = 1;
	textureDesc.Format = TEX_FORMAT;
	textureDesc.Width = width;
	textureDesc.Height = height;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	textureDesc.DepthOrArraySize = depth;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE3D;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_RENDER_TARGET,
		nullptr,
		IID_PPV_ARGS(&texture3D)
	));

	// create the rtv
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc;
	rtvDesc.Format = textureDesc.Format;
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
	rtvDesc.Texture3D.MipSlice = 0;
	rtvDesc.Texture3D.FirstWSlice = 0;
	rtvDesc.Texture3D.WSize = textureDesc.DepthOrArraySize;
	
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvTexStart, numResources, rtvDescriptorSize);
	device->CreateRenderTargetView(texture3D.Get(), &rtvDesc, rtvHandle);

	// create the srv for this texture
	D3D12_SHADER_RESOURCE_VIEW_DESC matDesc = {};
	matDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	matDesc.Format = textureDesc.Format;
	matDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
	matDesc.Texture3D.MipLevels = textureDesc.MipLevels;

	CD3DX12_CPU_DESCRIPTOR_HANDLE srvPos(srvTexStart, numResources, csuDescriptorSize);
	device->CreateShaderResourceView(texture3D.Get(), &matDesc, srvPos);

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(
			GetRequiredIntermediateSize(texture2D.Get(),
				0, 1)),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&texture2DUpload)
	));

	numResources++;
}

void Image::uploadTexture(Graphics* g, ID3D12GraphicsCommandList* commandList)
{
	for (UINT i = 0; i < depth; i++)
	{
		// reset the command allocator
		ThrowIfFailed(g->commandAllocator[g->frameIndex]->Reset());

		// reset the command list
		ThrowIfFailed(commandList->Reset(g->commandAllocator[g->frameIndex].Get(),
			g->renderPipelineSolidState.Get()));

		g->setupProceduralDescriptors();

		// set the pipeline
		commandList->SetPipelineState(g->uploadTexPipelineState.Get());

		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(rtvTexStart, numUploadedResources, rtvDescriptorSize);
		commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		commandList->IASetVertexBuffers(0, 1, &g->plainVB);
	
		cout << data[STANDARD_SIZE * STANDARD_SIZE * i].x << endl;
		D3D12_SUBRESOURCE_DATA textureData = {};
		textureData.pData = &data[STANDARD_SIZE * STANDARD_SIZE * i];
		textureData.RowPitch = STANDARD_SIZE * sizeof(XMFLOAT4);
		textureData.SlicePitch = textureData.RowPitch * STANDARD_SIZE;

		UpdateSubresources(commandList, texture2D.Get(),
			texture2DUpload.Get(), 0, 0, subresourceNum, &textureData);

		// transition phase of texture
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture2D.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE));

		// wait for shader
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::UAV(NULL));

		commandList->CopyBufferRegion(
			g->bufferSRV[Graphics::INSTANCE].Get(),
			0,
			instanceBuffer.Get(),
			sizeof(UINT) * i,
			sizeof(UINT)
		);

		commandList->DrawInstanced(_countof(g->plainVerts), 1, 0, 0);

		// wait for shader
		// transition phase of texture
		commandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(texture2D.Get(),
			D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST));

		// run the commands
		ThrowIfFailed(commandList->Close());
		ID3D12CommandList* commandLists[] = { g->commandList.Get() };
		g->commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		// wait on the gpu
		g->waitForGpu();
	}

	// update num of resources
	numUploadedResources++;
}

void Image::initDevil()
{
	ilInit();
}

void Image::setBase(CD3DX12_CPU_DESCRIPTOR_HANDLE srvTexStartPass, UINT csuDescriptorSizePass,
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvTexStartPass, UINT rtvDescriptorSizePass)
{
	// copy over the start pos for the texture base
	srvTexStart = srvTexStartPass;
	csuDescriptorSize = csuDescriptorSizePass;

	rtvTexStart = rtvTexStartPass;
	rtvDescriptorSize = rtvDescriptorSizePass;

	// reset number of resources
	numResources = 0;
	numUploadedResources = 0;
}

void Image::setPipeline(ComPtr<ID3D12PipelineState> pipeline)
{
	pipelineState = pipeline;
}
