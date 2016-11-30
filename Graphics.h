#ifndef GRPAHICS_H
#define GRAPHICS_H

#define NUM_VOXELS_X 1
#define NUM_VOXELS_Y 1
#define NUM_VOXELS_Z 5
#define NUM_VOXELS (NUM_VOXELS_X * NUM_VOXELS_Y * NUM_VOXELS_Z)

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <wrl.h>
#include "Manager.h"

using namespace Microsoft::WRL;
using namespace DirectX;

#define PERSON_HEIGHT 5.f
#define CHUNK_SIZE 25.f

#define EXTRA 4.f
#define VOXEL_SIZE_M2 31.f
#define VOXEL_SIZE_M1 32.f
#define VOXEL_SIZE 33.f
#define VOXEL_SIZE_P1 34.f
#define OCC_SIZE 41.f
#define OCC_SIZE_P1 42.f
#define OCC_SIZE_M1 40.f

/*
#define EXTRA 4.f
#define VOXEL_SIZE_M2 63.f
#define VOXEL_SIZE_M1 64.f
#define VOXEL_SIZE 65.f
#define VOXEL_SIZE_P1 66.f
#define OCC_SIZE 73.f
#define OCC_SIZE_P1 74.f
#define OCC_SIZE_M1 72.f*/

#define FINDY_SIZE 256.f
#define FINDY_SIZE_P1 257.f
#define SAMP_EXPANSION 5

#define SRVS_PER_FRAME 2

#define DENSITY_FORMAT DXGI_FORMAT_R32_FLOAT
#define INDEX_FORMAT DXGI_FORMAT_R32_UINT

#define CAM_DELTA .1f

constexpr UINT COUNTER_SIZE = sizeof(UINT64);

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

	const UINT NUM_POINTS = VOXEL_SIZE * VOXEL_SIZE;

	const UINT BYTES_POINTS = sizeof(OCCUPIED_POINT) * sizeof(OCCUPIED_POINT) *	NUM_POINTS;

	// TODO: check if need to throw more memory at gpu
	const UINT MAX_BUFFER_SIZE =
		(((2 * (UINT)VOXEL_SIZE_P1 * (UINT)VOXEL_SIZE_P1 * (UINT)VOXEL_SIZE_P1) >> 2) << 2) * sizeof(VERT_OUT);

	const UINT MAX_INDEX_SIZE =
		(((2 * (UINT)VOXEL_SIZE_P1 * (UINT)VOXEL_SIZE_P1 * (UINT)VOXEL_SIZE_P1) >> 2) << 2) * sizeof(UINT);

