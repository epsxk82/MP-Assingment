#include "stdafx.h"
#include <conio.h>
#include "Runner.h"
#include "Util.h"

using namespace std;

//Runner 생성자
//응응프로그램을 시작
Runner::Runner(string const& inputFilePath)
{
	Run(inputFilePath);
}

//입력파일로부터 입력정보를 읽어옴
void Runner::Initialize(std::string const& filePath, BatchInput* batchInput, std::string *immediatePath, std::string* outputPath) const
{
	cout << "Reading infromation from " << filePath << "..." << endl;

	vector<string> lines;
	Util::ReadFileByLine("input.txt", &lines);

	int level = stoi(lines[0]);
	vector<string> upperLeftTile;
	Util::Tokenize(lines[1], ",", &upperLeftTile);

	vector<string> belowRightTile;
	Util::Tokenize(lines[2], ",", &belowRightTile);

	int upperRow = stoi(upperLeftTile[0]);
	int upperColumn = stoi(upperLeftTile[1]);
	int belowRow = stoi(belowRightTile[0]);
	int belowColumn = stoi(belowRightTile[1]);

	batchInput->_Top = upperRow;
	batchInput->_Bottom = belowRow;
	batchInput->_Left = upperColumn;
	batchInput->_Right = belowColumn;
	batchInput->_Level = level;
	batchInput->_SuperLevelSpan = stoi(lines[4]);
	batchInput->_SubLevelSpan = stoi(lines[5]);

	*immediatePath = lines[3];
	*outputPath = lines[6];

	batchInput->buildBatchTiles(*immediatePath);
}

//응용프로그램을 수행
void Runner::DoRun(BatchInput& batchInput, std::string const& immediatePath, string const& outputPath) const
{
	//ImageFileIOHandler, ImageScaler 생성
	ImageFileIOHandler imageFileIOHandler(immediatePath, outputPath, &batchInput._BaseTilesForBatching);
	ImageScaler imageScaler(&imageFileIOHandler);


	//사용자로부터 필터 선택을 읽어옴
	int filter = RunFilterSelection();
	switch (filter)
	{
		//FreeImage 필터일 경우
	case 0:
	{
		BilinearFilter_FreeImage bilinearFilter_FreeImage;
		imageScaler.Scale(batchInput, bilinearFilter_FreeImage);
	}
	break;

	//OpenCL 필터일 경우
	case 1:
	{
		BilinearFilter_OpenCL bilinearFilter_OpenCL;
		//사용자로부터 작업을 수행할 OpenCL 디바이스 선택을 읽어옴
		int device = RunOpenCLDeviceSelection(bilinearFilter_OpenCL);
		bilinearFilter_OpenCL.SetRunningDevice(device);
		imageScaler.Scale(batchInput, bilinearFilter_OpenCL);
	}
	break;
	//Multithread 필터일 경우
	case 2:
	{
		BilinearFilter_Multithread bilinearFilter_Multithread;
		//사용자로부터 작업을 수행할 스레드의 개수를 읽어옴
		int numThreads = RunMultithreadSelection();
		bilinearFilter_Multithread.SetNumThreads(numThreads);
		imageScaler.Scale(batchInput, bilinearFilter_Multithread);
	}
	break;
	//Heterogeneous 필터인 경우
	case 3:
	{
		BilinearFilter_Multithread bilinearFilter_Multithread;
		int numThreads = RunMultithreadSelection();
		bilinearFilter_Multithread.SetNumThreads(numThreads);

		BilinearFilter_OpenCL bilinearFilter_OpenCL;
		//사용자로부터 작업을 수행할 OpenCL 디바이스 선택을 읽어옴
		int device = RunOpenCLDeviceSelection(bilinearFilter_OpenCL);
		bilinearFilter_OpenCL.SetRunningDevice(device);

		//CPU, GPU Weight를 받아옴
		int cpuWeight, gpuWeight;
		cpuWeight = RunHeterogeneousCpuSelection();
		gpuWeight = RunHeterogeneousGpuSelection();
		BilinearFilter_Heterogeneous bilinearFilter_Heterogeneous(&bilinearFilter_Multithread, &bilinearFilter_OpenCL, cpuWeight, gpuWeight);

		imageScaler.Scale(batchInput, bilinearFilter_Heterogeneous);
	}
	}

	cout << "Press ANY KEY to exit ";
	_getch();
}

