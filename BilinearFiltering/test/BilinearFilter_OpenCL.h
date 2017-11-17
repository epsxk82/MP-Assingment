#pragma once

#include "BilinearFilter.h"
#include "CL/cl.h"
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
	//OpenCL 장치
	struct Device
	{
	public:
		Device(cl_platform_id platform, cl_device_id device, cl_context context, cl_command_queue commandQueue);

		cl_platform_id _Platform;
		cl_device_id _Device;
		cl_command_queue _CommandQueue;
		cl_context _Context;

		cl_program _SWBilinearDownScalingProgram;
		cl_program _SWBilinearUpScalingProgram;

		cl_kernel _SWBilinearDownScalingKernel;
		cl_kernel _SWBilinearUpScalingKernel;
	};

private:
	void Initialize();
	bool InitializeDevice();
	bool BuildPrograms();
	cl_program BuildProgram(std::string const& programPath, cl_context context) const;

	void Release();

	void GetMaxSize(int width, int height, size_t totalImages, size_t *maxWidth, size_t *maxHeight);
	void GetPlatformInformation(cl_platform_id platform, std::string* platformInformation) const;
	void GetDeviceInformation(cl_device_id device, std::string* deviceInformation) const;

	bool CheckImageProperty(int width, int height, int pitch, int bytePerPixel, int unitWidth, int unitHeight) const;
	bool ReadyForRun() const;

	bool RunKernelWithBatch(cl_context context, cl_command_queue commandQueue, cl_kernel kernel, cl_mem sourceImage, cl_mem destinationImage,
		std::vector<std::vector<BYTE*>> const& colorBuffers, int width, int height, int unitWidth, int unitHeight, int pitch, int outputWidth, int outputHeight,
		std::vector<std::pair<size_t, size_t>> const& indiceOfOuputBitmapToRetreive, std::vector<FIBITMAP*>& outputBitmaps) const;
	
	void AddImageIntoAtlas(BYTE* source, unsigned int rowIndex, unsigned int columnIndex, int sourceWidth, int sourceHeight, int atlasWidth, int atlasHeight, BYTE* atlas) const;
	void GetImageFromAtlas(BYTE* atlas, unsigned int rowIndex, unsigned int columnIndex, int atlasWidth, int atlasHeight, int destinationWidth, int destinationHeight, BYTE* destination) const; 

	void OutputRunningError() const;
	void OutputInvalidImageProperty() const;

	bool _HardwareBilinear;
	bool _Initialized;
	//OpenCL 장치 리스트
	std::vector<Device> _Devices;
	//현재 선택된 OpenCL 장치
	Device* _RunningDevice;
};