// 3DFileTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "Common/Redguard3dFile.h"
#include "Common/RedguardTexBsiFile.h"
#include "Common/RedguardFbx.h"


using std::string;
using std::vector;
using namespace uesp;


const string REDGUARD_FILE_PATH = "D:\\Redguard\\Redguard\\fxart\\";
const dword REDGUARD_HEADER_SIZE = 64;

const string OUTPUT_FBX_PATH = "c:\\Temp\\fbx\\";
const string OUTPUT_TEXTURE_PATH = "c:\\Temp\\texture\\";


struct rg3d_header_t 
{
	dword Version;
	dword NumVertices;
	dword NumFaces;
	dword Radius;
	dword Unk1;
	dword Offset3;
	dword Unk2;
	dword Offset4;
	dword Unk3;
	dword Unk4;
	dword Offset5;
	dword Offset6;
	dword Offset1;
	dword Offset2;
	dword Unk5;
	dword Offset0;
};


struct rg3d_file_t 
{
	string FullName;
	string Name;
	dword Size;
	rg3d_header_t Header;

	CRedguard3dFile File;
};

std::vector<rg3d_file_t> g_FileInfos;



bool Parse3DFile(const string Filename)
{
	rg3d_file_t FileInfo;
	FILE* pFile;
	printf("\tParsing file %s...\n", Filename.c_str());

	pFile = fopen(Filename.c_str(), "rb");
	if (pFile == nullptr) return ReportError("Error: Failed to open file '%s' for reading!", Filename.c_str());

	FileInfo.FullName = Filename;
	FileInfo.Name = Filename;

	FileInfo.Name = ExtractFilename(Filename);

	fseek(pFile, 0, SEEK_END);
	FileInfo.Size = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	
	size_t BytesRead = fread(&FileInfo.Header, 1, REDGUARD_HEADER_SIZE, pFile);

	if (BytesRead != REDGUARD_HEADER_SIZE)
	{
		ReportError("Error: Only read %u of %u bytes from header in file '%s'!", BytesRead, REDGUARD_HEADER_SIZE, Filename.c_str());
	}
	
	fclose(pFile);

	std::string FbxFilename;

	FbxFilename = OUTPUT_FBX_PATH + FileInfo.Name + ".fbx";

	FileInfo.File.Load(Filename);
	FileInfo.File.SaveAsFbx(FbxFilename);

	g_FileInfos.push_back(FileInfo);

	return true;
}


bool ParseAll3DFiles(const string RootPath)
{
	string FileSpec = RootPath + "*.3D";
	WIN32_FIND_DATA FindData;
	HANDLE hFind;
	int FileCount = 0;

	printf("Parsing all Redguard 3D files in %s...\n", RootPath.c_str());
	
	hFind = FindFirstFile(FileSpec.c_str(), &FindData);
	if (hFind == INVALID_HANDLE_VALUE) return ReportError("Error: Failed to find 3D files in path '%s'!", FileSpec);

	do
	{
		string Filename = RootPath + FindData.cFileName;
		Parse3DFile(Filename);
		++FileCount;
	}
	while (FindNextFile(hFind, &FindData));
	
	FindClose(hFind);

	printf("Found %d 3D files!\n", FileCount);

	return true;
}


bool ParseAll3DCFiles(const string RootPath)
{
	string FileSpec = RootPath + "*.3DC";
	WIN32_FIND_DATA FindData;
	HANDLE hFind;
	int FileCount = 0;

	printf("Parsing all Redguard 3DC files in %s...\n", RootPath.c_str());

	hFind = FindFirstFile(FileSpec.c_str(), &FindData);
	if (hFind == INVALID_HANDLE_VALUE) return ReportError("Error: Failed to find 3DC files in path '%s'!", FileSpec);

	do
	{
		string Filename = RootPath + FindData.cFileName;
		Parse3DFile(Filename);
		++FileCount;
	} while (FindNextFile(hFind, &FindData));

	FindClose(hFind);

	printf("Found %d 3DC files!\n", FileCount);

	return true;
}


bool ParseAllTextureFiles(const string RootPath)
{
	CRedguardColFile DefaultPalette;
	string FileSpec = RootPath + "texbsi.*";
	WIN32_FIND_DATA FindData;
	HANDLE hFind;
	int FileCount = 0;
	int ErrorCount = 0;

	printf("Parsing all Redguard Texture files in %s...\n", RootPath.c_str());

	if (!DefaultPalette.Load(RootPath + "REDGUARD.COL")) ReportError("Error: Failed to load the default palette!");

	hFind = FindFirstFile(FileSpec.c_str(), &FindData);
	if (hFind == INVALID_HANDLE_VALUE) return ReportError("Error: Failed to find texture files in path '%s'!", FileSpec);

	do
	{
		CRedguardTexBsiFile TextureFile;
		string Filename = RootPath + FindData.cFileName;

		TextureFile.SetDefaultPalette(DefaultPalette);

		printf("Loading texture file %s...\n", FindData.cFileName);

		bool Result = TextureFile.Load(Filename);
		
		if (Result)
		{
			printf("\tLoaded %s with %u images!\n", Filename.c_str(), TextureFile.m_Filename.size());
			Result = TextureFile.ExportImages(OUTPUT_TEXTURE_PATH);
			if (!Result) printf("\tError: Failed to export image(s) to PNG!\n");
		}
		else
		{
			printf("\tError: Failed to load %s!\n", Filename.c_str());
			++ErrorCount;
		}

		++FileCount;

	} while (FindNextFile(hFind, &FindData));

	FindClose(hFind);

	printf("Found %d Texture files with %d errors!\n", FileCount, ErrorCount);

	return true;
}


