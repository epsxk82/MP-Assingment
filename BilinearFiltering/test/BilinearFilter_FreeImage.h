#pragma once

#include "BilinearFilter.h"

//FreeImage « ≈Õ
class BilinearFilter_FreeImage : public BilinearFilter
{
public:
	virtual void Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations);
	virtual void Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<FIBITMAP*>* destinations);
	virtual std::string const& GetName() const;
};