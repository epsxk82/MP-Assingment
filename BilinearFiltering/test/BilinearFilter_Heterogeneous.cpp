#include "stdafx.h"
#include "BilinearFilter_Heterogeneous.h"

// ������ ��Ŀ �� �����ϸ� �Լ�
void threadWorkerUpsample(BilinearFilter *bilinearFilter, std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations)
{
	bilinearFilter->Upsample(sources, width, height, pitch, bytePerPixel, destinations);
}

// ������ ��Ŀ �ٿ� �����ϸ� �Լ�
void threadWorkerDownsample(BilinearFilter *bilinearFilter, std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<FIBITMAP*>* destinations)
{
	bilinearFilter->Downsample(sources, width, height, pitch, bytePerPixel, destinations);
}

BilinearFilter_Heterogeneous::BilinearFilter_Heterogeneous(BilinearFilter_Multithread *_cpuWorker, BilinearFilter_OpenCL *_gpuWorker, size_t _cpuWeight, size_t _gpuWeight)
	:cpuWorker(_cpuWorker), gpuWorker(_gpuWorker), cpuWeight(_cpuWeight), gpuWeight(_gpuWeight)
{}

//�� �����ϸ� ����
void BilinearFilter_Heterogeneous::Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations)
{
	size_t sourceCount = sources.size();
	size_t cpuSourceCount = sourceCount * cpuWeight / (cpuWeight + gpuWeight);
	size_t gpuSourceCount = sourceCount - cpuSourceCount;

	std::vector<FIBITMAP*> cpuSources(sources.begin(), sources.begin() + cpuSourceCount);
	std::vector<FIBITMAP*> gpuSources(sources.begin() + cpuSourceCount, sources.end());
	std::vector<QuadBitMap> cpuDestinations;
	std::vector<QuadBitMap> gpuDestinations;

	std::thread *cpuThread = new std::thread(threadWorkerUpsample, cpuWorker, cpuSources, width, height, pitch, bytePerPixel, &cpuDestinations);
	std::thread *gpuThread = new std::thread(threadWorkerUpsample, gpuWorker, cpuSources, width, height, pitch, bytePerPixel, &gpuDestinations);

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

//�ٿ� �����ϸ� ����
void BilinearFilter_Heterogeneous::Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<FIBITMAP*>* destinations)
{
	size_t sourceCount = sources.size();
	size_t cpuSourceCount = sourceCount * cpuWeight / (cpuWeight + gpuWeight);
	size_t gpuSourceCount = sourceCount - cpuSourceCount;

	std::vector<QuadBitMap> cpuSources(sources.begin(), sources.begin() + cpuSourceCount);
	std::vector<QuadBitMap> gpuSources(sources.begin() + cpuSourceCount, sources.end());
	std::vector<FIBITMAP*> cpuDestinations(cpuSourceCount);
	std::vector<FIBITMAP*> gpuDestinations(gpuSourceCount);

	std::thread *cpuThread = new std::thread(threadWorkerDownsample, cpuWorker, cpuSources, width, height, pitch, bytePerPixel, &cpuDestinations);
	std::thread *gpuThread = new std::thread(threadWorkerDownsample, gpuWorker, cpuSources, width, height, pitch, bytePerPixel, &gpuDestinations);

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

//�����̸� ������
std::string const& BilinearFilter_Heterogeneous::GetName() const
{
	static std::string FilterName = "Heterogeneous";

	return FilterName;
}