//사용자로부터 필터 선택을 읽어옴
int Runner::RunFilterSelection() const
{
	cout << endl;
	cout << "Available Filters" << endl;
	cout << "0. FreeImage" << endl;
	cout << "1. OpenCL" << endl;
	cout << "2. Multithread" << endl;
	cout << "3. Heterogeneous" << endl;

	int filter;
	do
	{
		cout << "Select your filter : ";
		cin >> filter;
		if (4 <= filter)
		{
			cout << "Wrong filter number." << endl;
		}
		else
		{
			break;
		}
	} while (true);

	cout << endl;

	return filter;
}

//사용자로부터 작업을 수행할 OpenCL 디바이스 선택을 읽어옴
int Runner::RunOpenCLDeviceSelection(BilinearFilter_OpenCL& bilinearFiler) const
{
	typedef vector<string>::size_type DeviceIndexType;

	cout << endl;
	cout << "Available OpenCL devices in your system" << endl;

	vector<string> availableDevices;
	bilinearFiler.GetAvailableDevices(&availableDevices);

	DeviceIndexType deviceCount = availableDevices.size();
	for (DeviceIndexType deviceIndex = 0; deviceIndex < deviceCount; ++deviceIndex)
	{
		cout << deviceIndex << ". " << availableDevices[deviceIndex] << endl;
	}

	unsigned int device;
	do
	{
		cout << "Select your device :";
		cin >> device;
		if (deviceCount <= device)
		{
			cout << "Wrong device number." << endl;
		}
		else
		{
			break;
		}
	} while (true);

	cout << endl;

	return device;
}

//사용자로부터 작업을 수행할 스레드의 개수를 읽어옴
int Runner::RunMultithreadSelection() const
{
	cout << endl;
	cout << "Number of logical cores in your system: " << thread::hardware_concurrency() << endl;

	int numThreads;
	do
	{
		cout << "Enter the number of threads : ";
		cin >> numThreads;
		if (numThreads < 1) {
			cout << "Wrong thread number." << endl;
		}
		else
		{
			break;
		}
	} while (true);

	cout << endl;

	return numThreads;
}

//사용자로부터 CPU Weight를 읽어옴
int Runner::RunHeterogeneousCpuSelection() const
{
	cout << endl;
	int cpuWeight;
	do
	{
		cout << "Enter CPU Weight : ";
		cin >> cpuWeight;
		if (cpuWeight < 1)
		{
			cout << "Wrong CPU Weight." << endl;
		}
		else
		{
			break;
		}
	} while (true);

	return cpuWeight;
}

//사용자로부터 GPU Weight를 읽어옴
int Runner::RunHeterogeneousGpuSelection() const
{
	cout << endl;
	int gpuWeight;
	do
	{
		cout << "Enter GPU Weight : ";
		cin >> gpuWeight;
		if (gpuWeight < 1)
			{
			cout << "Wrong GPU Weight." << endl;
		}
		else
		{
			break;
		}
	} while (true);

	return gpuWeight;
}

//응응프로그램을 수행
void Runner::Run(string const& inputFilePath) const
{
	BatchInput batchInput;
	string immediatePath, outputPath;

	//입력파일을 먼저 읽고, 입력파일 형식이 올바르지 않을 경우 프로그램을 바로 종료
	Initialize(inputFilePath, &batchInput, &immediatePath, &outputPath);
	DoRun(batchInput, immediatePath, outputPath);
}
