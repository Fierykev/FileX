#include "Parser.h"
#include <fstream>

using namespace DirectX;

using namespace std;

void parsePoly(string polyName, string edgeName, INT* ptData, XMINT4 eData[256][5])
{
	ifstream in(polyName.c_str());
	string line;
	INT index = 0;

	while (getline(in, line))
	{
		ptData[index] = atoi(line.c_str());
		index++;
	}

	in.close();

	in.open(edgeName.c_str());

	index = 0;

	while (getline(in, line))
	{
		INT t[4];
		sscanf_s(line.c_str(), "%i %i %i %i",
			&t[0], &t[1], &t[2], &t[3]);

		eData[index / 5][index % 5].x = t[0];
		eData[index / 5][index % 5].y = t[1];
		eData[index / 5][index % 5].z = t[2];
		eData[index / 5][index % 5].w = t[3];

		index ++;
	}
}