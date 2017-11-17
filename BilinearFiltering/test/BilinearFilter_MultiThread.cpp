#include "stdafx.h"
#include "BilinearFilter_Multithread.h"
#include "Util.h"

using namespace std;

//�� �����ϸ� ����
void BilinearFilter_Multithread::Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations)
{
	destinations->resize(64);
}

//�ٿ� �����ϸ� ����
void BilinearFilter_Multithread::Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel,
	std::vector<FIBITMAP*>* destinations)
{
	
}

//�����̸� ������
string const& BilinearFilter_Multithread::GetName() const
{
	static string FilterName = "Multithread";

	return FilterName;
}