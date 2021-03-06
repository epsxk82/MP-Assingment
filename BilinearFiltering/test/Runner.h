#pragma once

#include <string>
#include "ImageScaler.h"
#include "BilinearFilter_FreeImage.h"
#include "BilinearFilter_OpenCL.h"
#include "BilinearFilter_Multithread.h"
#include "BilinearFilter_Heterogeneous.h"

//응용프로그램 수행자
class Runner
{
public:
	Runner(std::string const& inputFilePath = "input.txt");

private:
	void Initialize(std::string const& filePath, BatchInput* batchInput, std::string *immediatePath, std::string* outputPath) const;
	void DoRun(BatchInput& batchInput, std::string const& immediatePath, std::string const& outputPath) const;
	int RunFilterSelection() const;
	int RunOpenCLDeviceSelection(BilinearFilter_OpenCL& bilinearFiler) const;
	int RunMultithreadSelection() const;
	int RunHeterogeneousCpuSelection() const;
	int RunHeterogeneousGpuSelection() const;

	void Run(std::string const& filePath) const;
};