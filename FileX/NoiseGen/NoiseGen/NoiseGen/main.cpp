#include <stdio.h>
#include <time.h>
#include <iostream>
#include <string.h>
#include <Windows.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <errno.h>
#include <iostream>

#include <IL\il.h>

#include "open-simplex-noise.h"

#define WIDTH 16
#define HEIGHT WIDTH
#define DEPTH HEIGHT
//#define FEATURE_SIZE .1f

using namespace DirectX::PackedVector;
using namespace std;

void savePng(const wchar_t* filename, ILbyte* data, ILuint width, ILuint height, ILuint depth)
{
	ilEnable(IL_FILE_OVERWRITE);

	ILuint tex;
	ilGenImages(1, &tex);

	ilBindImage(tex);
	bool pass = ilTexImage(width, height, depth,
		4, IL_RGBA, IL_HALF, data);

	if (!pass)
		cout << "ERR COPY\n" << endl;

	pass = ilSave(IL_RAW, filename);

	if (!pass)
		cout << "ERR SAVE\n" << endl;

	ilDeleteImages(1, &tex);
}

struct float4
{
	float x = 0, y = 0, z = 0, w = 0;
	
	float& operator[] (unsigned int i)
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
	}
};

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
	}
};

int main()
{
	ilInit();

	int x, y, z, w;
	double value;
	half4 image4d[HEIGHT * WIDTH * DEPTH];
	struct osn_context *ctx;


	float sizes[] = { .1, .5, .9 };

	// low quality
	for (int i = 0; i < 3; i++)
	{
		open_simplex_noise(time(NULL), &ctx);

		float FEATURE_SIZE = sizes[i];

		for (z = 0; z < DEPTH; z++)
		{
			for (y = 0; y < HEIGHT; y++)
			{
				for (x = 0; x < WIDTH; x++)
				{
					for (w = 0; w < 4; w++)
					{
						value = open_simplex_noise4(ctx,
							(double)x / FEATURE_SIZE,
							(double)y / FEATURE_SIZE,
							(double)z / FEATURE_SIZE,
							(double)w / FEATURE_SIZE);

						image4d[x + y * WIDTH + z * WIDTH * HEIGHT][w] =
							XMConvertFloatToHalf(value);
					}
				}
			}
		}

		wstring s = L"../../../Noise/noise";
		s += '0' + i;
		s += L".raw";
		savePng(s.c_str(), (ILbyte*)image4d, WIDTH, HEIGHT, DEPTH);

		open_simplex_noise_free(ctx);
	}

	// high quality
	for (int i = 0; i < 3; i++)
	{
		open_simplex_noise(time(NULL), &ctx);

		float FEATURE_SIZE = sizes[i];

		// first loop for red value
		for (z = 0; z < DEPTH; z++)
		{
			for (y = 0; y < HEIGHT; y++)
			{
				for (x = 0; x < WIDTH; x++)
				{
					value = open_simplex_noise3(ctx,
						(double)x / FEATURE_SIZE,
						(double)y / FEATURE_SIZE,
						(double)z / FEATURE_SIZE);

					image4d[x + y * WIDTH + z * WIDTH * HEIGHT][0] =
						XMConvertFloatToHalf(value);
				}
			}
		}

		// second loop bga
		int delta[3][3] =
			{
				{0, 1, 0},
				{0, 0, 1},
				{0, 1, 1}
			};
		for (z = 0; z < DEPTH; z++)
		{
			for (y = 0; y < HEIGHT; y++)
			{
				for (x = 0; x < WIDTH; x++)
				{
					for (w = 1; w < 4; w++)
					{
						image4d[x + y * WIDTH + z * WIDTH * HEIGHT][w] =
							image4d[
								((x + delta[w - 1][0]) % WIDTH) +
								((y + delta[w - 1][1]) % HEIGHT) * WIDTH +
								((z + delta[w - 1][2]) % DEPTH) * WIDTH * HEIGHT
							][0];
					}
				}
			}
		}

		wstring s = L"../../../Noise/noiseH";
		s += '0' + i;
		s += L".raw";
		savePng(s.c_str(), (ILbyte*)image4d, WIDTH, HEIGHT, DEPTH);

		open_simplex_noise_free(ctx);
	}

	//system("PAUSE");

	return 0;
}
