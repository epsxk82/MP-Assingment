#include "stdafx.h"
#include "BilinearFilter_OpenCL.h"

#include <vector>
#include "Util.h"
#include <iostream>

#include "FreeImage.h"
using namespace std;

//BilinearFilter_OpenCL 기본 생성자
BilinearFilter_OpenCL::BilinearFilter_OpenCL() : _Initialized(false), _RunningDevice(NULL)
{
	Initialize();
}

BilinearFilter_OpenCL::~BilinearFilter_OpenCL()
{
	Release();
}

//현재 시스템의 OpenCL에서 사용가능한 모든 장치들을 얻어옴
void BilinearFilter_OpenCL::GetAvailableDevices(vector<string> *availableDevices) const
{
	typedef vector<OpenCLEnvironment::Device>::size_type DeviceIndexType;
	
	DeviceIndexType deviceCount = _Devices.size();
	for(DeviceIndexType deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
	{
		OpenCLEnvironment::Device const& device = _Devices[deviceIndex];
	
		string deviceInformation;
		OpenCLEnvironment::GetDeviceInformation(device._Device, &deviceInformation);
		availableDevices->push_back(deviceInformation);
	}
}

//초기화
void BilinearFilter_OpenCL::Initialize()
{
	_Initialized = OpenCLEnvironment::Initialize();
	if (_Initialized == true)
	{
		_Devices = OpenCLEnvironment::GetAvailableDevices();
	}
}

void BilinearFilter_OpenCL::Release()
{
	OpenCLEnvironment::Release();
}

//필터가 실행시킬 준비가 되었는지 검사
bool BilinearFilter_OpenCL::ReadyForRun() const
{
	if((_Initialized == true) && (_RunningDevice != NULL))
	{
		return true;
	}
	else
	{
		OutputRunningError();

		return false;
	}
}

//실행 에러 출력
void BilinearFilter_OpenCL::OutputRunningError() const
{
	cout << endl;
	cout << "OpenCL 필터를 실행할 수 없습니다." << endl;
	if(_Initialized == false)
	{
		cout << "필터가 초기화 되지 않았습니다." << endl;
		return;
	}

	if(_RunningDevice == NULL)
	{
		cout << "실행 장치가 선택되지 않았습니다." << endl;
		return;
	}
}

//이미지 속성값 에러 출력
void BilinearFilter_OpenCL::OutputInvalidImageProperty() const
{
	cout << "유효하지 않은 이미지 입니다" << endl;
}
//필터 이름 가져옴
string const& BilinearFilter_OpenCL::GetName() const
{
	static string FilterName = "OpenCL";

	return FilterName;
}

//실행 장치 선택
void BilinearFilter_OpenCL::SetRunningDevice(unsigned int device)
{
	if(device < _Devices.size())
	{
		_RunningDevice = &(_Devices[device]);
	}
}

//이미지 속성의 유효성을 검사
bool BilinearFilter_OpenCL::CheckImageProperty(int width, int height, int pitch, int bytePerPixel, int unitWidth, int unitHeight) const
{
	if ((width <= 0) || (height <= 0) || (pitch <= 0) || (bytePerPixel <= 0) || (CL_DEVICE_IMAGE2D_MAX_WIDTH < unitWidth)
		|| (CL_DEVICE_IMAGE2D_MAX_HEIGHT < unitHeight))
	{
		OutputInvalidImageProperty();

		return false;
	}
	else
	{
		return true;
	}
}

void BilinearFilter_OpenCL::GetMaxSize(int width, int height, size_t totalImages, size_t *maxWidth, size_t *maxHeight)
{
	static const size_t optimalMaxWidthHeuristic = 4096;
	static const size_t optimalMaxHeightHeuristic = 4096;

	size_t totalWidth = 2 * width * totalImages;
	if (totalWidth <= optimalMaxWidthHeuristic)
	{
		*maxWidth = totalWidth;
		*maxHeight = 2 * height;
	}
	else
	{
		*maxWidth = optimalMaxWidthHeuristic;
		*maxHeight = (2 * height) * (size_t)ceil(((double)totalImages) / ((double)(optimalMaxWidthHeuristic / (2 * width))));
		*maxHeight = __min(*maxHeight, optimalMaxHeightHeuristic);
	}
}

//업스케일링 수행, 필터가 초기화가 올바르게 안됐거나 현재 선택된 장치가 없으면 실행  안됨(현재 하드웨어 겹선형 필터링은 지원이 안됨)
//더 빠른 처리를 위해, OpenCL 이미지 객체에 최대한 많은 이미지를 배치해서 일괄적으로 필터링이 수행되도록 이미지 아틀라스를 만들어 스케일링 커널을 수행 함
void BilinearFilter_OpenCL::Upsample(std::vector<FIBITMAP*> const& sources, int width, int height, int pitch, int bytePerPixel, std::vector<QuadBitMap>* destinations)
{
	typedef vector<vector<BYTE*>>::size_type RowIndexType;
	typedef vector<BYTE*>::size_type ColumnIndexType;
	typedef vector<FIBITMAP*>::size_type BitMapIndexType;
	typedef vector<pair<size_t, size_t>>::size_type IndexPairIndexType;

	if ((ReadyForRun() == true) && (CheckImageProperty(width, height, pitch, bytePerPixel, width * 2, height * 2) == true))
	{
		size_t currentMaxImageWidth;
		size_t currentMaxImageHeight;
		GetMaxSize(width, height, sources.size(), &currentMaxImageWidth, &currentMaxImageHeight);

		const size_t upSampledWidth = width * 2;
		const size_t upSampledHeight = height * 2;
		const size_t outputAtlasWidth = (currentMaxImageWidth / upSampledWidth) * upSampledWidth;
		const size_t outputAtlasHeight = (currentMaxImageHeight / upSampledHeight) * upSampledHeight;
		const size_t inputAtlasWidth = outputAtlasWidth / 2;
		const size_t inputAtlasHeight = outputAtlasHeight / 2;

		const unsigned int rowBitmapCount = inputAtlasHeight / height;
		const unsigned int columnBitmapCount = inputAtlasWidth / width;

		vector<vector<BYTE*>> upsampledColorBuffers(rowBitmapCount);
		for (RowIndexType row = 0; row < rowBitmapCount; ++row)
		{
			upsampledColorBuffers[row].resize(columnBitmapCount);
		}

		int error;
		cl_image_format imageFormat;
		imageFormat.image_channel_data_type = CL_UNORM_INT8;
		imageFormat.image_channel_order = CL_RGBA;
		cl_context context = _RunningDevice->_Context;
		cl_mem sourceImage = clCreateImage2D(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, &imageFormat, inputAtlasWidth, inputAtlasHeight, 0, NULL, &error);
		if (error)
		{
			cout << "OpenCL 에러 : 이미지 객체 생성 중 에러가 발생하였습니다.." << endl;
			return;
		}
		cl_mem destinationImage = clCreateImage2D(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, &imageFormat, outputAtlasWidth, outputAtlasHeight, 0, NULL, &error);
		if (error)
		{
			cout << "OpenCL 에러 : 이미지 객체 생성 중 에러가 발생하였습니다.." << endl;
			return;
		}

		const unsigned int totalQuadBitmapPerSegment = columnBitmapCount * rowBitmapCount;
		const BitMapIndexType quadBitmapCount = sources.size();
		const unsigned int segmentCount = (unsigned int)ceilf((float)quadBitmapCount / (float)totalQuadBitmapPerSegment);
		
		BitMapIndexType segmentOffset = 0;
		for (unsigned int segment = 0; segment < segmentCount; ++segment)
		{
			BitMapIndexType quadBitmapLimit = segmentOffset + totalQuadBitmapPerSegment;
			if (quadBitmapCount < quadBitmapLimit)
			{
				quadBitmapLimit = quadBitmapCount;
			}

			BitMapIndexType totalQuadUpsampledBitmaps = (quadBitmapLimit - segmentOffset) * 4;

			vector<pair<size_t, size_t>> indiceOfOutputBitmapToRetreive(totalQuadUpsampledBitmaps);
			IndexPairIndexType indexPairIndex = 0;
			for (BitMapIndexType quadBitmapIndex = segmentOffset; quadBitmapIndex < quadBitmapLimit; ++quadBitmapIndex)
			{
				BitMapIndexType segmentBitmapIndex = quadBitmapIndex - segmentOffset;
				BitMapIndexType segmentBitmapRow = segmentBitmapIndex / columnBitmapCount;
				BitMapIndexType segmentBitmapColumn = segmentBitmapIndex % columnBitmapCount;
				
				upsampledColorBuffers[segmentBitmapRow][segmentBitmapColumn] = FreeImage_GetBits(sources[quadBitmapIndex]);
				indiceOfOutputBitmapToRetreive[indexPairIndex++] = pair<size_t, size_t>(segmentBitmapRow * 2 + 1, segmentBitmapColumn * 2);
				indiceOfOutputBitmapToRetreive[indexPairIndex++] = pair<size_t, size_t>(segmentBitmapRow * 2 + 1, segmentBitmapColumn * 2 + 1);
				indiceOfOutputBitmapToRetreive[indexPairIndex++] = pair<size_t, size_t>(segmentBitmapRow * 2, segmentBitmapColumn * 2);
				indiceOfOutputBitmapToRetreive[indexPairIndex++] = pair<size_t, size_t>(segmentBitmapRow * 2, segmentBitmapColumn * 2 + 1);
			}

			vector<FIBITMAP*> upsampledBitmaps;
			RunKernelWithBatch(_RunningDevice->_Context, _RunningDevice->_CommandQueue, _RunningDevice->_SWBilinearUpScalingKernel, sourceImage,
				destinationImage, upsampledColorBuffers, inputAtlasWidth, inputAtlasHeight, bytePerPixel, width, height, pitch, outputAtlasWidth, outputAtlasHeight,
				indiceOfOutputBitmapToRetreive, upsampledBitmaps);
			
			for (BitMapIndexType bitmapIndex = 0; bitmapIndex < totalQuadUpsampledBitmaps;)
			{
				destinations->push_back(QuadBitMap(upsampledBitmaps[bitmapIndex], upsampledBitmaps[bitmapIndex + 1], upsampledBitmaps[bitmapIndex + 2],
					upsampledBitmaps[bitmapIndex + 3]));
				bitmapIndex += 4;
			}

			segmentOffset += totalQuadBitmapPerSegment;

		}

		clReleaseMemObject(sourceImage);
		clReleaseMemObject(destinationImage);
	}
}

//다운 스케일링 수행, 필터가 초기화가 올바르게 안됐거나 현재 선택된 장치가 없으면 실행 안됨(현재 하드웨어 겹선형 필터링은 지원하지 않음)
//더 빠른 처리를 위해, OpenCL이미지 객체에 최대한 많은 이미지를 배치해서 일괄적으로 작업이 수행되도록 이미지 아틀라스를 만들어 스케일링 커널을 수행 함
void BilinearFilter_OpenCL::Downsample(vector<QuadBitMap> const& sources, int width, int height, int pitch, int bytePerPixel, vector<FIBITMAP*>* destinations)
{
	typedef vector<QuadBitMap>::size_type QuadBitMapIndexType;
	typedef vector<FIBITMAP*>::size_type BitMapIndexType;
	typedef vector<vector<BYTE*>>::size_type RowIndexType;
	typedef vector<BYTE*>::size_type ColumnIndexType;
	
	if((ReadyForRun() == true) && (CheckImageProperty(width, height, pitch, bytePerPixel, width * 2, height * 2) == true))
	{
		size_t currentMaxImageWidth;
		size_t currentMaxImageHeight;
		GetMaxSize(width, height, sources.size(), &currentMaxImageWidth, &currentMaxImageHeight);

		const int quadBitmapWidth = width * 2;
		const int quadBitmapHeight = height * 2;
		const int totalWidth = (currentMaxImageWidth / quadBitmapWidth) * quadBitmapWidth;
		const int totalHeight = (currentMaxImageHeight / quadBitmapHeight) * quadBitmapHeight;

		const unsigned int rowQuadBitmapCount = totalHeight / quadBitmapHeight;
		const unsigned int rowBitmapCount = rowQuadBitmapCount * 2;
		const unsigned int columnQuadBitmapCount = totalWidth / quadBitmapWidth;
		const unsigned int columnBitmapCount = columnQuadBitmapCount * 2;
		int atlasWidth = totalWidth / 2;
		int atlasHeight = totalHeight / 2;

		int error;
		cl_image_format imageFormat;
		imageFormat.image_channel_data_type = CL_UNORM_INT8;
		imageFormat.image_channel_order = CL_RGBA;
		cl_context context = _RunningDevice->_Context;
		cl_mem sourceImage = clCreateImage2D(context, CL_MEM_READ_ONLY | CL_MEM_ALLOC_HOST_PTR, &imageFormat, totalWidth, totalHeight, 0, NULL, &error);
		if (error)
		{
			cout << "OpenCL 에러 : 이미지 객체 생성 중 에러가 발생하였습니다.." << endl;
			return;
		}
		cl_mem destinationImage = clCreateImage2D(context, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, &imageFormat, atlasWidth, atlasHeight, 0, NULL, &error);
		if (error)
		{
			cout << "OpenCL 에러 : 이미지 객체 생성 중 에러가 발생하였습니다.." << endl;
			return;
		}
		
		vector<vector<BYTE*>> downsampledColorBuffers(rowBitmapCount);
		for (RowIndexType row = 0; row < rowBitmapCount; ++row)
		{
			downsampledColorBuffers[row].resize(columnBitmapCount);
		}
		
		const unsigned int totalQuadBitmapPerSegment = columnQuadBitmapCount * rowQuadBitmapCount;
		const QuadBitMapIndexType quadBitmapCount = sources.size();
		const unsigned int segmentCount = (unsigned int)ceilf((float)quadBitmapCount / (float)totalQuadBitmapPerSegment);
		QuadBitMapIndexType segmentOffset = 0;
		for (unsigned int segment = 0; segment < segmentCount; ++segment)
		{
			QuadBitMapIndexType quadBitmapLimit = segmentOffset + totalQuadBitmapPerSegment;
			if (quadBitmapCount < quadBitmapLimit)
			{
				quadBitmapLimit = quadBitmapCount;
			}
			
			vector<pair<size_t, size_t>> indiceOfOutputBitmapToRetrieve(quadBitmapLimit - segmentOffset);
			for (QuadBitMapIndexType quadBitmapIndex = segmentOffset; quadBitmapIndex < quadBitmapLimit; ++quadBitmapIndex)
			{
				QuadBitMap const& quadBitmap = sources[quadBitmapIndex];
				QuadBitMapIndexType segmentQuadBitmapIndex = quadBitmapIndex - segmentOffset;
				QuadBitMapIndexType segmentQuadBitmapRow = segmentQuadBitmapIndex / columnQuadBitmapCount;
				QuadBitMapIndexType segmentQuadBitmapColumn = segmentQuadBitmapIndex % columnQuadBitmapCount;

				QuadBitMapIndexType leftTopBitmapRow = segmentQuadBitmapRow * 2 + 1;
				QuadBitMapIndexType leftTopBitmapColumn = segmentQuadBitmapColumn * 2;
				downsampledColorBuffers[leftTopBitmapRow][leftTopBitmapColumn] = FreeImage_GetBits(quadBitmap._LeftTop);

				QuadBitMapIndexType rightTopBitmapRow = segmentQuadBitmapRow * 2 + 1;
				QuadBitMapIndexType rightTopBitmapColumn = segmentQuadBitmapColumn * 2 + 1;
				downsampledColorBuffers[rightTopBitmapRow][rightTopBitmapColumn] = FreeImage_GetBits(quadBitmap._RightTop);

				QuadBitMapIndexType leftBottomBitmapRow = segmentQuadBitmapRow * 2;
				QuadBitMapIndexType leftBottomBitmapColumn = segmentQuadBitmapColumn * 2;
				downsampledColorBuffers[leftBottomBitmapRow][leftBottomBitmapColumn] = FreeImage_GetBits(quadBitmap._LeftBottom);

				QuadBitMapIndexType rightBottomBitmapRow = segmentQuadBitmapRow * 2;
				QuadBitMapIndexType rightBottomBitmapColumn = segmentQuadBitmapColumn * 2 + 1;
				downsampledColorBuffers[rightBottomBitmapRow][rightBottomBitmapColumn] = FreeImage_GetBits(quadBitmap._RightBottom);

				indiceOfOutputBitmapToRetrieve[quadBitmapIndex] = make_pair(segmentQuadBitmapRow, segmentQuadBitmapColumn);
			}

			RunKernelWithBatch(_RunningDevice->_Context, _RunningDevice->_CommandQueue, _RunningDevice->_SWBilinearDownScalingKernel, sourceImage, 
				destinationImage, downsampledColorBuffers, totalWidth, totalHeight, bytePerPixel, width, height, pitch, atlasWidth, atlasHeight, 
				indiceOfOutputBitmapToRetrieve, *destinations);
			
			segmentOffset += totalQuadBitmapPerSegment;

		}

		clReleaseMemObject(sourceImage);
		clReleaseMemObject(destinationImage);
	}
}

//이미지 아틀라스를 가지고 겹선형 필터링 커널을 수행
bool BilinearFilter_OpenCL::RunKernelWithBatch(cl_context context, cl_command_queue commandQueue, cl_kernel kernel, cl_mem sourceImage, cl_mem destinationImage, 
	vector<vector<BYTE*>> const& colorBuffers, int width, int height, int bytePerPixel, int unitWidth, int unitHeight, int pitch, int outputWidth, int outputHeight,
	vector<pair<size_t, size_t>> const& indiceOfOutputBitmapToRetreive, vector<FIBITMAP*>& outputBitmaps) const
{
	typedef vector<vector<BYTE*>>::size_type RowIndexType;
	typedef vector<BYTE*>::size_type ColumnIndexType;
	typedef vector<pair<size_t, size_t>>::size_type IndexPairIndexType;

	cl_int error;
	
	size_t origin[3] = {0, 0, 0};
	size_t region[3] = {(size_t)width, (size_t)height, 1};
	
	size_t rowPitch;
	BYTE* input = (BYTE*)clEnqueueMapImage(commandQueue, sourceImage, CL_TRUE, CL_MAP_WRITE, origin, region, &rowPitch, NULL, 0, NULL, NULL, &error);
	if(error)
	{
		cout << "OpenCL 에러 : 이미지 바인딩 중 에러가 발생하였습니다.." << endl;
		return false;
	}

	RowIndexType rowCount = colorBuffers.size();
	for (RowIndexType row = 0; row < rowCount; ++row)
	{
		vector<BYTE*> const& rowColorBuffers = colorBuffers[row];
		ColumnIndexType columnCount = rowColorBuffers.size();
		for (ColumnIndexType column = 0; column < columnCount; ++column)
		{
			Util::AddImageIntoAtlas(rowColorBuffers[column], row, column, unitWidth, unitHeight, width, height, input);
		}
	}
	
	clEnqueueUnmapMemObject(commandQueue, sourceImage, input, 0, NULL, NULL);
	
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &sourceImage);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &destinationImage);
	
	const size_t globalWorkSize[2] = {(size_t)outputWidth, (size_t)outputHeight};
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
	clFinish(commandQueue);

	size_t regionForBilinear[3] = {(size_t)outputWidth, (size_t)outputHeight, 1};
	BYTE* output = (BYTE*)clEnqueueMapImage(commandQueue, destinationImage, CL_TRUE, CL_MAP_READ, origin, regionForBilinear, &rowPitch, NULL, 0, NULL, NULL, &error);
	if(error)
	{
		cout << "OpenCL 에러 : 이미지 바인딩 중 에러가 발생하였습니다.." << endl;
		return false;
	}

	if (colorBuffers.empty() == false)
	{
		BYTE* destination = (BYTE*)_aligned_malloc(unitWidth * unitHeight * 4, 16);
		IndexPairIndexType totalIndexPairs = indiceOfOutputBitmapToRetreive.size();
		for (IndexPairIndexType indexPairIndex = 0; indexPairIndex < totalIndexPairs; ++indexPairIndex)
		{
			pair<size_t, size_t> const& indexPair = indiceOfOutputBitmapToRetreive[indexPairIndex];
			Util::GetImageFromAtlas(output, indexPair.first, indexPair.second, outputWidth, outputHeight, unitWidth, unitHeight, destination);
			outputBitmaps.push_back(FreeImage_ConvertFromRawBits(destination, unitWidth, unitHeight, pitch, bytePerPixel * 8, 0xffffffff, 0xffffffff, 0xffffffff));
		}
	
		_aligned_free(destination);
	}

	clEnqueueUnmapMemObject(commandQueue, destinationImage, output, 0, NULL, NULL);

	return true;
}