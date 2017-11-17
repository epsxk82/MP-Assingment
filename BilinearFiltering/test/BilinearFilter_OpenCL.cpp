#include "stdafx.h"
#include "BilinearFilter_OpenCL.h"

#include <vector>
#include "Util.h"
#include <iostream>


#include "FreeImage.h"
using namespace std;

//Device 멤버 초기화 생성자
BilinearFilter_OpenCL::Device::Device(cl_platform_id platform, cl_device_id device, cl_context context, cl_command_queue commandQueue)
	: _Platform(platform), _Device(device), _Context(context), _CommandQueue(commandQueue)
{
}

//BilinearFilter_OpenCL 기본 생성자
BilinearFilter_OpenCL::BilinearFilter_OpenCL() : _Initialized(false), _HardwareBilinear(false), _RunningDevice(NULL)
{
	Initialize();
}

BilinearFilter_OpenCL::~BilinearFilter_OpenCL()
{
	Release();
}

//장치 초기화
bool BilinearFilter_OpenCL::InitializeDevice()
{
	cout << endl;
	cout << "OpenCL 장치를 초기화 중입니다." << endl;

	cl_int error;
	cl_platform_id platforms[10];
	cl_uint actualNumberOfPlatforms;
	error = clGetPlatformIDs(10, platforms, &actualNumberOfPlatforms);
	if(error)
	{
		cout << "OpenCL 에러 : OpenCL 플랫폼을 가져오는 중에 에러가 발생하였습니다." << endl;
		return false;
	}

	for(unsigned int platformIndex = 0; platformIndex < actualNumberOfPlatforms; ++platformIndex)
	{
		cl_platform_id platform = platforms[platformIndex];

		cl_device_id devices[10];
		cl_uint actualNumberOfDevices;
		clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 10, devices, & actualNumberOfDevices);
	
		cl_context_properties contextProperties[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };
		cl_context context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_ALL, NULL, NULL, &error);
		if(error)
		{
			cout << "OpenCL 에러 : OpenCL 컨텍스트를 생성하는 중에 에러가 발생하였습니다." << endl;
			string platformInformation;
			GetPlatformInformation(platform, &platformInformation);
			cout << "대상 플랫폼 : " << platformInformation << endl;
			
			continue;
		}

		for(unsigned int deviceIndex = 0; deviceIndex < actualNumberOfDevices; ++deviceIndex)
		{
			cl_device_id device = devices[deviceIndex];
			cl_command_queue commandQueue = clCreateCommandQueue(context, device, 0, &error);
			if(error)
			{
				cout << "OpenCL 에러 : OpenCL 장치의 커맨드큐를 생성하는 중에 에러가 발생하였습니다." << endl;
				string deviceInformation;
				GetDeviceInformation(device, &deviceInformation);
				cout << "대상 디바이스 : " << deviceInformation << endl;
			}
			else
			{
				_Devices.push_back(Device(platform, device, context, commandQueue));
			}
		}
	}

	if(_Devices.empty() == false)
	{
		return true;
	}
	else
	{
		return false;
	}
}

