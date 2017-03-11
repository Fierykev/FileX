#ifndef _PATHING_H_
#define _PATHING_H_

#include <DirectXMath.h>
#include <wrl.h>
#include <Windows.h>
#include <vector>

#include "d3dx12.h"

using namespace Microsoft::WRL;
using namespace DirectX;
using namespace std;

class Pathing
{
public:
	Pathing();

	void createPathing(XMFLOAT3 startPos, XMFLOAT3 endPos);
#pragma pack(push, 1)
	struct PATH
	{
		XMFLOAT2 point;
	};
#pragma pack(pop)
#

private:
	const float STRAY = 2.f;
	const float MAX_DEGREE_DELTA = 4.f;

	vector<PATH> path;

};

#endif