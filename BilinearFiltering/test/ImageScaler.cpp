#include "stdafx.h"
#include "ImageScaler.h"
#include "Util.h"
#include <iostream>
#include <malloc.h>

using namespace std;

//ImageTile 타일 기본 생성자
ImageScaler::ImageTile::ImageTile() : _BitMap(NULL)
{
}

//ImageTile 타일 멤버초기화 생성자
ImageScaler::ImageTile::ImageTile(FIBITMAP* bitMap, unsigned int row, unsigned int column) : _BitMap(bitMap), _Row(row), _Column(column)
{
}

//DownScaleTask 기본 생성자
ImageScaler::DownScaleTask::DownScaleTask() : _LeftTopBitMap(NULL), _RightTopBitMap(NULL), _LeftBottomBitMap(NULL), _RightBottomBitMap(NULL)
{
}

//DownScaleTaskManager 생성자
//인자 : 레벨업 스케일링을 적용할 타일들의 범위값
ImageScaler::DownScaleTaskManager::DownScaleTaskManager(unsigned int top, unsigned int bottom, unsigned int left, unsigned int right, int width, int height) : _BaseRow((unsigned int)(bottom / 2)), _BaseColumn((unsigned int)(left / 2))
{
	Initialize(top, bottom, left, right, width, height);
}

//DownScaleTask 리스트의 초기화를 수행. DownScaleTask는  미리 만들어 놓음. 나중에 각 DownScaleTask에 해당하는 비트맵만 추가함.
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

//해당 행과 열이 DownScaleTask에서 유효한 인덱스 값인지를 검사
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

//해당 행과 열이 DownScaleTask의 쿼드 병합에서 오른쪽/왼쪽, 상/하단인지를 알려줌
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

//주어진 열과 행에 해당하는 비트맵에 해당하는 DownScaleTask를 찾아서 추가
void ImageScaler::DownScaleTaskManager::Add(FIBITMAP* bitMap, unsigned int row, unsigned int column)
{
	//리스트에서 실제 인덱스값을 계산
	int actualRow = ((int)(row / 2)) - _BaseRow;
	int actualColumn = ((int)(column / 2)) - _BaseColumn;
	
	if(ValidActualIndex(actualRow, actualColumn) == true)
	{
		//상/하단, 오른쪽/왼쪽 인지를 검사하여 해당 위치에 추가
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

//2차원 DownScaleTask를 1차원 리스트로 단층화
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

//레벨 별로 타일의 쿼드분할을 수행하여 하위 레벨 이미지 타일을 생성하고 파일로 저장
//newSubImageTiles : 쿼드분할로 생성된 다음 하위 레벨의 이미지 타일리스트
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
		//타일들을 일괄적으로 업스케일링을 수행
		filter.Upsample(sources, width, height, FreeImage_GetPitch(firstBitMap), FreeImage_GetBPP(firstBitMap) / 8, &destinations); 
		_TotalScalingTime += _PerformanceTimer.Stop();

		//다음 하위 레벨의 타일들을 생성하고 파일로 저장
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

//다운레벨 스케일링 수행
//imageTiles : 기본 레벨의 이미지 타일 리스트
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
		//레벨별로 타일 쿼드분할을 수행
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

//업레벨 스케일링 수행
//imageTiles : 기본 레벨의 이미지 타일 리스트
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
		//레벨별로 쿼드병합을 수행
		for(unsigned int count = 0; count < superLevelSpan; ++count)
		{
			//taskManager에서 비트맵들의 행,열 정보를 가지고 함께 병합을 수행할 비트맵들끼리 정렬
			DownScaleTaskManager taskManager(levelTop, levelBottom, levelLeft, levelRight, width, height);
			tile_size imageTileCount = imageTilesForTask.size();
			for(tile_size imageTileIndex = 0; imageTileIndex < imageTileCount; ++imageTileIndex)
			{
				ImageTile const& imageTile = imageTilesForTask[imageTileIndex];
				taskManager.Add(imageTile._BitMap, imageTile._Row, imageTile._Column);
			}
			//DownscaleTask목록을 단층화 리스트로 변환
			vector<DownScaleTask> tasks;
			taskManager.ToList(&tasks);

			imageTilesForTask.clear();
			//쿼드 병합을 수행
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

//쿼드 병합을 수행, 다음 
//newImageTile : 다음 상위 레벨의 이미지 타일리스트
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

	//타일들을 일괄적으로 다운 스케일링을 수행
	vector<FIBITMAP*> destinations;
	_PerformanceTimer.Start();
	filter.Downsample(sources, width, height, pitch, bytePerPixel, &destinations);
	_TotalScalingTime += _PerformanceTimer.Stop();

	//다음 상위 레벨의 타일들을 생성하고 파일로 저장
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

//배치 인풋정보로 부터 기본 레벨의 이미지 타일들을 생성
//모든 이미지 타일들의 넓이, 높이가 같은지를 검사하여 일치하지 않으면 false를 반환
bool ImageScaler::GetBaseTiles(BatchInput const& batchInput, std::vector<ImageTile>* baseImageTile) const
{
	bool dimensionUniform = true;
	
	//모든 이미지 타일들의 넓이, 높이가 같은지를 검사하기 위한 변수
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

//레벨업/다운 스케일링을 수행
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