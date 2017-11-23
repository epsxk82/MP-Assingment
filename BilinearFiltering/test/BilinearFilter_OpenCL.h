#pragma once

#include "BilinearFilter.h"
#include "OpenCLEnvironment.h"
#include <unordered_map>

//OpenCL 필터
class BilinearFilter_OpenCL : public BilinearFilter
{
public:
	BilinearFilter_OpenCL();
	~BilinearFilter_OpenCL();
	virtual void Downsample(std::vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<FIBITMAP*>* destinations);
	virtual void Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations);
	virtual std::string const& GetName() const;
	void GetAvailableDevices(std::vector<std::string> *availableDevices) const;
	void SetRunningDevice(unsigned int device);


private:
	void Initialize();
	void Release();

	void GetMaxSize(int width, int height, size_t totalImages, size_t *maxWidth, size_t *maxHeight);

	bool CheckImageProperty(int width, int height, int pitch, int bytePerPixel, int unitWidth, int unitHeight) const;
	bool ReadyForRun() const;

	bool RunKernelWithBatch(cl_context context, cl_command_queue commandQueue, cl_kernel kernel, cl_mem sourceImage, cl_mem destinationImage,
		std::vector<std::vector<BYTE*>> const& colorBuffers, int width, int height, int bytePerPixel, int unitWidth, int unitHeight, int pitch, int outputWidth, int outputHeight,
		std::vector<std::pair<size_t, size_t>> const& indiceOfOuputBitmapToRetreive, std::vector<FIBITMAP*>& outputBitmaps) const; 

	void OutputRunningError() const;
	void OutputInvalidImageProperty() const;

	bool _Initialized;
	//OpenCL 장치 리스트
	std::vector<OpenCLEnvironment::Device> _Devices;
	//현재 선택된 OpenCL 장치
	OpenCLEnvironment::Device *_RunningDevice;
};