private:
	// methods
	void loadPipeline();
	void loadAssets();
	void populateCommandList();
	void moveToNextFrame();
	void waitForGpu();

	// added methods
	void setupProceduralDescriptors();
	void renderDensity(UINT index);
	void renderOccupied(UINT index);
	bool renderGenVerts(UINT index);
	void renderVertexMesh(UINT index);
	void renderClearTex(UINT index);
	void renderVertSplat(UINT index);
	void renderGenIndices(UINT index);
	void getVertIndexData(UINT index);
	float findY();
	void findYRender();
	void searchTerrain();
	void sampleDensity();
	void phase1(UINT index);
	bool phase2(UINT index);
	void phase3(UINT index);
	void phase4(UINT index);
	void drawPhase();

	enum ComputeShader : UINT32
	{
		CS_SEARCH_TERRAIN = 0,
		CS_COUNT
	};

	// setup the srv and uav enums
	enum BVCB : UINT32
	{
		CB_WORLD_POS = 0,
		CB_VOXEL_POS,
		CBV_COUNT
	};

	enum BVSRV : UINT32
	{
		DENSITY_TEXTURE = 0,
		INDEX_TEXTURE,
		FINDY_TEXTURE,
		SRV_COUNT
	};

	enum BVUAV : UINT32
	{
		UAV_YPOS = 0,
		DEBUG_VAR,
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
		bufferUAV[UAV_COUNT],
		zeroBuffer, plainVCB, pointVCB,
		vertexBuffer[NUM_VOXELS], vertexBackBuffer,
		indexBuffer[NUM_VOXELS],
		vertexCount,
		indexCount, yposMap;

	D3D12_VERTEX_BUFFER_VIEW plainVB, pointVB;

	struct PLAIN_VERTEX
	{
		XMFLOAT3 position;
		XMFLOAT2 texcoord;
	};

	struct OCCUPIED_POINT
	{
		XMFLOAT2 position;
		XMFLOAT2 uv;
	};

	struct BITPOS
	{
		UINT bitpos;
	};

	struct POLY_CONSTANTS
	{
		INT numberPolygons[256];
	};

	struct EDGE_CONSTANTS
	{
		XMINT4 edgeNumber[256][5];
	};

	struct VERT_OUT
	{
		XMFLOAT4 position;
		XMFLOAT3 normal;
	};

	PLAIN_VERTEX plainVerts[6] = {
		PLAIN_VERTEX{ XMFLOAT3(1, 1, 0), XMFLOAT2(1, 1) },
		PLAIN_VERTEX{ XMFLOAT3(1, -1, 0), XMFLOAT2(1, 0) },
		PLAIN_VERTEX{ XMFLOAT3(-1, 1, 0), XMFLOAT2(0, 1) },
		PLAIN_VERTEX{ XMFLOAT3(-1, -1, 0), XMFLOAT2(0, 0) },
		PLAIN_VERTEX{ XMFLOAT3(-1, 1, 0), XMFLOAT2(0, 1) },
		PLAIN_VERTEX{ XMFLOAT3(1, -1, 0), XMFLOAT2(1, 0) }
	};

	struct INTERMEDIATE_VB
	{
		XMFLOAT4 position;
		UINT bitPoints;
	};

	// verts
	UINT vertCount[NUM_VOXELS],
		indCount[NUM_VOXELS];

	// fences
	UINT frameIndex;
	UINT64 fenceVal[numFrames];

	// pipeline objects
	D3D12_VIEWPORT viewport, voxelViewport, vertSplatViewport, viewYViewport;
	ComPtr<ID3D12Device> device;
	D3D12_RECT scissorRect, voxelScissorRect, vertSplatScissorRect, viewYScissorRect;
	ComPtr<ID3D12CommandQueue> commandQueue;
	ComPtr<ID3D12RootSignature> rootSignature;
	ComPtr<ID3D12RootSignature> computeRootSignature;
	ComPtr<ID3D12PipelineState> densityPipelineState,
		occupiedPipelineState,
		genVertsPipelineState,
		renderPipelineSolidState,
		renderPipelineWireframeState,
		vertexMeshPipelineState,
		dataVertSplatPipelineState,
		dataGenIndicesPipelineState,
		dataClearTexPipelineState,
		sampleDensityPipelineState;
	ComPtr<ID3D12PipelineState> computeStatePR;
	ComPtr<ID3D12PipelineState> computeStateMC;
	ComPtr<ID3D12PipelineState> computeStateCS[CS_COUNT];
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ComPtr<IDXGISwapChain3> swapChain;

	// per frame vars
	ComPtr<ID3D12Resource> renderTarget[numFrames], intermediateTarget[SRV_COUNT];
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

	D3D12_SAMPLER_DESC samplerDesc = {};

	struct WORLD_POS
	{
		XMMATRIX world;
		XMMATRIX worldView;
		XMMATRIX worldViewProjection;
	};

	struct VOXEL_POS
	{
		XMUINT4 voxelPos;
		XMFLOAT4 voxelPosF;
	};

	bool isSolid = false;

	// constant buffers
	VOXEL_POS* voxelPosData;
	WORLD_POS* worldPosCB;

	// view params
	XMMATRIX world, view, projection, worldViewProjection;

	const XMVECTOR origEye{ 0.0f, 0.0f, 0.0f };
	XMVECTOR eye = origEye;
	XMVECTOR at{ 0.0f, 0.0f, 20.0f };
	XMVECTOR up{ 0.0f, 1.f, 0.0f };

	float yAngle = 0, xAngle = 0;

	// locations in the root parameter table
	enum RootParameters : UINT32
	{
		rpCB = 0,
		rpSRV,
		rpUAV,
		rpSAMPLER,
		rpCount
	};
};

#endif