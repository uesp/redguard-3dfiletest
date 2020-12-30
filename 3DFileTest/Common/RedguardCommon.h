#pragma once


#include <string>
#include <vector>
#include <iostream>
#include <IntSafe.h>
#include <unordered_map>


namespace uesp 
{

	typedef DWORD dword;
	typedef BYTE byte;
	typedef WORD word;


	struct redguard_image_color_t
	{
		byte r;
		byte g;
		byte b;
	};


	typedef std::vector< byte > redguard_image_data_t;
	typedef std::vector< redguard_image_color_t > redguard_palette_t;

	std::string ExtractFilename(const std::string Filename);

	void InitializeImageLib();
	
	bool OpenLog(const char* pFilename);
	void SendLogToStream(FILE* pStream);
	void DuplicateLogToStdOut(const bool flag);
	void CloseLog();
	void PrintLog(const char* pString, ...);
	void PrintLogV(const char* pString, va_list Args);
	void SetLogLineHeaderOutput(const bool flag);
	
	bool ReportError(const char* pMsg, ...);

	dword SwapDword(dword val);

	bool SaveImagePng(const int Width, const int Height, std::vector<byte>& Data, const std::string Filename);

	bool ConvertPaletteImageData(const int Width, const int Height, std::vector<byte>& InputData, std::vector<byte>& OutputData, redguard_palette_t& Palette);

	bool StringEndsWith(std::string const &fullString, std::string const &ending);
	bool StringEndsWithI(std::string const &fullString, std::string const &ending);

};


