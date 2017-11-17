#include "stdafx.h"
#include "BilinearFilter_FreeImage.h"
#include "Util.h"

using namespace std;

//업 스케일링 수행
void BilinearFilter_FreeImage::Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations)
{
	typedef vector<FIBITMAP*>::size_type BitMapIndexType;

	BitMapIndexType sourceCount = sources.size();
	int halfWidth = width / 2;
	int halfHeight = height / 2;
	for(BitMapIndexType bitMapIndex = 0; bitMapIndex < sourceCount; ++bitMapIndex)
	{
		FIBITMAP* bitMap = sources[bitMapIndex];

		FIBITMAP* leftTopBitMap = FreeImage_RescaleRect(bitMap, width, height, 0, 0, halfWidth, halfHeight, FILTER_BILINEAR);
		FIBITMAP* rightTopBitMap = FreeImage_RescaleRect(bitMap, width, height, halfWidth, 0, width, halfHeight, FILTER_BILINEAR);
		FIBITMAP* leftBottomBitMap = FreeImage_RescaleRect(bitMap, width, height, 0, halfHeight, halfWidth, height, FILTER_BILINEAR);
		FIBITMAP* rightBottomBitMap = FreeImage_RescaleRect(bitMap, width, height, halfWidth, halfHeight, width, height, FILTER_BILINEAR);

		destinations->push_back(QuadBitMap(leftTopBitMap, rightTopBitMap, leftBottomBitMap, rightBottomBitMap));
	}
}

//다운 스케일링 수행
void BilinearFilter_FreeImage::Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, 
	std::vector<FIBITMAP*>* destinations)
{
	typedef vector<QuadBitMap*>::size_type QuadBitMapIndexType;

	QuadBitMapIndexType sourceCount = sources.size();
	int halfWidth = width / 2;
	int halfHeight = height / 2;
	BYTE* mergedBits = (BYTE*)_aligned_malloc(width * height * bytePerPixel, 16);
	for(QuadBitMapIndexType quadBitMapIndex = 0; quadBitMapIndex < sourceCount; ++quadBitMapIndex)
	{
		QuadBitMap const& quadBitMap = sources[quadBitMapIndex];

		if(quadBitMap._LeftTop)
		{
			FIBITMAP* leftTopHalf = FreeImage_Rescale(quadBitMap._LeftTop, halfWidth, halfHeight, FILTER_BILINEAR);
			Util::Merge(mergedBits, width, height, FreeImage_GetBits(leftTopHalf), bytePerPixel, LeftTop);
		}
		else
		{
			Util::Merge(mergedBits, width, height, NULL, bytePerPixel, LeftTop);
		}

		if(quadBitMap._RightTop)
		{
			FIBITMAP* rightTopHalf = FreeImage_Rescale(quadBitMap._RightTop, halfWidth, halfHeight, FILTER_BILINEAR);
			Util::Merge(mergedBits, width, height, FreeImage_GetBits(rightTopHalf), bytePerPixel, RightTop);
		}
		else
		{
			Util::Merge(mergedBits, width, height, NULL, bytePerPixel, RightTop);
		}

		if(quadBitMap._LeftBottom)
		{
			FIBITMAP* leftBottomHalf = FreeImage_Rescale(quadBitMap._LeftBottom, halfWidth, halfHeight, FILTER_BILINEAR);
			Util::Merge(mergedBits, width, height, FreeImage_GetBits(leftBottomHalf), bytePerPixel, LeftBottom);
		}
		else
		{
			Util::Merge(mergedBits, width, height, NULL, bytePerPixel, LeftBottom);
		}

		if(quadBitMap._RightBottom)
		{
			FIBITMAP* rightBottomHalf = FreeImage_Rescale(quadBitMap._RightBottom, halfWidth, halfHeight, FILTER_BILINEAR);
			Util::Merge(mergedBits, width, height, FreeImage_GetBits(rightBottomHalf), bytePerPixel, RightBottom);
		}
		else
		{
			Util::Merge(mergedBits, width, height, FreeImage_GetBits(NULL), bytePerPixel, RightBottom);
		}

		FIBITMAP* mergedBitMap = FreeImage_ConvertFromRawBits(mergedBits, width, height, pitch, bytePerPixel * 8, 0xffffffff, 0xffffffff, 0xffffffff);
		destinations->push_back(mergedBitMap);
	}
	_aligned_free(mergedBits);
}

//필터이름 가져옴
string const& BilinearFilter_FreeImage::GetName() const
{
	static string FilterName = "FreeImage";

	return FilterName;
}