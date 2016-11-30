#ifndef _RAYS_H_
#define _RAYS_H_

float3 ray32[32] =
{
	float3(0.286582, 0.257763, -0.922729),
	float3(-0.171812, -0.888079, 0.426375),
	float3(0.440764, -0.502089, -0.744066),
	float3(-0.841007, -0.428818, -0.329882),
	float3(-0.380213, -0.588038, -0.713898),
	float3(-0.055393, -0.207160, -0.976738),
	float3(-0.901510, -0.077811, 0.425706),
	float3(-0.974593, 0.123830, -0.186643),
	float3(0.208042, -0.524280, 0.825741),
	float3(0.258429, -0.898570, -0.354663),
	float3(-0.262118, 0.574475, -0.775418),
	float3(0.735212, 0.551820, 0.393646),
	float3(0.828700, -0.523923, -0.196877),
	float3(0.788742, 0.005727, -0.614698),
	float3(-0.696885, 0.649338, -0.304486),
	float3(-0.625313, 0.082413, -0.776010),
	float3(0.358696, 0.928723, 0.093864),
	float3(0.188264, 0.628978, 0.754283),
	float3(-0.495193, 0.294596, 0.817311),
	float3(0.818889, 0.508670, -0.265851),
	float3(0.027189, 0.057757, 0.997960),
	float3(-0.188421, 0.961802, -0.198582),
	float3(0.995439, 0.019982, 0.093282),
	float3(-0.315254, -0.925345, -0.210596),
	float3(0.411992, -0.877706, 0.244733),
	float3(0.625857, 0.080059, 0.775818),
	float3(-0.243839, 0.866185, 0.436194),
	float3(-0.725464, -0.643645, 0.243768),
	float3(0.766785, -0.430702, 0.475959),
	float3(-0.446376, -0.391664, 0.804580),
	float3(-0.761557, 0.562508, 0.321895),
	float3(0.344460, 0.753223, -0.560359)
};

float4 occWeigths[16] =
{
	float4(1, 1.000000000, 1.000000000, 1.000000000),
	float4(0.850997317, 0.980824675, 0.97451496, 0.968245837),
	float4(0.716176609, 0.960732353, 0.947988832, 0.935414347),
	float4(0.595056802, 0.93960866, 0.920299843, 0.901387819),
	float4(0.48713929, 0.917314755, 0.891301229, 0.866025404),
	float4(0.391905859, 0.893679531, 0.860813523, 0.829156198),
	float4(0.308816178, 0.868488366, 0.828613504, 0.790569415),
	float4(0.237304688, 0.841466359, 0.794417881, 0.75),
	float4(0.176776695, 0.812252396, 0.757858283, 0.707106781),
	float4(0.126603334, 0.780357156, 0.718441189, 0.661437828),
	float4(0.086114874, 0.745091108, 0.675480019, 0.612372436),
	float4(0.054591503, 0.705431757, 0.627971608, 0.559016994),
	float4(0.03125, 0.659753955, 0.574349177, 0.5),
	float4(0.015223103, 0.605202038, 0.511918128, 0.433012702),
	float4(0.005524272, 0.535886731, 0.435275282, 0.353553391),
	float4(0.000976563, 0.435275282, 0.329876978, 0.25)
};

#endif