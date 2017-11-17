#pragma once

#include <iostream>
#include <vector>
#include <string>
#include "FreeImage.h"

//�⺻ ���� Ÿ�� ����
struct BaseTileInformation
{
public:
	BaseTileInformation(unsigned int level, unsigned int row, unsigned int column, std::string const& tilePath);

	unsigned int _Row;
	unsigned int _Column;
	std::string _TilePath;
};

//�̹��� �����Ϸ����� ������ �ϰ� �۾�
struct BatchInput
{
public:
	BatchInput();
	BatchInput(int top, int bottom, int left, int right,  int level, int superLevelSpan, int subLevelSpan);

	int _Top;
	int _Bottom;
	int _Left;
	int _Right;
	int _Level;
	int _SubLevelSpan;
	int _SuperLevelSpan;
	//�⺻ ���� Ÿ�� ���� ����Ʈ
	std::vector<BaseTileInformation> _BaseTilesForBatching;

	void buildBatchTiles(std::string const& inputPath);
};

//�̹����� ����� ���
class ImageFileIOHandler
{
public:
	ImageFileIOHandler(std::string const& immediatePath, std::string const& outputPath, std::vector<BaseTileInformation> *baseTileInformations);

	FIBITMAP* LoadTile(std::string const& filePath) const;
	void SaveTile(FIBITMAP* bitmap, unsigned int level, unsigned int row, unsigned int column) const;
	void UnloadTile(FIBITMAP* bitmap) const;
 
private:
	const std::string _ImmediatePath;
	const std::string _OutputPath;
};