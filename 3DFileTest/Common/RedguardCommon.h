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
	
	bool ReportError(const char* pMsg, ...);

	dword SwapDword(dword val);

	bool SaveImagePng(const int Width, const int Height, std::vector<byte>& Data, const std::string Filename);

	bool ConvertPaletteImageData(const int Width, const int Height, std::vector<byte>& InputData, std::vector<byte>& OutputData, redguard_palette_t& Palette);

};


