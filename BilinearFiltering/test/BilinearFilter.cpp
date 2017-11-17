#include "stdafx.h"
#include "BilinearFilter.h"

//QuadBitMap 기본 생성자
QuadBitMap::QuadBitMap()
{
}

//QuadBitMap 멤버초기화 생성자
QuadBitMap::QuadBitMap(FIBITMAP* leftTop, FIBITMAP* rightTop, FIBITMAP* leftBottom, FIBITMAP* rightBottom) 
	: _LeftTop(leftTop), _RightTop(rightTop), _LeftBottom(leftBottom), _RightBottom(rightBottom)
{
}