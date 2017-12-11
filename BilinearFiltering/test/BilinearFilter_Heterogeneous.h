#include <thread>
#include "BilinearFilter_Multithread.h"
#include "BilinearFilter_OpenCL.h"

class BilinearFilter_Heterogeneous : public BilinearFilter
{
private:
	BilinearFilter_Multithread *cpuWorker;
	BilinearFilter_OpenCL *gpuWorker;
	int cpuWeight, gpuWeight;
public:
	BilinearFilter_Heterogeneous(BilinearFilter_Multithread *_cpuWorker, BilinearFilter_OpenCL *_gpuWorker, size_t _cpuWeight, size_t _gpuWeight);
	virtual void Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations);
	virtual void Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<FIBITMAP*>* destinations);
	virtual std::string const& GetName() const;
};