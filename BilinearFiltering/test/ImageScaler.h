#pragma once

#include <string>
#include <vector>
#include <queue>
#include <map>
#include <list>

#include "PerformanceTimer.h"
#include "ImageFileIOHandler.h"
#include "SubTileType.h"
#include "BilinearFilter.h"

//이미지 레벨업/다운 스케일 배치 수행자
class ImageScaler
{
public :
	ImageScaler(ImageFileIOHandler *imageFileIOHandler);
	void Scale(BatchInput const& batchInput, BilinearFilter& filter);

private: 
	//이미지 타일
	struct ImageTile
	{
	public:
		ImageTile();
		ImageTile(FIBITMAP* bitMap, unsigned int row, unsigned int column);

		//비트맵 객체
		FIBITMAP* _BitMap;
		//좌표
		unsigned int _Row;
		unsigned int _Column;
	};

	//레벨 업 스케일링 시 인접한 4개의 타일을 나타내는 객체
	struct DownScaleTask
	{
	public:
		DownScaleTask();

		FIBITMAP* _LeftTopBitMap;
		FIBITMAP* _RightTopBitMap;
		FIBITMAP* _LeftBottomBitMap;
		FIBITMAP* _RightBottomBitMap;

		//오른쪽 하단 좌표
		unsigned int _BaseRow;
		unsigned int _BaseColumn;
	};

	//레벨 업 스케일링 시 DownScaleTask를 생성해주는 관리자, 비트맵들의 행,열 정보를 가지고 함께 병합을 수행할 비트맵들끼리 정렬시킴
	class DownScaleTaskManager
	{
	public:
		DownScaleTaskManager(unsigned int top, unsigned int bottom, unsigned int left, unsigned int right, int width, int height);

		void Add(FIBITMAP* bitMap, unsigned int row, unsigned int column);
		void ToList(std::vector<DownScaleTask>* downScaleTaskList) const;

	private:
		void Initialize(unsigned int top, unsigned int bottom, unsigned int left, unsigned int right, int width, int height);
		bool ValidActualIndex(int actualRow, int actualColumn) const;
		SubTileType GetSubTileType(unsigned int row, unsigned int column) const;

	private:
		std::vector<std::vector<DownScaleTask>> _DownScaleTasks;
		//비트맵의 열과 행이 주어졌을 때 DownScaleTask리스트에서 실제 인덱스값을 계산하기 위해서 사용
		unsigned int _BaseRow;
		unsigned int _BaseColumn;
	};

private:
	void UpScale(std::vector<ImageTile> const& imageTiles, BilinearFilter& filter, unsigned int level, unsigned int subLevelSpan);
	void DownScale(std::vector<ImageTile> const& imageTiles, BilinearFilter& filter, unsigned int targetLevel, unsigned int top, unsigned int bottom, 
		unsigned int left, unsigned int right, unsigned int superLevelSpan);

	void Subdivide(std::vector<ImageTile> const& imageTiles, BilinearFilter& filter, int width, int height, int level, std::vector<ImageTile>* newSubImageTiles);
	void Merge(std::vector<DownScaleTask> const& tasks, BilinearFilter& filter, unsigned int targetLevel, int width, int height, int pitch, unsigned int bytePerPixel, 
		std::vector<ImageTile> *newImageTile);

	bool GetBaseTiles(BatchInput const& batchInput, std::vector<ImageTile>* baseImageTile) const;

private:
	ImageFileIOHandler* _ImageFileIOHandler;
	PerformanceTimer _PerformanceTimer;
	__int64 _TotalScalingTime;
};