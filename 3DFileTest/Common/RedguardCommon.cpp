#include "RedguardCommon.h"
#include <stdarg.h>
#include "devil/include/il/il.h"
#include "devil/include/il/ilu.h"
#include <time.h>
#include "windows.h"


using std::string;
using namespace uesp;


FILE* g_pLogFile = nullptr;
bool g_HasRegisteredCloseLog = false;
bool g_IsLogFileStream = false;
bool g_OutputLogLineHeader = true;
bool g_DuplicateLogToStdOut = false;


void uesp::CloseLog()
{
	if (g_pLogFile)
	{
		if (!g_IsLogFileStream) fclose(g_pLogFile);
		g_pLogFile = nullptr;
		g_IsLogFileStream = false;
	}
}


void uesp::SetLogLineHeaderOutput(const bool flag)
{
	g_OutputLogLineHeader = flag;
}


void uesp::DuplicateLogToStdOut(const bool flag)
{
	g_DuplicateLogToStdOut = flag;
}


void uesp::SendLogToStream(FILE* pStream)
{
	CloseLog();

	if (pStream)
	{
		g_IsLogFileStream = true;
		g_pLogFile = pStream;
	}

}


bool uesp::OpenLog(const char* pFilename)
{
	CloseLog();

	g_pLogFile = fopen(pFilename, "wb");
	if (g_pLogFile == nullptr) return ReportError("Error: Failed to open log file '%s' for output!", pFilename);

	if (!g_HasRegisteredCloseLog)
	{
		atexit(CloseLog);
		g_HasRegisteredCloseLog = true;
	}

	return true;
}


void uesp::PrintLogV(const char* pString, va_list Args)
{
	if (g_pLogFile == nullptr) return;

	if (g_OutputLogLineHeader)
	{
		time_t Now = time(nullptr);
		char Buffer[110];
		SYSTEMTIME SysTime;

		GetLocalTime(&SysTime);
		strftime(Buffer, 100, "%H:%M:%S", localtime(&Now));

		fprintf(g_pLogFile, "%s.%03d -- ", Buffer, SysTime.wMilliseconds);
		if (g_DuplicateLogToStdOut) printf("%s.%03d -- ", Buffer, SysTime.wMilliseconds);
	}
	
	vfprintf(g_pLogFile, pString, Args);
	fprintf(g_pLogFile, "\n");

	if (g_DuplicateLogToStdOut) 
	{
		vprintf(pString, Args);
		printf("\n");
	}

	fflush(g_pLogFile);
}


void uesp::PrintLog(const char* pString, ...)
{
	va_list Args;

	va_start(Args, pString);
	PrintLogV(pString, Args);
	va_end(Args);
}


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


bool uesp::SaveImagePng(const int Width, const int Height, std::vector<byte>& Data, const std::string Filename, const bool flip)
{
	ilTexImage(Width, Height, 1, 4, IL_RGBA, IL_UNSIGNED_BYTE, Data.data());
	if (flip) iluFlipImage();
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


bool uesp::StringEndsWith(std::string const &fullString, std::string const &ending)
{
	if (fullString.length() >= ending.length()) 
	{
		return (0 == fullString.compare(fullString.length() - ending.length(), ending.length(), ending));
	}
	else 
	{
		return false;
	}
}


bool uesp::StringEndsWithI(std::string const &fullString, std::string const &ending)
{
	if (fullString.length() >= ending.length())
	{
		return _stricmp(fullString.c_str() + fullString.length() - ending.length(), ending.c_str()) == 0;
	}
	else
	{
		return false;
	}
}

