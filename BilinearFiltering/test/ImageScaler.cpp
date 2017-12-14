#include "stdafx.h"
#include "ImageScaler.h"
#include "Util.h"
#include <iostream>
#include <malloc.h>

using namespace std;

//ImageTile Ÿ�� �⺻ ������
ImageScaler::ImageTile::ImageTile() : _BitMap(NULL)
{
}

//ImageTile Ÿ�� ����ʱ�ȭ ������
ImageScaler::ImageTile::ImageTile(FIBITMAP* bitMap, unsigned int row, unsigned int column) : _BitMap(bitMap), _Row(row), _Column(column)
{
}

//DownScaleTask �⺻ ������
ImageScaler::DownScaleTask::DownScaleTask() : _LeftTopBitMap(NULL), _RightTopBitMap(NULL), _LeftBottomBitMap(NULL), _RightBottomBitMap(NULL)
{
}

//DownScaleTaskManager ������
//���� : ������ �����ϸ��� ������ Ÿ�ϵ��� ������
ImageScaler::DownScaleTaskManager::DownScaleTaskManager(unsigned int top, unsigned int bottom, unsigned int left, unsigned int right, int width, int height) : _BaseRow((unsigned int)(bottom / 2)), _BaseColumn((unsigned int)(left / 2))
{
	Initialize(top, bottom, left, right, width, height);
}

//DownScaleTask ����Ʈ�� �ʱ�ȭ�� ����. DownScaleTask��  �̸� ����� ����. ���߿� �� DownScaleTask�� �ش��ϴ� ��Ʈ�ʸ� �߰���.
void ImageScaler::DownScaleTaskManager::Initialize(unsigned int top, unsigned int bottom, unsigned int left, unsigned int right, int width, int height)
{
	typedef vector<vector<DownScaleTask>>::size_type size;

	size rowSize = ((size)(top / 2)) - ((size)(bottom / 2)) + 1;
	size columnSize = ((size)(right / 2)) - ((size)(left / 2)) + 1;

	_DownScaleTasks.resize(rowSize);
	for(size rowIndex = 0; rowIndex < rowSize; ++rowIndex)
	{
		vector<DownScaleTask>& downScaleTaskForRow = _DownScaleTasks[rowIndex];
		downScaleTaskForRow.resize(columnSize);
		for(size columnIndex = 0; columnIndex < columnSize; ++columnIndex)
		{
			DownScaleTask& downScaleTask = downScaleTaskForRow[columnIndex];
			downScaleTask._BaseRow = (rowIndex + _BaseRow) * 2;
			downScaleTask._BaseColumn = (columnIndex + _BaseColumn) * 2;
		}
	}
}

