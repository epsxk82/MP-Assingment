#pragma once

#include <vector>
#include <string>
#include "CL/cl.h"

class OpenCLEnvironment
{
public:
	struct Device
	{
	public:
		Device(cl_platform_id platform, cl_device_id device, cl_context context, cl_command_queue commandQueue);

	public:
		cl_platform_id _Platform;
		cl_device_id _Device;
		cl_command_queue _CommandQueue;
		cl_context _Context;

		cl_program _SWBilinearDownScalingProgram;
		cl_program _SWBilinearUpScalingProgram;

		cl_kernel _SWBilinearDownScalingKernel;
		cl_kernel _SWBilinearUpScalingKernel;
	};

public:
	static bool Initialize();
	static std::vector<Device> GetAvailableDevices();
	static void Release();
	static void GetDeviceInformation(cl_device_id device, std::string* deviceInformation);

private:
	static bool InitializeDevice();
	static bool CreateProgram(Device* device);
	static cl_program BuildProgram(std::string const& programPath, cl_context context);

	static void GetPlatformInformation(cl_platform_id platform, std::string* platformInformation);

private:
	static bool _Initialized;
	static std::vector<Device> _Devices;
};