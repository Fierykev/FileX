#include <stdio.h>
#include <time.h>
#include <iostream>
#include <string.h>
#include <Windows.h>
#include <errno.h>

#include <IL\il.h>

#include "open-simplex-noise.h"

#define WIDTH 64
#define HEIGHT WIDTH
#define DEPTH 1//HEIGHT
#define FEATURE_SIZE 24

void savePng(LPWSTR filename, ILbyte* data, ILuint width, ILuint height, ILuint depth)
{
	ilEnable(IL_FILE_OVERWRITE);

	ILuint tex;
	ilGenImages(1, &tex);

	ilBindImage(tex);
	ilTexImage(width, height, depth,
		4, IL_RGBA, IL_FLOAT, data);

	ilSave(IL_BMP, filename);

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

int main()
{
	ilInit();

	int x, y, z, w;
	double value;
	float4 image4d[HEIGHT * WIDTH * DEPTH];
	struct osn_context *ctx;

	open_simplex_noise(time(NULL), &ctx);

	for (z = 0; z < DEPTH; z++)
	{
		for (y = 0; y < HEIGHT; y++)
		{
			for (x = 0; x < WIDTH; x++)
			{
				for (w = 0; w < 4; w++)
				{
					image4d[x + y * WIDTH + z * WIDTH * HEIGHT][w] =
						(open_simplex_noise3(ctx,
						(double)x / FEATURE_SIZE,
							(double)y / FEATURE_SIZE,
							(double)z / FEATURE_SIZE));
						//0));//(double)w / FEATURE_SIZE
				}
			}
		}
	}

	savePng(L"Noise/tmp.bmp", (ILbyte*)image4d, WIDTH, HEIGHT, DEPTH);

	open_simplex_noise_free(ctx);
	return 0;
}