//현재 시스템의 OpenCL에서 사용가능한 모든 장치들을 얻어옴
void BilinearFilter_OpenCL::GetAvailableDevices(vector<string> *availableDevices) const
{
	typedef vector<Device>::size_type DeviceIndexType;
	
	DeviceIndexType deviceCount = _Devices.size();
	for(DeviceIndexType deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
	{
		string deviceInformation;
		GetDeviceInformation(_Devices[deviceIndex]._Device, &deviceInformation);
		availableDevices->push_back(deviceInformation);
	}
}

//OpenCL 프로그램을 빌드
bool BilinearFilter_OpenCL::BuildPrograms()
{
	typedef vector<Device>::size_type DeviceIndexType;

	struct ProgramInformation
	{
	public:
		ProgramInformation(string const& programPath, string const& kernelName) 
			: _ProgramPath(programPath), _KernelName(kernelName)
		{
		}

		string _ProgramPath;
		string _KernelName;
	};
	typedef vector<ProgramInformation>::size_type ProgramInformationIndexType;

	cout << "OpenCL 프로그램들을 빌드 중입니다." << endl;

	vector<ProgramInformation> programInformations;
	programInformations.push_back(ProgramInformation("SWBilinearDownScaling.cl", "SWBilinearDownScaling"));
	programInformations.push_back(ProgramInformation("SWBilinearUpScaling.cl", "SWBilinearUpScaling"));

	DeviceIndexType deviceCount = _Devices.size();
	for(DeviceIndexType deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
	{
		Device& device = _Devices[deviceIndex];
		ProgramInformationIndexType programInformationCount = programInformations.size();
		for(ProgramInformationIndexType programInformationIndex = 0; programInformationIndex < programInformationCount; ++programInformationIndex)
		{
			ProgramInformation const& programInformation = programInformations[programInformationIndex];
			string const& programPath = programInformation._ProgramPath;
			cl_program program = BuildProgram(programPath, device._Context);
			if(!program)
			{
				cout << "OpenCL 에러 : 프로그램을 생성하는 도중 에러가 발생하였습니다." << endl;
				cout << "대상 프로그램 : " << programPath << endl;

				return false;
			}
			else
			{
				cl_int error;
				cl_kernel kernel = clCreateKernel(program, programInformation._KernelName.c_str(), &error);
				if(error)
				{
					cout << "OpenCL 에러 : 커널을 생성하는 도중 에러가 발생하였습니다." << endl;
					cout << "대상 커널 : " << programInformation._KernelName << "(" << programPath << ")" << endl;

					return false;
				}
			
				if(programPath == "SWBilinearDownScaling.cl")
				{
					device._SWBilinearDownScalingProgram = program;
					device._SWBilinearDownScalingKernel = kernel;
				}
				else
					if(programPath == "SWBilinearUpScaling.cl")
					{
						device._SWBilinearUpScalingProgram = program;
						device._SWBilinearUpScalingKernel = kernel;
					}
			}
		}
	}

	return true;
}

//OpenCL 프로그램을 빌드
cl_program BilinearFilter_OpenCL::BuildProgram(string const& programPath, cl_context context) const
{
	cl_int error;

	string source;
	Util::ReadFile(programPath, &source);
	const char* sourceBuffer = source.c_str();

	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&sourceBuffer, NULL, &error);
	if(error)
	{
		cout << "OpenCL 에러 : 프로그램을 생성하는 중에 에러가 발생하였습니다.";
		cout << "프로그램 경로 : " << programPath << endl;

		return NULL;
	}
	
	error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if(error)
	{
		cout << "OpenCL 에러 : 프로그램을 빌드하는 중에 에러가 발생하였습니다.";
		cout << "프로그램 경로 : " << programPath << endl;

		return NULL;
	}

	return program;
}

//초기화
void BilinearFilter_OpenCL::Initialize()
{
	if(InitializeDevice() == true)
	{
		if(BuildPrograms() == true)
		{
			_Initialized = true;
		}
	}
}

void BilinearFilter_OpenCL::Release()
{
	typedef vector<Device>::size_type DeviceIndexType;

	if (_Initialized == true)
	{
		DeviceIndexType deviceIndexLimit = _Devices.size();
		for (DeviceIndexType deviceIndex = 0; deviceIndex < deviceIndexLimit; ++deviceIndex)
		{
			Device const& device = _Devices[deviceIndex];

			clReleaseKernel(device._SWBilinearUpScalingKernel);
			clReleaseKernel(device._SWBilinearDownScalingKernel);

			clReleaseProgram(device._SWBilinearUpScalingProgram);
			clReleaseProgram(device._SWBilinearDownScalingProgram);
			
			clReleaseCommandQueue(device._CommandQueue);
			clReleaseDevice(device._Device);
			clReleaseContext(device._Context);
		}
	}
}

//주어진 플랫폼 정보를 얻어옴
void BilinearFilter_OpenCL::GetPlatformInformation(cl_platform_id platform, std::string *platformInformation) const
{	
	size_t platformNameSize;
	clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, NULL, &platformNameSize);
	char* platformName = new char[platformNameSize];
	clGetPlatformInfo(platform, CL_PLATFORM_NAME, platformNameSize, platformName, NULL);

	size_t platformVendorSize;
	clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, 0, NULL, &platformVendorSize);
	char* platformVendor = new char[platformVendorSize];
	clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, platformVendorSize, platformVendor, NULL);

	size_t platformVersionSize;
	clGetPlatformInfo(platform, CL_PLATFORM_VERSION, 0, NULL, &platformVersionSize);
	char* platformVersion = new char[platformVersionSize];
	clGetPlatformInfo(platform, CL_PLATFORM_VERSION, platformVersionSize, platformVersion, NULL);

	size_t platformInformationBufferSize = 57 + platformNameSize + platformVendorSize + platformVersionSize;
	char* platformInformationBuffer = new char[platformInformationBufferSize];
	int writtenSize = sprintf_s(platformInformationBuffer, platformInformationBufferSize, "Platform Name : %s, Platform Vendor : %s, Platform Version : %s", 
		platformName, platformVendor, platformVersion);
	platformInformationBuffer[writtenSize] = '\0';

	*platformInformation = platformInformationBuffer;

	delete [] platformName;
	delete [] platformVendor;
	delete [] platformVersion;
	delete [] platformInformationBuffer;
}

