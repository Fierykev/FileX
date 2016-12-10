#ifndef PARSER_H
#define PARSER_H

#include <Windows.h>
#include <DirectXMath.h>
#include <string>

void parsePoly(std::string polyName, std::string edgeName,
	INT* ptData, DirectX::XMINT4 eData[256][5]);

#endif