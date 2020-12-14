#include "RedguardCommon.h"
#include <stdarg.h>
#include "devil/include/il/il.h"
#include "devil/include/il/ilu.h"


using std::string;
using namespace uesp;



bool uesp::ReportError(const char* pMsg, ...)
{
	va_list args;

	va_start(args, pMsg);

	vprintf(pMsg, args);
	printf("\n");

	va_end(args);

	return false;
}


dword uesp::SwapDword(dword value)
{
	value = ((value << 8) & 0xFF00FF00) | ((value >> 8) & 0xFF00FF);
	return (value << 16) | (value >> 16);
}


std::string uesp::ExtractFilename(const std::string Filename)
{
	std::size_t dirPos = Filename.find_last_of("\\");
	if (dirPos != std::string::npos) return Filename.substr(dirPos + 1, Filename.length() - dirPos - 1);
	return Filename;
}



void uesp::InitializeImageLib()
{
	ilInit();
	iluInit();
	ilEnable(IL_FILE_OVERWRITE);
}


bool uesp::SaveImagePng(const int Width, const int Height, std::vector<byte>& Data, const std::string Filename)
{
	ilTexImage(Width, Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, Data.data());
	return ilSave(IL_PNG, Filename.c_str());
}


bool uesp::ConvertPaletteImageData(const int Width, const int Height, std::vector<byte>& InputData, std::vector<byte>& OutputData, redguard_palette_t& Palette)
{
	size_t ImageSize = Width * Height;

	if (Palette.size() < 256) return ReportError("Error: Cannot convert image from palette data to 32-bit data. Supplied palette has less than 256 entries!");
	if (InputData.size() < ImageSize) return ReportError("Error: Input image has less than expected image size! Expected %u bytes but found %u bytes!", ImageSize, InputData.size());
	
	OutputData.resize(ImageSize * 4, { 0 });

	for (size_t i = 0; i < ImageSize; ++i)
	{
		redguard_image_color_t Color = Palette[InputData[i]];

		OutputData[i * 4 + 0] = Color.r;
		OutputData[i * 4 + 1] = Color.g;
		OutputData[i * 4 + 2] = Color.b;
		OutputData[i * 4 + 3] = 255;
	}
	
	return true;
}