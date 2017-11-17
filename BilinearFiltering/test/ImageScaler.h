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

//�̹��� ������/�ٿ� ������ ��ġ ������
class ImageScaler
{
public :
	ImageScaler(ImageFileIOHandler *imageFileIOHandler);
	void Scale(BatchInput const& batchInput, BilinearFilter& filter);

private: 
	//�̹��� Ÿ��
	struct ImageTile
	{
	public:
		ImageTile();
		ImageTile(FIBITMAP* bitMap, unsigned int row, unsigned int column);

		//��Ʈ�� ��ü
		FIBITMAP* _BitMap;
		//��ǥ
		unsigned int _Row;
		unsigned int _Column;
	};

	//���� �� �����ϸ� �� ������ 4���� Ÿ���� ��Ÿ���� ��ü
	struct DownScaleTask
	{
	public:
		DownScaleTask();

		FIBITMAP* _LeftTopBitMap;
		FIBITMAP* _RightTopBitMap;
		FIBITMAP* _LeftBottomBitMap;
		FIBITMAP* _RightBottomBitMap;

		//������ �ϴ� ��ǥ
		unsigned int _BaseRow;
		unsigned int _BaseColumn;
	};

	//���� �� �����ϸ� �� DownScaleTask�� �������ִ� ������, ��Ʈ�ʵ��� ��,�� ������ ������ �Բ� ������ ������ ��Ʈ�ʵ鳢�� ���Ľ�Ŵ
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
		//��Ʈ���� ���� ���� �־����� �� DownScaleTask����Ʈ���� ���� �ε������� ����ϱ� ���ؼ� ���
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