//주어진 장치 정보를 얻어옴
void BilinearFilter_OpenCL::GetDeviceInformation(cl_device_id device, std::string* deviceInformation) const
{
	size_t deviceNameSize;
	clGetDeviceInfo(device, CL_DEVICE_NAME, 0, NULL, &deviceNameSize);
	char* deviceName = new char[deviceNameSize];
	clGetDeviceInfo(device, CL_DEVICE_NAME, deviceNameSize, deviceName, NULL);

	cl_device_type deviceType;
	clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type), &deviceType, NULL); 
	string deviceTypeName;
	switch(deviceType)
	{
	case CL_DEVICE_TYPE_DEFAULT :
		deviceTypeName = "Default";
		break;

	case CL_DEVICE_TYPE_CPU:
		deviceTypeName = "CPU";
		break;

	case CL_DEVICE_TYPE_GPU:
		deviceTypeName = "GPU";
		break;

	case CL_DEVICE_TYPE_ACCELERATOR:
		deviceTypeName = "Accelerator";
		break;

	case CL_DEVICE_TYPE_CUSTOM:
		deviceTypeName = "Custom";
		break;
	}

	size_t deviceVendorSize;
	clGetDeviceInfo(device, CL_DEVICE_VENDOR, 0, NULL, &deviceVendorSize);
	char* deviceVendor = new char[deviceVendorSize];
	clGetDeviceInfo(device, CL_DEVICE_VENDOR, deviceVendorSize, deviceVendor, NULL);

	size_t deviceVersionSize;
	clGetDeviceInfo(device, CL_DEVICE_VERSION, 0, NULL, &deviceVersionSize);
	char* deviceVersion = new char[deviceVersionSize];
	clGetDeviceInfo(device, CL_DEVICE_VERSION, deviceVersionSize, deviceVersion, NULL);

	cl_platform_id platform;
	clGetDeviceInfo(device, CL_DEVICE_PLATFORM, sizeof(cl_platform_id), &platform, NULL);
	size_t platformNameSize;
	clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, NULL, &platformNameSize);
	char* platformName = new char[platformNameSize];
	clGetPlatformInfo(platform, CL_PLATFORM_NAME, platformNameSize, platformName, NULL);

	size_t deviceInformationBufferSize = 80 + deviceNameSize + deviceTypeName.size() + platformNameSize + deviceVersionSize + deviceVendorSize;
	char* deviceInformationBuffer = new char[deviceInformationBufferSize];
	int writtenSize = sprintf_s(deviceInformationBuffer, deviceInformationBufferSize, "Device Name : %s, Device Type : %s, Device Version : %s, DeviceVendor : %s, Platform : %s",
		deviceName, deviceTypeName.c_str(), deviceVersion, deviceVendor, platformName);
	deviceInformationBuffer[writtenSize] = '\0';
	
	*deviceInformation = deviceInformationBuffer;

	delete[] deviceName;
	delete[] deviceVendor;
	delete[] deviceVersion;
	delete[] platformName;
	delete[] deviceInformationBuffer;
}

//필터가 실행시킬 준비가 되었는지 검사
bool BilinearFilter_OpenCL::ReadyForRun() const
{
	if((_Initialized == true) && _RunningDevice)
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

	if(!_RunningDevice)
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

//이미지 한 장을 이미지 아틀라스의 지정된 위치(행,열)에 저장
void BilinearFilter_OpenCL::AddImageIntoAtlas(BYTE* source, unsigned int rowIndex, unsigned int columnIndex, int sourceWidth, int sourceHeight, int atlasWidth, int atlasHeight, BYTE* atlas) const
{
	int row = sourceHeight * rowIndex;
	int column = sourceWidth * columnIndex;
	int atlasWidthInByte = atlasWidth * 4;
	int rowOffsetInByte = row * atlasWidthInByte;
	int columnOffsetInByte = column * 4; 
	int startOffsetInByte = rowOffsetInByte + columnOffsetInByte;
	int sourceWidthInByte = sourceWidth * 3;

	BYTE* destinationStart = atlas + startOffsetInByte;
	BYTE* sourceStart = source;
	if (source)
	{
		for (int heightIndex = 0; heightIndex < sourceHeight; ++heightIndex)
		{
			BYTE* destinationColor = destinationStart;
			for (int widthIndex = 0; widthIndex < sourceWidth; ++widthIndex)
			{
				memcpy(destinationColor, sourceStart, 3);
				destinationColor[3] = 255;
				sourceStart += 3;
				destinationColor += 4;
			}
			destinationStart += atlasWidthInByte;
		}
	}
	else
	{
		unsigned int numberToSetPerLine = sizeof(BYTE) * 4 * sourceWidth;
		for (int heightIndex = 0; heightIndex < sourceHeight; ++heightIndex)
		{
			memset(destinationStart, 0xff, numberToSetPerLine);
			destinationStart += atlasWidthInByte;
		}
	}
}

//이미지 아틀라스로부터 지정된 좌표(행,열)에 위치한 이미지 한장을 가져옴
void BilinearFilter_OpenCL::GetImageFromAtlas(BYTE* atlas, unsigned int rowIndex, unsigned int columnIndex, int atlasWidth, int atlasHeight, int destinationWidth, int destinationHeight, BYTE* destination) const
{
	int row = destinationHeight * rowIndex;
	int column = destinationWidth * columnIndex;
	int atlasWidthInByte = atlasWidth * 4;
	int rowOffsetInByte = row * atlasWidthInByte;
	int columnOffsetInByte = column * 4; 
	int startOffsetInByte = rowOffsetInByte + columnOffsetInByte;
	int destinationWidthInByte = destinationWidth * 3;

	BYTE* sourceStart = atlas + startOffsetInByte;
	BYTE* destinationStart = destination;
	for(int heightIndex = 0; heightIndex < destinationHeight; ++heightIndex)
	{
		BYTE* sourceColor = sourceStart;
		for(int widthIndex = 0; widthIndex < destinationWidth; ++widthIndex)
		{
			memcpy(destinationStart, sourceColor, 3);
			destinationStart += 3;
			sourceColor += 4;
		}
		sourceStart += atlasWidthInByte;
	}
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
				destinationImage, upsampledColorBuffers, inputAtlasWidth, inputAtlasHeight, width, height, pitch, outputAtlasWidth, outputAtlasHeight,
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
				destinationImage, downsampledColorBuffers, totalWidth, totalHeight, width, height, pitch, atlasWidth, atlasHeight, 
				indiceOfOutputBitmapToRetrieve, *destinations);
			
			segmentOffset += totalQuadBitmapPerSegment;

		}

		clReleaseMemObject(sourceImage);
		clReleaseMemObject(destinationImage);
	}
}