//�ش� ��� ���� DownScaleTask���� ��ȿ�� �ε��� �������� �˻�
bool ImageScaler::DownScaleTaskManager::ValidActualIndex(int actualRow, int actualColumn) const
{
	if((-1 < actualRow) && (actualRow < (int)(_DownScaleTasks.size())))
	{
		if((-1 < actualColumn) && (actualColumn < (int)(_DownScaleTasks[actualRow].size())))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

//�ش� ��� ���� DownScaleTask�� ���� ���տ��� ������/����, ��/�ϴ������� �˷���
SubTileType ImageScaler::DownScaleTaskManager::GetSubTileType(unsigned int row, unsigned int column) const
{
	bool isRowEven = (((row % 2) == 0) ? true : false);
	bool isColumnEven = (((column % 2) == 0) ? true : false);
	if((isRowEven == true) && (isColumnEven == true))
	{
		return LeftBottom;
	}
	else
		if((isRowEven == false) && isColumnEven == true)
		{
			return LeftTop;
		}
		else
			if((isRowEven == true) && isColumnEven == false)
			{
				return RightBottom;
			}
			else
			{
				return RightTop;
			}
}

//�־��� ���� �࿡ �ش��ϴ� ��Ʈ�ʿ� �ش��ϴ� DownScaleTask�� ã�Ƽ� �߰�
void ImageScaler::DownScaleTaskManager::Add(FIBITMAP* bitMap, unsigned int row, unsigned int column)
{
	//����Ʈ���� ���� �ε������� ���
	int actualRow = ((int)(row / 2)) - _BaseRow;
	int actualColumn = ((int)(column / 2)) - _BaseColumn;
	
	if(ValidActualIndex(actualRow, actualColumn) == true)
	{
		//��/�ϴ�, ������/���� ������ �˻��Ͽ� �ش� ��ġ�� �߰�
		switch(GetSubTileType(row, column))
		{
		case LeftTop:
			_DownScaleTasks[actualRow][actualColumn]._LeftTopBitMap = bitMap;
			break;

		case RightTop:
			_DownScaleTasks[actualRow][actualColumn]._RightTopBitMap = bitMap;
			break;

		case LeftBottom:
			_DownScaleTasks[actualRow][actualColumn]._LeftBottomBitMap = bitMap;
			break;

		case RightBottom:
			_DownScaleTasks[actualRow][actualColumn]._RightBottomBitMap = bitMap;
			break;
		}
	}
}

//2���� DownScaleTask�� 1���� ����Ʈ�� ����ȭ
void ImageScaler::DownScaleTaskManager::ToList(std::vector<DownScaleTask>* downScaleTaskList) const
{
	typedef vector<vector<DownScaleTask>>::size_type size;

	size rowSize = _DownScaleTasks.size();
	for(size rowIndex = 0; rowIndex < rowSize; ++rowIndex)
	{
		vector<DownScaleTask> const& downScaleTaskForRow = _DownScaleTasks[rowIndex];
		size columnSize = downScaleTaskForRow.size();
		for(size columnIndex = 0; columnIndex < columnSize; ++columnIndex)
		{
			downScaleTaskList->push_back(downScaleTaskForRow[columnIndex]);
		}
	}
}

ImageScaler::ImageScaler(ImageFileIOHandler *imageFileIOHandler) : _ImageFileIOHandler(imageFileIOHandler)
{
}

//���� ���� Ÿ���� ��������� �����Ͽ� ���� ���� �̹��� Ÿ���� �����ϰ� ���Ϸ� ����
//newSubImageTiles : ������ҷ� ������ ���� ���� ������ �̹��� Ÿ�ϸ���Ʈ
void ImageScaler::Subdivide(std::vector<ImageTile> const& imageTiles, BilinearFilter& filter, int width, int height, int level, 
	std::vector<ImageTile>* newSubImageTiles)
{
	typedef vector<ImageTile>::size_type ImageTileIndex;
	typedef vector<FIBITMAP*>::size_type BitMapIndex;
	typedef vector<QuadBitMap>::size_type QuadBitMapIndex;
	
	if(imageTiles.empty() == false)
	{
		ImageTileIndex downScaleTaskCount = imageTiles.size();
		vector<FIBITMAP*> sources(downScaleTaskCount);
		for(ImageTileIndex downScaleTaskIndex = 0; downScaleTaskIndex < downScaleTaskCount; ++downScaleTaskIndex)
		{
			sources[downScaleTaskIndex] = imageTiles[downScaleTaskIndex]._BitMap;
		}

		vector<QuadBitMap> destinations;
		FIBITMAP* firstBitMap = imageTiles.front()._BitMap;
		_PerformanceTimer.Start();
		//Ÿ�ϵ��� �ϰ������� �������ϸ��� ����
		filter.Upsample(sources, width, height, FreeImage_GetPitch(firstBitMap), FreeImage_GetBPP(firstBitMap) / 8, &destinations); 
		_TotalScalingTime += _PerformanceTimer.Stop();

		//���� ���� ������ Ÿ�ϵ��� �����ϰ� ���Ϸ� ����
		QuadBitMapIndex quadBitMapCount = destinations.size();
		unsigned int subLevel = level - 1;
		for(QuadBitMapIndex quadBitMapIndex = 0; quadBitMapIndex < quadBitMapCount; ++quadBitMapIndex)
		{
			ImageTile const& scaleTask = imageTiles[quadBitMapIndex];
			unsigned int bottom = scaleTask._Row * 2;
			unsigned int top = bottom + 1;
			unsigned int left = scaleTask._Column * 2;
			unsigned int right = left + 1;

			QuadBitMap const& quadBitMap = destinations[quadBitMapIndex];
			_ImageFileIOHandler->SaveTile(quadBitMap._LeftTop, subLevel, top, left);
			_ImageFileIOHandler->SaveTile(quadBitMap._RightTop, subLevel, top, right);
			_ImageFileIOHandler->SaveTile(quadBitMap._LeftBottom, subLevel, bottom, left);
			_ImageFileIOHandler->SaveTile(quadBitMap._RightBottom, subLevel, bottom, right);

			newSubImageTiles->push_back(ImageTile(quadBitMap._LeftTop, top, left));
			newSubImageTiles->push_back(ImageTile(quadBitMap._RightTop, top, right));
			newSubImageTiles->push_back(ImageTile(quadBitMap._LeftBottom, bottom, left));
			newSubImageTiles->push_back(ImageTile(quadBitMap._RightBottom, bottom, right));
		}
	}
}

//�ٿ�� �����ϸ� ����
//imageTiles : �⺻ ������ �̹��� Ÿ�� ����Ʈ
void ImageScaler::UpScale(std::vector<ImageTile> const& imageTiles, BilinearFilter& filter, unsigned int level, unsigned int subLevelSpan)
{
	typedef vector<ImageTile>::size_type ImageTileIndexType;

	if(imageTiles.empty() == false)
	{
		vector<ImageTile> imageTilesForTask = imageTiles;

		FIBITMAP* first = imageTiles.front()._BitMap;
		int width = FreeImage_GetWidth(first);
		int height = FreeImage_GetHeight(first);
		unsigned int currentLevel = level;
		vector<ImageTile> nextLevelTiles;
		vector<ImageTile>* currentTiles = &imageTilesForTask;
		vector<ImageTile>* nextTiles = &nextLevelTiles;
		//�������� Ÿ�� ��������� ����
		for(unsigned int count = 0; count < subLevelSpan; ++count)
		{
			Subdivide(*currentTiles, filter, width, height, currentLevel, nextTiles);

			if (currentLevel < level)
			{
				ImageTileIndexType totalImageTiles = currentTiles->size();
				for(ImageTileIndexType imageTileIndex = 0; imageTileIndex < totalImageTiles; ++imageTileIndex)
				{
					_ImageFileIOHandler->UnloadTile((*currentTiles)[imageTileIndex]._BitMap);
				}
			}
			vector<ImageTile>* temporary = currentTiles;
			currentTiles = nextTiles;
			nextTiles = temporary;
			currentLevel -= 1;
			nextTiles->clear();
		}
		if (currentLevel < level)
		{
			ImageTileIndexType totalImageTiles = currentTiles->size();
			for (ImageTileIndexType imageTileIndex = 0; imageTileIndex < totalImageTiles; ++imageTileIndex)
			{
				_ImageFileIOHandler->UnloadTile((*currentTiles)[imageTileIndex]._BitMap);
			}

		}
	}
}

//������ �����ϸ� ����
//imageTiles : �⺻ ������ �̹��� Ÿ�� ����Ʈ
void ImageScaler::DownScale(vector<ImageTile> const& imageTiles, BilinearFilter& filter, unsigned int level, unsigned int top, unsigned int bottom,
	unsigned int left, unsigned int right, unsigned int superLevelSpan)
{	
	typedef vector<ImageTile>::size_type tile_size;
	typedef vector<DownScaleTask>::size_type task_size;

	if(imageTiles.empty() == false)
	{
		FIBITMAP* firstBitMap = imageTiles[0]._BitMap;
		int bytePerPixel = FreeImage_GetBPP(firstBitMap) / 8;
		int width = FreeImage_GetWidth(firstBitMap);
		int height = FreeImage_GetHeight(firstBitMap);
		int pitch = FreeImage_GetPitch(firstBitMap);

		vector<ImageTile> imageTilesForTask = imageTiles;
		unsigned int levelTop = top;
		unsigned int levelBottom = bottom;
		unsigned int levelLeft = left;
		unsigned int levelRight = right;
		unsigned int targetLevel = level + 1;
		//�������� ���庴���� ����
		for(unsigned int count = 0; count < superLevelSpan; ++count)
		{
			//taskManager���� ��Ʈ�ʵ��� ��,�� ������ ������ �Բ� ������ ������ ��Ʈ�ʵ鳢�� ����
			DownScaleTaskManager taskManager(levelTop, levelBottom, levelLeft, levelRight, width, height);
			tile_size imageTileCount = imageTilesForTask.size();
			for(tile_size imageTileIndex = 0; imageTileIndex < imageTileCount; ++imageTileIndex)
			{
				ImageTile const& imageTile = imageTilesForTask[imageTileIndex];
				taskManager.Add(imageTile._BitMap, imageTile._Row, imageTile._Column);
			}
			//DownscaleTask����� ����ȭ ����Ʈ�� ��ȯ
			vector<DownScaleTask> tasks;
			taskManager.ToList(&tasks);

			imageTilesForTask.clear();
			//���� ������ ����
			Merge(tasks, filter, targetLevel, width, height, pitch, bytePerPixel, &imageTilesForTask);
			
			if (level < (targetLevel - 1))
			{
				task_size totalTasks = tasks.size();
				for (tile_size taskIndex = 0; taskIndex < totalTasks; ++taskIndex)
				{
					DownScaleTask const& task = tasks[taskIndex];
					_ImageFileIOHandler->UnloadTile(task._LeftTopBitMap);
					_ImageFileIOHandler->UnloadTile(task._RightTopBitMap);
					_ImageFileIOHandler->UnloadTile(task._LeftBottomBitMap);
					_ImageFileIOHandler->UnloadTile(task._RightBottomBitMap);
				}
			}

			levelTop = (unsigned int)(levelTop / 2);
			levelBottom = (unsigned int)(levelBottom / 2);
			levelLeft = (unsigned int)(levelLeft / 2);
			levelRight = (unsigned int)(levelRight / 2);
			targetLevel += 1;
		}
		if (level < (targetLevel - 1))
		{
			tile_size totalImageTiles = imageTilesForTask.size();
			for (tile_size imageTileIndex = 0; imageTileIndex < totalImageTiles; ++imageTileIndex)
			{
				_ImageFileIOHandler->UnloadTile(imageTilesForTask[imageTileIndex]._BitMap);
			}
		}
	}
}

//���� ������ ����, ���� 
//newImageTile : ���� ���� ������ �̹��� Ÿ�ϸ���Ʈ
void ImageScaler::Merge(vector<DownScaleTask> const& tasks, BilinearFilter& filter, unsigned int targetLevel, int width, int height, int pitch, 
	unsigned int bytePerPixel, std::vector<ImageTile> *newImageTile)
{
	typedef vector<DownScaleTask>::size_type DownScaleTaskIndexType;
	typedef vector<FIBITMAP*>::size_type BitMapIndexType;

	DownScaleTaskIndexType taskCount = tasks.size();
	vector<QuadBitMap> sources(taskCount);
	for(DownScaleTaskIndexType taskIndex = 0; taskIndex < taskCount; ++taskIndex)
	{
		DownScaleTask const& task = tasks[taskIndex];
		QuadBitMap& quadBitMap = sources[taskIndex];
		quadBitMap._LeftTop = task._LeftTopBitMap;
		quadBitMap._RightTop = task._RightTopBitMap;
		quadBitMap._LeftBottom = task._LeftBottomBitMap;
		quadBitMap._RightBottom = task._RightBottomBitMap;
	}

	//Ÿ�ϵ��� �ϰ������� �ٿ� �����ϸ��� ����
	vector<FIBITMAP*> destinations;
	_PerformanceTimer.Start();
	filter.Downsample(sources, width, height, pitch, bytePerPixel, &destinations);
	_TotalScalingTime += _PerformanceTimer.Stop();

	//���� ���� ������ Ÿ�ϵ��� �����ϰ� ���Ϸ� ����
	BitMapIndexType bitMapCount = destinations.size();
	for(BitMapIndexType bitMapIndex = 0; bitMapIndex < bitMapCount; ++bitMapIndex)
	{
		DownScaleTask const& task = tasks[bitMapIndex];

		unsigned int nextLevelRow = (unsigned int)(task._BaseRow / 2);
		unsigned int nextLevelColumn = (unsigned int)(task._BaseColumn / 2);
		FIBITMAP* mergedBitMap = destinations[bitMapIndex];
		
		_ImageFileIOHandler->SaveTile(mergedBitMap, targetLevel, nextLevelRow, nextLevelColumn);
		newImageTile->push_back(ImageTile(mergedBitMap, nextLevelRow, nextLevelColumn));	
	}
}

//��ġ ��ǲ������ ���� �⺻ ������ �̹��� Ÿ�ϵ��� ����
//��� �̹��� Ÿ�ϵ��� ����, ���̰� �������� �˻��Ͽ� ��ġ���� ������ false�� ��ȯ
bool ImageScaler::GetBaseTiles(BatchInput const& batchInput, std::vector<ImageTile>* baseImageTile) const
{
	bool dimensionUniform = true;
	
	//��� �̹��� Ÿ�ϵ��� ����, ���̰� �������� �˻��ϱ� ���� ����
	int uniformWidth;
	int uniformHeight;
	bool uniformWidthInitialized = false;
	bool uniformHeightInitialized = false;

	vector<BaseTileInformation> const& baseTileInformations = batchInput._BaseTilesForBatching;
	vector<BaseTileInformation>::size_type baseTileInformationCount = baseTileInformations.size();	
	baseImageTile->resize(baseTileInformationCount);
	for(vector<BaseTileInformation>::size_type baseTileInformationIndex = 0; baseTileInformationIndex < baseTileInformationCount; ++baseTileInformationIndex)
	{
		BaseTileInformation const& baseTileInformation = baseTileInformations[baseTileInformationIndex];
		FIBITMAP* bitMap = _ImageFileIOHandler->LoadTile(baseTileInformation._TilePath);
		(*baseImageTile)[baseTileInformationIndex] = ImageTile(bitMap, baseTileInformation._Row, baseTileInformation._Column);

		int width = FreeImage_GetWidth(bitMap);
		if(uniformWidthInitialized == false)
		{
			uniformWidth = width;
			uniformWidthInitialized = true;
		}

		int height = FreeImage_GetHeight(bitMap);
		if(uniformHeightInitialized == false)
		{
			uniformHeight = height;
			uniformHeightInitialized = true;
		}

		if(width != uniformWidth)
		{
			dimensionUniform = false;
			break;
		}
		if(height != uniformHeight)
		{
			dimensionUniform = false;
			break;
		}
	}

	return dimensionUniform;
}

//������/�ٿ� �����ϸ��� ����
void ImageScaler::Scale(BatchInput const& batchInput, BilinearFilter& filter)
{
	typedef vector<ImageTile>::size_type ImageTileIndexType;

	
	cout << endl;
	cout << "Running Image Scaling" << endl;
	cout << "Running Filter : " << filter.GetName() << endl;

	vector<ImageTile> baseImageTiles;
	if(GetBaseTiles(batchInput, &baseImageTiles) == true)
	{
		_TotalScalingTime = 0;

		UpScale(baseImageTiles, filter, batchInput._Level, batchInput._SubLevelSpan);
		DownScale(baseImageTiles, filter, batchInput._Level, batchInput._Top, batchInput._Bottom, batchInput._Left, batchInput._Right, batchInput._SuperLevelSpan);

		ImageTileIndexType totalImageTiles = baseImageTiles.size();
		for (ImageTileIndexType imageTileIndex = 0; imageTileIndex < totalImageTiles; ++imageTileIndex)
		{
			_ImageFileIOHandler->UnloadTile(baseImageTiles[imageTileIndex]._BitMap);
		}

		cout << "Total Scaling Time(microseconds, without File I/O time) : " << _TotalScalingTime << endl;
	}
}