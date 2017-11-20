#pragma once

#include <thread>
#include "BilinearFilter.h"
#include "BilinearFilter_FreeImage.h"

//FreeImage « ≈Õ
class BilinearFilter_Multithread : public BilinearFilter
{
private:
	int numThreads;
public:
	virtual void Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations);
	virtual void Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<FIBITMAP*>* destinations);
	virtual std::string const& GetName() const;
	void SetNumThreads(int numThreads);
};
