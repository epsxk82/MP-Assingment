#include "stdafx.h"
#include "BilinearFilter_Multithread.h"
#include "Util.h"

using namespace std;

// ������ �� �����ϸ� �Լ�
void threadUpsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, QuadBitMap* destinations, int start) {

	BilinearFilter_FreeImage worker;
	auto resultSize = sources.size();
	vector<QuadBitMap> workerDestinations;

	worker.Upsample(sources, width, height, pitch, bytePerPixel, &workerDestinations);

	for (size_t i = 0; i < resultSize; i++) {
		destinations[i + start] = workerDestinations[i];
	}
}

// ������ �ٿ� �����ϸ� �Լ�
void threadDownsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, FIBITMAP** destinations, int start)
{
	BilinearFilter_FreeImage worker;
	auto resultSize = sources.size();
	vector<FIBITMAP*> workerDestinations;

	worker.Downsample(sources, width, height, pitch, bytePerPixel, &workerDestinations);

	for (size_t i = 0; i < resultSize; i++) {
		destinations[i + start] = workerDestinations[i];
	}
}

//�� �����ϸ� ����
void BilinearFilter_Multithread::Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations)
{
	auto sourceCount = sources.size();
	auto workSize = sourceCount / numThreads;

	vector<thread*> workerThreads;

	destinations->resize(sourceCount);

	for (int i = 0; i < numThreads; i++) {
		auto start = i * workSize;
		auto startIter = sources.cbegin() + start;

		auto end = start + workSize;
		if (i == numThreads - 1) {
			end = sourceCount;
		}
		auto endIter = sources.cbegin() + end;

		vector<FIBITMAP*> workerSources(startIter, endIter);
		workerThreads.push_back(new thread(threadUpsample, workerSources, width, height, pitch, bytePerPixel, destinations->data(), start));
	}

	for (int i = 0; i < numThreads; i++) {
		workerThreads[i]->join();
		delete workerThreads[i];
	}

}

//�ٿ� �����ϸ� ����
void BilinearFilter_Multithread::Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<FIBITMAP*>* destinations)
{
	auto sourceCount = sources.size();
	auto workSize = sourceCount / numThreads;

	vector<thread*> workerThreads;

	destinations->resize(sourceCount);

	for (int i = 0; i < numThreads; i++) {
		auto start = i * workSize;
		auto startIter = sources.cbegin() + start;

		auto end = start + workSize;
		if (i == numThreads - 1) {
			end = sourceCount;
		}
		auto endIter = sources.cbegin() + end;

		vector<QuadBitMap> workerSources(startIter, endIter);
		workerThreads.push_back(new thread(threadDownsample, workerSources, width, height, pitch, bytePerPixel, destinations->data(), start));
	}

	for (int i = 0; i < numThreads; i++) {
		workerThreads[i]->join();
		delete workerThreads[i];
	}
}

//�����̸� ������
string const& BilinearFilter_Multithread::GetName() const
{
	static string FilterName = "Multithread";

	return FilterName;
}

//������ ���� ����
void BilinearFilter_Multithread::SetNumThreads(int numThreads) {
	this->numThreads = numThreads;
}
