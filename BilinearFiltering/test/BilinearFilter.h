#pragma once

#include <vector>
#include "SubTileType.h"
#include "FreeImage.h"

//4���� ��Ʈ�� ����
struct QuadBitMap
{
public:
	QuadBitMap();
	QuadBitMap(FIBITMAP* leftTop, FIBITMAP* rightTop, FIBITMAP* leftBottom, FIBITMAP* rightBottom);

	FIBITMAP* _LeftTop;
	FIBITMAP* _RightTop;
	FIBITMAP* _LeftBottom;
	FIBITMAP* _RightBottom;
};
//�����ϸ� ���� ����
class BilinearFilter
{
public:
	virtual void Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations) = 0;
	virtual void Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<FIBITMAP*>* destinations) = 0;
	virtual std::string const& GetName() const = 0;
};