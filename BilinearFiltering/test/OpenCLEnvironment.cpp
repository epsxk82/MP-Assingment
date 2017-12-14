#include "stdafx.h"
#include "OpenCLEnvironment.h"
#include "Util.h"
#include <iostream>

using namespace std;

//Device 멤버 초기화 생성자
OpenCLEnvironment::Device::Device(cl_platform_id platform, cl_device_id device, cl_context context, cl_command_queue commandQueue)
	: _Platform(platform), _Device(device), _Context(context), _CommandQueue(commandQueue)
{
}

vector<OpenCLEnvironment::Device> OpenCLEnvironment::_Devices;
bool OpenCLEnvironment::_Initialized;

bool OpenCLEnvironment::Initialize()
{
	if (InitializeDevice() == true)
	{
		_Initialized = true;

		return true;
	}
	else
	{
		_Initialized = false;

		return false;
	}
}

bool OpenCLEnvironment::InitializeDevice()
{
	cout << endl;
	cout << "Initializing OpenCL Devices." << endl;

	cl_int error;
	cl_platform_id platforms[10];
	cl_uint actualNumberOfPlatforms;
	error = clGetPlatformIDs(10, platforms, &actualNumberOfPlatforms);
	if (error)
	{
		cout << "OpenCL Error : Failed to retrieve OpenCL platforms." << endl;
		return false;
	}

	for (unsigned int platformIndex = 0; platformIndex < actualNumberOfPlatforms; ++platformIndex)
	{
		cl_platform_id platform = platforms[platformIndex];

		cl_device_id devices[10];
		cl_uint actualNumberOfDevices;
		clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 10, devices, &actualNumberOfDevices);

		cl_context_properties contextProperties[3] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platform, 0 };
		cl_context context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_ALL, NULL, NULL, &error);
		if (error)
		{
			cout << "OpenCL Error : Failed to create OpenCL context." << endl;
			string platformInformation;
			GetPlatformInformation(platform, &platformInformation);
			cout << "Target Platform : " << platformInformation << endl;

			continue;
		}

		for (unsigned int deviceIndex = 0; deviceIndex < actualNumberOfDevices; ++deviceIndex)
		{
			cl_device_id device = devices[deviceIndex];
			cl_command_queue commandQueue = clCreateCommandQueue(context, device, 0, &error);
			if (error)
			{
				cout << "OpenCL Error : Failed to create OpenCL device command queue." << endl;
				string deviceInformation;
				GetDeviceInformation(device, &deviceInformation);
				cout << "Target Device : " << deviceInformation << endl;
			}
			else
			{
				Device deviceObject(platform, device, context, commandQueue);
				if (CreateProgram(&deviceObject) == true)
				{
					_Devices.push_back(deviceObject);
				}
			}
		}
	}

	if (_Devices.empty() == false)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool OpenCLEnvironment::CreateProgram(Device* device)
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

	cout << "Building OpenCL program." << endl;

	vector<ProgramInformation> programInformations;
	programInformations.push_back(ProgramInformation("SWBilinearDownScaling.cl", "SWBilinearDownScaling"));
	programInformations.push_back(ProgramInformation("SWBilinearUpScaling.cl", "SWBilinearUpScaling"));

	ProgramInformationIndexType programInformationCount = programInformations.size();
	for (ProgramInformationIndexType programInformationIndex = 0; programInformationIndex < programInformationCount; ++programInformationIndex)
	{
		ProgramInformation const& programInformation = programInformations[programInformationIndex];
		string const& programPath = programInformation._ProgramPath;
		cl_program program = BuildProgram(programPath, device->_Context);
		if (!program)
		{
			cout << "OpenCL Error : Failed to build OpenCL program." << endl;
			cout << "Target Program : " << programPath << endl;

			string deviceInformation;
			GetDeviceInformation(device->_Device, &deviceInformation);
			cout << "OpenCL Device: " << deviceInformation << endl;

			return false;
		}
		else
		{
			cl_int error;
			cl_kernel kernel = clCreateKernel(program, programInformation._KernelName.c_str(), &error);
			if (error)
			{
				cout << "OpenCL Error : Failed to create OpenCL kernel." << endl;
				cout << "Target Kernel : " << programInformation._KernelName << "(" << programPath << ")" << endl;

				return false;
			}

			if (programPath == "SWBilinearDownScaling.cl")
			{
				device->_SWBilinearDownScalingProgram = program;
				device->_SWBilinearDownScalingKernel = kernel;
			}
			else
				if (programPath == "SWBilinearUpScaling.cl")
				{
					device->_SWBilinearUpScalingProgram = program;
					device->_SWBilinearUpScalingKernel = kernel;
				}
		}
	}

	return true;
}

//OpenCL 프로그램을 빌드
cl_program OpenCLEnvironment::BuildProgram(string const& programPath, cl_context context)
{
	cl_int error;

	string source;
	Util::ReadFile(programPath, &source);
	const char* sourceBuffer = source.c_str();

	cl_program program = clCreateProgramWithSource(context, 1, (const char**)&sourceBuffer, NULL, &error);
	if (error)
	{
		cout << "OpenCL Error : Failed to build OpenCL program.";
		cout << "Program Path : " << programPath << endl;

		return NULL;
	}

	error = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
	if (error)
	{
		cout << "OpenCL Error : Failed to build OpenCL program.";
		cout << "Program Path : " << programPath << endl;

		// Determine the size of the log
		size_t log_size;
		clGetProgramBuildInfo(program, _Devices.front()._Device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

		// Allocate memory for the log
		char *log = new char[1024];
		clGetProgramBuildInfo(program, _Devices.front()._Device, CL_PROGRAM_BUILD_LOG, 1024, log, NULL);

		cout << log << endl;
		delete log;

		return NULL;
	}

	return program;
}


void OpenCLEnvironment::Release()
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
void OpenCLEnvironment::GetPlatformInformation(cl_platform_id platform, std::string *platformInformation)
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

	delete[] platformName;
	delete[] platformVendor;
	delete[] platformVersion;
	delete[] platformInformationBuffer;
}

//주어진 장치 정보를 얻어옴
void OpenCLEnvironment::GetDeviceInformation(cl_device_id device, std::string* deviceInformation)
{
	size_t deviceNameSize;
	clGetDeviceInfo(device, CL_DEVICE_NAME, 0, NULL, &deviceNameSize);
	char* deviceName = new char[deviceNameSize];
	clGetDeviceInfo(device, CL_DEVICE_NAME, deviceNameSize, deviceName, NULL);

	cl_device_type deviceType;
	clGetDeviceInfo(device, CL_DEVICE_TYPE, sizeof(cl_device_type), &deviceType, NULL);
	string deviceTypeName;
	switch (deviceType)
	{
	case CL_DEVICE_TYPE_DEFAULT:
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

std::vector<OpenCLEnvironment::Device> OpenCLEnvironment::GetAvailableDevices()
{
	if (_Initialized == true)
	{
		return _Devices;
	}
	else
	{
		return vector<OpenCLEnvironment::Device>();
	}
}