void ShowFileInfos()
{

	printf("Showing information for %u 3D files:\n", g_FileInfos.size());

	for (const auto& Info : g_FileInfos)
	{
		printf("\t%s: %d bytes\n", Info.Name.c_str(), Info.Size);

				/* Make sure offsets are in order */
		if (Info.Header.Offset0 != 0x40) ReportError("\tOffset0 is not 0x40 (0x%04X)!", Info.Header.Offset0);
		if (Info.Header.Offset1 <= Info.Header.Offset0) ReportError("\tOffset1 is too small (0x%04X)!", Info.Header.Offset1);
		if (Info.Header.Offset1 >= Info.Header.Offset2) ReportError("\tOffset1 is too large (0x%04X)!", Info.Header.Offset1);
		if (Info.Header.Offset2 >= Info.Header.Offset3) ReportError("\tOffset2 is too large (0x%04X)!", Info.Header.Offset2);

		if (Info.Header.Offset4 == 0)
		{
			if (Info.Header.Offset5 == 0)
			{
				if (Info.Header.Offset3 >= Info.Header.Offset6) ReportError("\tOffset3 is too large (0x%04X, 0x%04X)!", Info.Header.Offset3, Info.Header.Offset5);
			}
			else
			{
				if (Info.Header.Offset3 >= Info.Header.Offset5) ReportError("\tOffset3 is too large (0x%04X, 0x%04X)!", Info.Header.Offset3, Info.Header.Offset5);
				if (Info.Header.Offset5 >= Info.Header.Offset6) ReportError("\tOffset5 is too large (0x%04X)!", Info.Header.Offset5);
			}
		}
		else
		{
			if (Info.Header.Offset3 >= Info.Header.Offset4) ReportError("\tOffset3 is too large (0x%04X, 0x%04X)!", Info.Header.Offset3, Info.Header.Offset4);

			if (Info.Header.Offset5 == 0)
			{

			}
			else
			{
				if (Info.Header.Offset4 >= Info.Header.Offset5) ReportError("\tOffset4 is too large (0x%04X)!", Info.Header.Offset4);
				if (Info.Header.Offset5 >= Info.Header.Offset6) ReportError("\tOffset5 is too large (0x%04X)!", Info.Header.Offset5);
			}
		}
		
		if (Info.Header.Offset6 >= Info.Size) ReportError("\tOffset6 is too large (0x%04X)!", Info.Header.Offset6);
	}


	printf("Name, Size, Version, NumVert, NumFace, Radius, Unk1, Offset3, Unk2, Offset4, Unk3, Unk4, Offset5, Offset6, Offset1, Offset2, Unk5, Offset0, 0Size, 1Size, 2Size, 3Size, 4Size, 5Size, 6Size\n");

	for (const auto& Info : g_FileInfos)
	{
		int rec1size = 0;
		int rec2size = 0;
		int rec3size = 0;
		int rec4size = 0;
		int rec5size = 0;
		int rec0size = 0;
		int rec6size = 0;

		rec0size = Info.Header.Offset1 - Info.Header.Offset0;
		rec1size = Info.Header.Offset2 - Info.Header.Offset1;
		rec2size = Info.Header.Offset3 - Info.Header.Offset2;
		rec3size = Info.Header.Offset4 - Info.Header.Offset3;
		rec4size = Info.Header.Offset5 - Info.Header.Offset4;
		rec5size = Info.Header.Offset6 - Info.Header.Offset5;
		rec6size = Info.Size - Info.Header.Offset6;

		if (Info.Header.Offset4 == 0)
		{
			rec3size = Info.Header.Offset5 - Info.Header.Offset3;
			rec4size = 0;
		}

		printf("%12.12s, %6d, %4.4s, %4d, %4d, %7d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d,  %5d, %5d, %5d, %5d, %5d, %5d\n", Info.Name.c_str(), Info.Size, (const char *)&Info.Header.Version, Info.Header.NumVertices, Info.Header.NumFaces, Info.Header.Radius,
					Info.Header.Unk1, Info.Header.Offset3, Info.Header.Unk2, Info.Header.Offset4, Info.Header.Unk3, Info.Header.Unk4, Info.Header.Offset5, Info.Header.Offset6,
					Info.Header.Offset1, Info.Header.Offset2, Info.Header.Unk5, Info.Header.Offset0, rec0size, rec1size, rec2size, rec3size, rec4size, rec5size, rec6size);
	}


	CRedguard3dFile::ReportInfo();
}


int main()
{
	//printf("Image Header Size = %d\n", sizeof(redguard_image_header_t));
	//printf("Header Size = %d\n", sizeof(rg3d_header_t));
	//printf("Header Size = %d\n", sizeof(Redguard3dFile_Header_t));

	InitializeImageLib();
	InitializeFbxSdkObjects();
    
	//ParseAll3DCFiles(REDGUARD_FILE_PATH);
	//ParseAll3DFiles(REDGUARD_FILE_PATH);
	ParseAllTextureFiles(REDGUARD_FILE_PATH);

	//ShowFileInfos();

	DestroyFbxSdkObjects();

	return 0;
}

