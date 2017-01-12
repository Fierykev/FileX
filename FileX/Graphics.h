#ifndef GRPAHICS_H
#define GRAPHICS_H

#include <d3d12.h>
#include <dxgi1_4.h>
#include <DirectXMath.h>
#include <wrl.h>

#include "Manager.h"
#include "Image.h"
#include "Image2D.h"

#include "ProcGen.h"
#include "FindY.h"

#define NUM_RENDER_TYPES 4
#define NUM_DENSITY_TYPES 4

using namespace Microsoft::WRL;
using namespace DirectX;

constexpr float PERSON_HEIGHT = 0.f;

constexpr float FINDY_SIZE = 256.f;
constexpr float FINDY_SIZE_P1 = 257.f;
constexpr float SAMP_EXPANSION = .1f;// VOXEL_SIZE / CHUNK_SIZE;

#define SRVS_PER_FRAME 2

#define DSV_FORMAT DXGI_FORMAT_D32_FLOAT

#define CAM_DELTA .1f

static constexpr UINT COUNTER_SIZE = sizeof(UINT64);
static constexpr float speed = 1.f, angleSpeed = .3f;

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

	// static globals
	static const UINT numFrames = 2;

//private:
	// methods
	void loadPipeline();
	void loadAssets();
	void populateCommandList();
	void moveToNextFrame();
	static void waitForGpu();

	// added methods
	static void setupDescriptors();
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
		CB_GENERATION_CONSTANTS,
		CB_DENSITY_CONSTANTS,
		CBV_COUNT
	};

	enum BVSRV : UINT32
	{
		DENSITY_TEXTURE = 0,
		INDEX_TEXTURE,
		FINDY_TEXTURE,
		UPLOAD_TEX,
		NOISE_0,
		NOISE_1,
		NOISE_2,
		NOISEH_0,
		NOISEH_1,
		NOISEH_2,
		ALTITUDE,
		BUMPMAP,
		SRV_COUNT
	};

	enum BVUAV : UINT32
	{
		UAV_YPOS = 0,
		DEBUG_VAR,
		UAV_COUNT
	};

	enum BVDSV : UINT32
	{
		DSV_TEX = 0,
		DSV_COUNT
	};

	enum BVRTV : UINT32
	{
		RTV_FRAMES = numFrames - 1, // not to be used
		RTV_DENSITY_TEXTURE,
		RTV_INDEX_TEXTURE,
		RTV_FINDY_TEXTURE,
		RTV_UPLOAD_TEXTURE,
		RTV_COUNT
	};

	enum BVSAMPLER :UINT32
	{
		NEAREST_CLAMP_SAMPLER = 0,
		LINEAR_CLAMP_SAMPLER,
		LINEAR_REPEAT_SAMPLER,
		NEAREST_REPEAT_SAMPLER,
		SAMPLER_COUNT
	};

	// buffers
	static ComPtr<ID3D12Resource> bufferCB[CBV_COUNT],
		bufferSRV[SRV_COUNT],
		bufferUAV[UAV_COUNT],
		bufferDSV[DSV_COUNT],
		plainVCB;

	static ComPtr<ID3D12Resource> zeroBuffer;

	XMFLOAT3 startLoc;
	XMINT3 currentMid;

	static D3D12_VERTEX_BUFFER_VIEW plainVB;

	struct PLAIN_VERTEX
	{
		XMFLOAT3 position;
		XMFLOAT2 texcoord;
	};

	struct POLY_CONSTANTS
	{
		INT numberPolygons[256];
	};

	struct EDGE_CONSTANTS
	{
		XMINT4 edgeNumber[256][5];
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

	// fences
	static UINT frameIndex;
	static UINT64 fenceVal[numFrames];

	// pipeline objects
	static D3D12_VIEWPORT viewport, viewYViewport;
	static ComPtr<ID3D12Device> device;
	static D3D12_RECT scissorRect, viewYScissorRect;
	static ComPtr<ID3D12CommandQueue> commandQueue;
	static ComPtr<ID3D12RootSignature> rootSignature;
	static ComPtr<ID3D12RootSignature> computeRootSignature;
	static ComPtr<ID3D12PipelineState> renderPipelineSolidState,
		renderPipelineWireframeState;
	static ComPtr<ID3D12GraphicsCommandList> commandList;
	static ComPtr<IDXGISwapChain3> swapChain;

	// per frame vars
	static ComPtr<ID3D12Resource> renderTarget[numFrames], intermediateTarget[RTV_COUNT];
	static ComPtr<ID3D12CommandAllocator> commandAllocator[numFrames];

	// heaps
	static ComPtr<ID3D12DescriptorHeap> rtvHeap;
	static ComPtr<ID3D12DescriptorHeap> dsvHeap;
	static ComPtr<ID3D12DescriptorHeap> csuHeap;
	static 	ComPtr<ID3D12DescriptorHeap> samplerHeap;
	static UINT rtvDescriptorSize;
	static UINT dsvDescriptorSize;
	static UINT csuDescriptorSize;
	static UINT samplerDescriptorSize;

	// synchronized objects
	static ComPtr<ID3D12Fence> fence;
	static HANDLE fenceEvent;
	static HANDLE swapChainEvent;

	// Images
	Image noise0, noise1, noise2,
		noiseH0, noiseH1, noiseH2;
	Image2D altitude, bumpMap;

	// proc gen
	ProcGen procGen;

	// findY
	FindY findY;

#pragma pack(push, 1)
	struct WORLD_POS
	{
		XMMATRIX world;
		XMMATRIX worldView;
		XMMATRIX worldViewProjection;
	};

#pragma pack(pop)

	bool isSolid = false;

	// constant buffers
	WORLD_POS* worldPosCB;

	// view params
	XMMATRIX world, view, projection, worldViewProjection;

	XMFLOAT3 origDelta{ 0.0f, 10.0f, 20.0f };
	XMFLOAT3 eyeDelta = origDelta;
	XMVECTOR at{ eyeDelta.x, eyeDelta.y, eyeDelta.z };
	XMVECTOR eye = { 0, 0, 0 };
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