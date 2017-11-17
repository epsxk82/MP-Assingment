#include "stdafx.h"
#include "BilinearFilter_Multithread.h"
#include "Util.h"

using namespace std;

//업 스케일링 수행
void BilinearFilter_Multithread::Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations)
{
	destinations->resize(64);
}

//다운 스케일링 수행
void BilinearFilter_Multithread::Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel,
	std::vector<FIBITMAP*>* destinations)
{
	
}

//필터이름 가져옴
string const& BilinearFilter_Multithread::GetName() const
{
	static string FilterName = "Multithread";

	return FilterName;
}