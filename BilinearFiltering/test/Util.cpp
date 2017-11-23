#include "stdafx.h"
#include "Util.h"
#include <fstream>
#include <sstream>
//�׽�Ʈ �ڵ�, �ݵ�� ���� ��
#include <iostream>
//

using namespace std;

void Util::Tokenize(string const& source, string const& delimiters, vector<string> *tokens)
{
	string::size_type lastPos = source.find_first_not_of(delimiters, 0);
	string::size_type pos = source.find_first_of(delimiters, lastPos);

	while (string::npos != pos || string::npos != lastPos)
	{
		string tok = source.substr(lastPos, pos - lastPos);
		tok.shrink_to_fit();
		(*tokens).push_back(tok);

		lastPos = source.find_first_not_of(delimiters, pos);
		pos = source.find_first_of(delimiters, lastPos);
	}
}

void Util::ReadFile(string const& fileName, string* content)
{
	std::ifstream input(fileName);

	//R-Value Reference�� Move Semantics�� �����ϴٰ� ����(�� ���α׷��� �ۼ��� Visual Stutio 2015������ ����)
	*content = string((std::istreambuf_iterator<char>(input)), (std::istreambuf_iterator<char>()));
}

//������ ������ �ٺ��� ���
void Util::ReadFileByLine(string const& fileName, vector<string>* const lines)
{
	string fileBuffer;
	ReadFile(fileName, &fileBuffer);
	Tokenize(fileBuffer, "\n", lines);
}

//���� �̹����� ū �̹����� ��Ʈ�� ����
//SubTileType ���� �̹����� ��/�ϴ�, ������/���� ��Ʈ���� ����
void Util::Merge(BYTE *dst, int dstWidth, int dstHeight, BYTE *src, int bytePerPixel, SubTileType subTyleType)
{
	int srcWidth = dstWidth / 2;
	int srcHeight = dstHeight / 2;
	size_t srcWidthinByte = srcWidth * bytePerPixel;
	size_t dstWidthinByte = dstWidth * bytePerPixel;

	BYTE* srcCurrentRowStartingPosition = src;
	BYTE* dstCurrentRowStartingPosition;
	switch(subTyleType)
	{
	case LeftTop:
		dstCurrentRowStartingPosition = dst + (dstWidthinByte * srcHeight);
		break;

	case RightTop:
		dstCurrentRowStartingPosition = dst + (dstWidthinByte * srcHeight) + srcWidthinByte;
		break;

	case LeftBottom:
		dstCurrentRowStartingPosition = dst;
		break;

	case RightBottom:
		dstCurrentRowStartingPosition = dst + srcWidthinByte;
		break;
	}
	
	for(int srcHeightIndex = 0; srcHeightIndex < srcHeight; ++srcHeightIndex)
	{
		if(src)
		{
			memcpy(dstCurrentRowStartingPosition, srcCurrentRowStartingPosition, srcWidthinByte);
		}
		else
		{
			memset(dstCurrentRowStartingPosition, 0xffffffff, srcWidthinByte);
		}
		srcCurrentRowStartingPosition += srcWidthinByte;
		dstCurrentRowStartingPosition += dstWidthinByte;
	}
}

//ū�̹����� ���� �̹����� ����
//SubTileType ���� �̹����� ��/�ϴ�, ������/���� ��Ʈ���� ����
 void Util::Subdivide(BYTE *dst, BYTE* src, int srcWidth, int srcHeight, int bytePerPixel, SubTileType subTyleType)
 {
	 int dstWidth = srcWidth / 2;
	 int dstHeight = srcHeight / 2;

	 int srcWidthinByte = srcWidth * bytePerPixel;
	 int dstWidthinByte = dstWidth * bytePerPixel;

	 BYTE* srcCurrentRowStartingPosition;
	 switch(subTyleType)
	{
	case LeftTop:
		srcCurrentRowStartingPosition = src + (srcWidthinByte * dstHeight);
		break;

	case RightTop:
		srcCurrentRowStartingPosition = src + (srcWidthinByte * dstHeight) + dstWidthinByte;
		break;

	case LeftBottom:
		srcCurrentRowStartingPosition = src;
		break;

	case RightBottom:
		srcCurrentRowStartingPosition = src + dstWidthinByte;
		break;
	}

	BYTE* dstCurrentRowStartingPosition = dst;
	for(int heightIndex = 0; heightIndex < dstHeight; ++heightIndex)
	{
		memcpy(dstCurrentRowStartingPosition, srcCurrentRowStartingPosition, dstWidthinByte);
		dstCurrentRowStartingPosition += dstWidthinByte;
		srcCurrentRowStartingPosition += srcWidthinByte;
	}
 }

 //�̹��� �� ���� �̹��� ��Ʋ���� ������ ��ġ(��,��)�� ����
 void Util::AddImageIntoAtlas(BYTE* source, unsigned int rowIndex, unsigned int columnIndex, int sourceWidth, int sourceHeight, int atlasWidth, int atlasHeight, BYTE* atlas)
 {
	 int row = sourceHeight * rowIndex;
	 int column = sourceWidth * columnIndex;
	 int atlasWidthInByte = atlasWidth * 4;
	 int rowOffsetInByte = row * atlasWidthInByte;
	 int columnOffsetInByte = column * 4;
	 int startOffsetInByte = rowOffsetInByte + columnOffsetInByte;
	 int sourceWidthInByte = sourceWidth * 4;

	 BYTE* destinationStart = atlas + startOffsetInByte;
	 BYTE* sourceStart = source;
	 if (source)
	 {
		 for (int heightIndex = 0; heightIndex < sourceHeight; ++heightIndex)
		 {
			 memcpy(destinationStart, sourceStart, sourceWidthInByte);
			 sourceStart += sourceWidthInByte;
			 destinationStart += atlasWidthInByte;
		 }
	 }
	 else
	 {
		 unsigned int numberToSetPerLine = sizeof(BYTE) * 4 * sourceWidth;
		 for (int heightIndex = 0; heightIndex < sourceHeight; ++heightIndex)
		 {
			 memset(destinationStart, 0xff, numberToSetPerLine);
			 destinationStart += atlasWidthInByte;
		 }
	 }
 }

 //�̹��� ��Ʋ�󽺷κ��� ������ ��ǥ(��,��)�� ��ġ�� �̹��� ������ ������
 void Util::GetImageFromAtlas(BYTE* atlas, unsigned int rowIndex, unsigned int columnIndex, int atlasWidth, int atlasHeight, int destinationWidth, int destinationHeight, BYTE* destination)
 {
	 int row = destinationHeight * rowIndex;
	 int column = destinationWidth * columnIndex;
	 int atlasWidthInByte = atlasWidth * 4;
	 int rowOffsetInByte = row * atlasWidthInByte;
	 int columnOffsetInByte = column * 4;
	 int startOffsetInByte = rowOffsetInByte + columnOffsetInByte;
	 int destinationWidthInByte = destinationWidth * 4;

	 BYTE* sourceStart = atlas + startOffsetInByte;
	 BYTE* destinationStart = destination;
	 for (int heightIndex = 0; heightIndex < destinationHeight; ++heightIndex)
	 {
		 memcpy(destinationStart, sourceStart, destinationWidthInByte);
		 destinationStart += destinationWidthInByte;
		 sourceStart += atlasWidthInByte;
	 }
 }
