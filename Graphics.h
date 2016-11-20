#ifndef GRPAHICS_H
#define GRAPHICS_H

#define NUM_VOXELS_X 10
#define NUM_VOXELS_Y 10
#define NUM_VOXELS_Z 10
#define NUM_VOXELS (NUM_VOXELS_X * NUM_VOXELS_Y * NUM_VOXELS_Z)

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "Manager.h"

using namespace Microsoft::WRL;
using namespace DirectX;

#define VOXEL_SIZE 32
#define VOXEL_SIZE_P1 33
#define SRVS_PER_FRAME 2

#define MAX_BUFFER_SIZE 5 * VOXEL_SIZE_P1 * VOXEL_SIZE_P1

#define DENSITY_FORMAT DXGI_FORMAT_R32_FLOAT

#define CAM_DELTA .1f

class Graphics : public Manager
{
public:
	// use Manager's constructor
	Graphics(std::wstring title, unsigned int width, unsigned int height);

	virtual void onInit();
	virtual void onUpdate();
	virtual void onRender();
	virtual void onDestroy();
	virtual void onKeyDown(UINT8 key);
	virtual void onKeyUp(UINT8 key);

private:
	// methods
	void loadPipeline();
	void loadAssets();
	void populateCommandList();
	void moveToNextFrame();
	void waitForGpu();

	// added methods
	void setupProceduralDescriptors();
	void renderDensity(XMUINT3 voxelPos);
	void renderOccupied(XMUINT3 voxelPos);
	void renderGenVerts(XMUINT3 voxelPos);
	void renderVertexMesh(XMUINT3 voxelPos);

	enum ComputeShader : UINT32
	{
		CSTMP = 0,
		CS_COUNT
	};

	// setup the srv and uav enums
	enum BVCB : UINT32
	{
		CB_VOXEL_POS = 0,
		CBV_COUNT = 1 // TMP
	};

	enum BVSRV : UINT32
	{
		DENSITY_TEXTURE = 0,
		SRV_COUNT
	};

	enum BVUAV : UINT32
	{
		NEAREST_SAMPLE = 0,
		UAV_COUNT
	};

	enum BVSAMPLER :UINT32
	{
		NEAREST_SAMPLER = 0,
		SAMPLER_COUNT
	};

	// static globals
	static const UINT numFrames = 2;

	// buffers
	ComPtr<ID3D12Resource> bufferCB[CBV_COUNT], 
		bufferSRV[SRV_COUNT],
		bufferCS[UAV_COUNT],
		zeroBuffer, plainVCB,
		vertexBuffer[NUM_VOXELS];

	D3D12_VERTEX_BUFFER_VIEW plainVB;

	struct PLAIN_VERTEX
	{
		XMFLOAT3 position;
		XMFLOAT2 texcoord;
		UINT svInstance;
	};

	PLAIN_VERTEX plainVerts[6] = {
		PLAIN_VERTEX{ XMFLOAT3(1, 1, 1), XMFLOAT2(1, 1), 0 },
		PLAIN_VERTEX{ XMFLOAT3(1, -1, 0), XMFLOAT2(1, 0), 0 },
		PLAIN_VERTEX{ XMFLOAT3(-1, 1, 0), XMFLOAT2(0, 1), 0 },
		PLAIN_VERTEX{ XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 0), 0 },
		PLAIN_VERTEX{ XMFLOAT3(-1, 1, 0), XMFLOAT2(0, 1), 0 },
		PLAIN_VERTEX{ XMFLOAT3(1, -1, 0), XMFLOAT2(1, 0), 0 }
	};

	struct INTERMEDIATE_VB
	{
		XMFLOAT4 position;
		UINT bitPoints;
	};

	// fences
	UINT frameIndex;
	UINT64 fenceVal[numFrames];

	// pipeline objects
	D3D12_VIEWPORT viewport, voxelViewport;
	ComPtr<ID3D12Device> device;
	D3D12_RECT scissorRect, voxelScissorRect;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12RootSignature> computeRootSignature;
	ComPtr<ID3D12PipelineState> densityPipelineState,
		occupiedPipelineState,
		genVertsPipelineState,
		renderPipelineState,
		vertexMeshPipelineState;
	ComPtr<ID3D12PipelineState> computeStatePR;
	ComPtr<ID3D12PipelineState> computeStateMC;
	ComPtr<ID3D12PipelineState> computeStateCS[CS_COUNT];
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<IDXGISwapChain3> swapChain;

	// per frame vars
	ComPtr<ID3D12Resource> renderTarget[numFrames], intermediateTarget[1];
	ComPtr<ID3D12CommandAllocator> commandAllocator[numFrames];

	// heaps
	ComPtr<ID3D12DescriptorHeap> rtvHeap;
	ComPtr<ID3D12DescriptorHeap> csuHeap;
	ComPtr<ID3D12DescriptorHeap> samplerHeap;
	UINT rtvDescriptorSize;
	UINT csuDescriptorSize;
	UINT samplerDescriptorSize;

	// synchronized objects
	ComPtr<ID3D12Fence> fence;
	HANDLE fenceEvent;
	HANDLE swapChainEvent;

	D3D12_RESOURCE_DESC voxelTextureDesc = {};
	D3D12_SAMPLER_DESC samplerDesc = {};

	struct Ray
	{
		XMFLOAT3 origin;
		XMFLOAT3 direction;

		XMFLOAT3 invDirection;
	};

	struct RAYPRESENT
	{
		FLOAT intensity;
		Ray ray;
		XMFLOAT4 color;
	};

	struct Box
	{
		XMFLOAT3 bbMin, bbMax;
	};

	struct NODE
	{
		int parent;
		int childL, childR;

		UINT code;

		// bounding box calc
		Box bbox;

		// index start value
		UINT index;
	};

	struct RESTART_BUFFER
	{
		UINT restart;
	};

	struct WORLD_POS
	{
		XMMATRIX worldViewProjection;
		XMMATRIX worldView;
	};

	struct VOXEL_POS
	{
		XMUINT3 voxelPos;
	};

	struct RAY_TRACE_BUFFER
	{
		UINT numGrps, numObjects;
		UINT screenWidth, screenHeight;
		XMFLOAT3 sceneBBMin;
		UINT numIndices;
		XMFLOAT3 sceneBBMax;
	};

	// constant buffers
	VOXEL_POS* voxelPosData;
	WORLD_POS* worldPosCB;
	RAY_TRACE_BUFFER* rayTraceCB;

	RESTART_BUFFER* bufferRestartData;

	// view params
	XMMATRIX world, view, projection, worldViewProjection;

	const XMVECTOR origEye{ 0.0f, 5.0f, -100.0f };
	XMVECTOR eye = origEye;
	XMVECTOR at{ 0.0f, 0.0f, 0.0f };
	XMVECTOR up{ 0.0f, 1.f, 0.0f };

	float yAngle = 0, xAngle = 0;

	// locations in the root parameter table
	enum RootParameters : UINT32
	{
		rpCB = 0,
		rpSRV,
		//rpUAV,
		rpSAMPLER,
		rpCount
	};
};

#endif