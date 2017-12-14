#include "stdafx.h"
#include "BilinearFilter_Heterogeneous.h"

// 스레드 워커 업 스케일링 함수
void threadWorkerUpsample(BilinearFilter *bilinearFilter, std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations)
{
	bilinearFilter->Upsample(sources, width, height, pitch, bytePerPixel, destinations);
}

// 스레드 워커 다운 스케일링 함수
void threadWorkerDownsample(BilinearFilter *bilinearFilter, std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<FIBITMAP*>* destinations)
{
	bilinearFilter->Downsample(sources, width, height, pitch, bytePerPixel, destinations);
}

BilinearFilter_Heterogeneous::BilinearFilter_Heterogeneous(BilinearFilter_Multithread *_cpuWorker, BilinearFilter_OpenCL *_gpuWorker, size_t _cpuWeight, size_t _gpuWeight)
	:cpuWorker(_cpuWorker), gpuWorker(_gpuWorker), cpuWeight(_cpuWeight), gpuWeight(_gpuWeight)
{}

//업 스케일링 수행
void BilinearFilter_Heterogeneous::Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations)
{
	auto sourceCount = sources.size();
	auto cpuSourceCount = sourceCount * cpuWeight / (cpuWeight + gpuWeight);
	auto gpuSourceCount = sourceCount - cpuSourceCount;

	auto cpuStartIter = sources.begin();
	auto cpuEndIter = sources.begin() + cpuSourceCount;
	auto gpuStartIter = sources.begin() + cpuSourceCount;
	auto gpuEndIter = sources.end();
	std::vector<FIBITMAP*> cpuSources(cpuStartIter, cpuEndIter);
	std::vector<FIBITMAP*> gpuSources(gpuStartIter, gpuEndIter);
	std::vector<QuadBitMap> cpuDestinations;
	std::vector<QuadBitMap> gpuDestinations;

	std::thread *cpuThread = new std::thread(threadWorkerUpsample, cpuWorker, cpuSources, width, height, pitch, bytePerPixel, &cpuDestinations);
	std::thread *gpuThread = new std::thread(threadWorkerUpsample, gpuWorker, gpuSources, width, height, pitch, bytePerPixel, &gpuDestinations);

	destinations->resize(sourceCount);
	QuadBitMap *destinationsData = destinations->data();

	cpuThread->join();
	gpuThread->join();

	for (size_t i = 0; i < cpuSourceCount; i++) {
		destinationsData[i] = cpuDestinations[i];
	}
	for (size_t i = 0; i < gpuSourceCount; i++) {
		destinationsData[i + cpuSourceCount] = gpuDestinations[i];
	}
}

//다운 스케일링 수행
void BilinearFilter_Heterogeneous::Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<FIBITMAP*>* destinations)
{
	size_t sourceCount = sources.size();
	size_t cpuSourceCount = sourceCount * cpuWeight / (cpuWeight + gpuWeight);
	size_t gpuSourceCount = sourceCount - cpuSourceCount;
	
	auto cpuStartIter = sources.begin();
	auto cpuEndIter = sources.begin() + cpuSourceCount;
	auto gpuStartIter = sources.begin() + cpuSourceCount;
	auto gpuEndIter = sources.end();
	std::vector<QuadBitMap> cpuSources(cpuStartIter, cpuEndIter);
	std::vector<QuadBitMap> gpuSources(gpuStartIter, gpuEndIter);
	std::vector<FIBITMAP*> cpuDestinations;
	std::vector<FIBITMAP*> gpuDestinations;

	std::thread *cpuThread = new std::thread(threadWorkerDownsample, cpuWorker, cpuSources, width, height, pitch, bytePerPixel, &cpuDestinations);
	std::thread *gpuThread = new std::thread(threadWorkerDownsample, gpuWorker, gpuSources, width, height, pitch, bytePerPixel, &gpuDestinations);

	destinations->resize(sourceCount);
	FIBITMAP* *destinationsData = destinations->data();

	cpuThread->join();
	gpuThread->join();

	for (size_t i = 0; i < cpuSourceCount; i++) {
		destinationsData[i] = cpuDestinations[i];
	}
	for (size_t i = 0; i < gpuSourceCount; i++) {
		destinationsData[i + cpuSourceCount] = gpuDestinations[i];
	}
}

//필터이름 가져옴
std::string const& BilinearFilter_Heterogeneous::GetName() const
{
	static std::string FilterName = "Heterogeneous";

	return FilterName;
}
