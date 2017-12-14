#include "stdafx.h"
#include <Windows.h>
#include "ImageFileIOHandler.h"

using namespace std;

//배치 인풋 기본 생성자
BatchInput::BatchInput()
{
}

//배치 인풋 멤버 초기화 생성자
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

//기본 레벨 타일 정보 멤버 초기화 생성자
BaseTileInformation::BaseTileInformation(unsigned int level, unsigned int row, unsigned int column, string const& tilePath)
	: _Row(row), _Column(column), _TilePath(tilePath)
{
}

//ImageFileIOHandler 멤버 초기화 생성자
ImageFileIOHandler::ImageFileIOHandler(string const& immediatePath, string const& outputPath, std::vector<BaseTileInformation> *baseTileInformations)
	: _ImmediatePath(immediatePath), _OutputPath(outputPath)
{
}


//주어진 파일로부터 JPEG파일을 로드
FIBITMAP* ImageFileIOHandler::LoadTile(string const& filePath) const
{
	return FreeImage_Load(FIF_BMP, filePath.c_str());
}

//주어진 위치에 BMP 파일을 저장
void ImageFileIOHandler::SaveTile(FIBITMAP* bitmap, unsigned int level, unsigned int row, unsigned int column) const
{
	//저장할 디렉토리를 먼저 생성
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
