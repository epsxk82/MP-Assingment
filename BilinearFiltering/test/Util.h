#pragma once

#include <string>
#include <vector>
#include "SubTileType.h"
#include <Windows.h>

//다른 클래스에서 사용하는 유틸모듬
class Util
{
public:
	static void Tokenize(std::string const& sourceString, std::string const& delimeterString, std::vector<std::string>* tokens);
	static void ReadFile(std::string const& fileName, std::string* content);
	static void ReadFileByLine(std::string const& fileName, std::vector<std::string>* lines);
	static void Merge(BYTE *dst, int dstWidth, int dstHeight, BYTE *src, int bytePerPixel, SubTileType subTyleType);
	static void Subdivide(BYTE *dst, BYTE* src, int srcWidth, int srcHeight, int bytePerPixel, SubTileType subTyleType);
};