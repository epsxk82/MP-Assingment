#include "stdafx.h"
#include <conio.h>
#include "Runner.h"
#include "Util.h"

using namespace std;

//Runner ������
//�������α׷��� ����
Runner::Runner(string const& inputFilePath)
{
	Run(inputFilePath);
}

//�Է����Ϸκ��� �Է������� �о��
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

//�������α׷��� ����
void Runner::DoRun(BatchInput& batchInput, std::string const& immediatePath, string const& outputPath) const
{
	//ImageFileIOHandler, ImageScaler ����
	ImageFileIOHandler imageFileIOHandler(immediatePath, outputPath, &batchInput._BaseTilesForBatching);
	ImageScaler imageScaler(&imageFileIOHandler);


	//����ڷκ��� ���� ������ �о��
	int filter = RunFilterSelection();
	switch (filter)
	{
		//FreeImage ������ ���
	case 0:
	{
		BilinearFilter_FreeImage bilinearFilter_FreeImage;
		imageScaler.Scale(batchInput, bilinearFilter_FreeImage);
	}
	break;

	//OpenCL ������ ���
	case 1:
	{
		BilinearFilter_OpenCL bilinearFilter_OpenCL;
		//����ڷκ��� �۾��� ������ OpenCL ����̽� ������ �о��
		int device = RunOpenCLDeviceSelection(bilinearFilter_OpenCL);
		bilinearFilter_OpenCL.SetRunningDevice(device);
		imageScaler.Scale(batchInput, bilinearFilter_OpenCL);
	}
	break;
	//Multithread ������ ���
	case 2:
	{
		BilinearFilter_Multithread bilinearFilter_Multithread;
		//����ڷκ��� �۾��� ������ �������� ������ �о��
		int numThreads = RunMultithreadSelection();
		bilinearFilter_Multithread.SetNumThreads(numThreads);
		imageScaler.Scale(batchInput, bilinearFilter_Multithread);
	}
	break;
	//Heterogeneous ������ ���
	case 3:
	{
		BilinearFilter_Multithread bilinearFilter_Multithread;
		int numThreads = RunMultithreadSelection();
		bilinearFilter_Multithread.SetNumThreads(numThreads);

		BilinearFilter_OpenCL bilinearFilter_OpenCL;
		//����ڷκ��� �۾��� ������ OpenCL ����̽� ������ �о��
		int device = RunOpenCLDeviceSelection(bilinearFilter_OpenCL);
		bilinearFilter_OpenCL.SetRunningDevice(device);

		//CPU, GPU Weight�� �޾ƿ�
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

//����ڷκ��� ���� ������ �о��
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

//����ڷκ��� �۾��� ������ OpenCL ����̽� ������ �о��
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

//����ڷκ��� �۾��� ������ �������� ������ �о��
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

//����ڷκ��� CPU Weight�� �о��
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

//����ڷκ��� GPU Weight�� �о��
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

//�������α׷��� ����
void Runner::Run(string const& inputFilePath) const
{
	BatchInput batchInput;
	string immediatePath, outputPath;

	//�Է������� ���� �а�, �Է����� ������ �ùٸ��� ���� ��� ���α׷��� �ٷ� ����
	Initialize(inputFilePath, &batchInput, &immediatePath, &outputPath);
	DoRun(batchInput, immediatePath, outputPath);
}
