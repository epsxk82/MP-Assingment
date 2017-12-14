#include "stdafx.h"
#include <Windows.h>
#include "ImageFileIOHandler.h"

using namespace std;

//��ġ ��ǲ �⺻ ������
BatchInput::BatchInput()
{
}

//��ġ ��ǲ ��� �ʱ�ȭ ������
BatchInput::BatchInput(int top, int bottom, int left, int right, int level, int superLevelSpan, int subLevelSpan)
	: _Top(top), _Bottom(bottom), _Left(left), _Right(right), _Level(level), _SubLevelSpan(subLevelSpan), _SuperLevelSpan(superLevelSpan)
{
}

void BatchInput::buildBatchTiles(string const& inputPath)
{
	for (int row = _Bottom; row <= _Top; ++row)
	{
		for (int column = _Left; column <= _Right; ++column)
		{
			_BaseTilesForBatching.push_back(BaseTileInformation((unsigned int)_Level, row, column, inputPath + "L" + ::to_string(_Level) + "\\" + ::to_string(row) + "\\" + ::to_string(column) + ".bmp"));
		}
	}
}

//�⺻ ���� Ÿ�� ���� ��� �ʱ�ȭ ������
BaseTileInformation::BaseTileInformation(unsigned int level, unsigned int row, unsigned int column, string const& tilePath)
	: _Row(row), _Column(column), _TilePath(tilePath)
{
}

//ImageFileIOHandler ��� �ʱ�ȭ ������
ImageFileIOHandler::ImageFileIOHandler(string const& immediatePath, string const& outputPath, std::vector<BaseTileInformation> *baseTileInformations)
	: _ImmediatePath(immediatePath), _OutputPath(outputPath)
{
}


//�־��� ���Ϸκ��� JPEG������ �ε�
FIBITMAP* ImageFileIOHandler::LoadTile(string const& filePath) const
{
	return FreeImage_Load(FIF_BMP, filePath.c_str());
}

//�־��� ��ġ�� BMP ������ ����
void ImageFileIOHandler::SaveTile(FIBITMAP* bitmap, unsigned int level, unsigned int row, unsigned int column) const
{
	//������ ���丮�� ���� ����
	CreateDirectoryA(_OutputPath.c_str(), NULL);
	string levelPath = _OutputPath + "L" + to_string(level);
	CreateDirectoryA(levelPath.c_str(), NULL);
	string rowPath = levelPath + "\\" + to_string(row);
	CreateDirectoryA(rowPath.c_str(), NULL);
	string filePath = rowPath + "\\" + to_string(column) + ".bmp";
	FreeImage_Save(FIF_BMP, bitmap, filePath.c_str());

	cout << filePath << " file is created." << endl;
}

void ImageFileIOHandler::UnloadTile(FIBITMAP* bitmap) const
{
	FreeImage_Unload(bitmap);
}