//이미지 아틀라스를 가지고 겹선형 필터링 커널을 수행
bool BilinearFilter_OpenCL::RunKernelWithBatch(cl_context context, cl_command_queue commandQueue, cl_kernel kernel, cl_mem sourceImage, cl_mem destinationImage, 
	vector<vector<BYTE*>> const& colorBuffers, int width, int height, int unitWidth, int unitHeight, int pitch, int outputWidth, int outputHeight, 
	vector<pair<size_t, size_t>> const& indiceOfOutputBitmapToRetreive, vector<FIBITMAP*>& outputBitmaps) const
{
	typedef vector<vector<BYTE*>>::size_type RowIndexType;
	typedef vector<BYTE*>::size_type ColumnIndexType;
	typedef vector<pair<size_t, size_t>>::size_type IndexPairIndexType;

	cl_int error;
	
	size_t origin[3] = {0, 0, 0};
	size_t region[3] = {width, height, 1};
	
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
			AddImageIntoAtlas(rowColorBuffers[column], row, column, unitWidth, unitHeight, width, height, input);
		}
	}
	
	clEnqueueUnmapMemObject(commandQueue, sourceImage, input, 0, NULL, NULL);
	
	clSetKernelArg(kernel, 0, sizeof(cl_mem), &sourceImage);
	clSetKernelArg(kernel, 1, sizeof(cl_mem), &destinationImage);
	
	const size_t globalWorkSize[2] = {outputWidth, outputHeight};
	error = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, NULL);
	clFinish(commandQueue);

	size_t regionForBilinear[3] = {outputWidth, outputHeight, 1};
	BYTE* output = (BYTE*)clEnqueueMapImage(commandQueue, destinationImage, CL_TRUE, CL_MAP_READ, origin, regionForBilinear, &rowPitch, NULL, 0, NULL, NULL, &error);
	if(error)
	{
		cout << "OpenCL 에러 : 이미지 바인딩 중 에러가 발생하였습니다.." << endl;
		return false;
	}

	if (colorBuffers.empty() == false)
	{
		BYTE* destination = (BYTE*)_aligned_malloc(unitWidth * unitHeight * 3, 16);
		IndexPairIndexType totalIndexPairs = indiceOfOutputBitmapToRetreive.size();
		for (IndexPairIndexType indexPairIndex = 0; indexPairIndex < totalIndexPairs; ++indexPairIndex)
		{
			pair<size_t, size_t> const& indexPair = indiceOfOutputBitmapToRetreive[indexPairIndex];
			GetImageFromAtlas(output, indexPair.first, indexPair.second, outputWidth, outputHeight, unitWidth, unitHeight, destination);
			outputBitmaps.push_back(FreeImage_ConvertFromRawBits(destination, unitWidth, unitHeight, pitch, 24, 0xffffffff, 0xffffffff, 0xffffffff));
		}
	
		_aligned_free(destination);
	}

	clEnqueueUnmapMemObject(commandQueue, destinationImage, output, 0, NULL, NULL);

	return true;
}