#include "stdafx.h"
#include "BilinearFilter.h"

//QuadBitMap �⺻ ������
QuadBitMap::QuadBitMap()
{
}

//QuadBitMap ����ʱ�ȭ ������
QuadBitMap::QuadBitMap(FIBITMAP* leftTop, FIBITMAP* rightTop, FIBITMAP* leftBottom, FIBITMAP* rightBottom) 
	: _LeftTop(leftTop), _RightTop(rightTop), _LeftBottom(leftBottom), _RightBottom(rightBottom)
{
}