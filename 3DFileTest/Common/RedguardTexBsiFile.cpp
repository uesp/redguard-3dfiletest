#include "RedguardTexBsiFile.h"
#include <algorithm>


using namespace uesp;



CRedguardColFile::CRedguardColFile() :
	m_FileSize(0), m_Unknown(0)
{

}

CRedguardColFile::~CRedguardColFile()
{
}


bool CRedguardColFile::Load(const std::string Filename)
{
	FILE* pFile = fopen(Filename.c_str(), "rb");
	size_t BytesRead;

	if (pFile == nullptr) return ReportError("Error: Failed to open the COL file '%s' for reading!", Filename.c_str());

	BytesRead = fread(&m_FileSize, 1, 4, pFile);

	if (BytesRead != 4) 
	{
		fclose(pFile);
		return ReportError("Error: Failed to read 4 bytes of FileSize from COL file header!");
	}

	BytesRead = fread(&m_Unknown, 1, 4, pFile);

	if (BytesRead != 4)
	{
		fclose(pFile);
		return ReportError("Error: Failed to read 4 bytes of Unknown from COL file header!");
	}
		
	m_Palette.resize(256);

	if (m_FileSize <= 8) return ReportError("Error: COL file has no palette data or invalid file size entry!");

	size_t ReadSize = 256*3;
	size_t NumEntries = m_FileSize - 8;

	if (NumEntries < ReadSize)
	{
		ReportError("Warning: COL size of %d is less than 256 colors!", NumEntries/3);
		ReadSize = NumEntries;
	}
	else if (NumEntries > ReadSize)
	{
		ReportError("Warning: COL size of %d is greater than 256 colors!", NumEntries/3);
	}

	BytesRead = fread(m_Palette.data(), 1, ReadSize, pFile);
	fclose(pFile);

	if (BytesRead != ReadSize) return ReportError("Error: Only read %u of %u bytes from COL file palette data!", BytesRead, ReadSize);
	return true;
}


CRedguardTextureImage::CRedguardTextureImage() :
	m_HasBsifData(false), m_HasIfhdData(false), m_Header({}), m_Size(0)
{
	//m_ColorMap.resize(256, { 0, 0, 0 });
}


CRedguardTextureImage::~CRedguardTextureImage()
{
}


bool CRedguardTextureImage::ReadBsif(FILE* pFile, const dword RecordSize)
{
	m_HasBsifData = true;
	m_HasIfhdData = false;

	m_BsifData.clear();

	if (RecordSize == 0) return true;
	
	m_BsifData.resize(RecordSize);

	size_t BytesRead = fread(m_BsifData.data(), 1, RecordSize, pFile);
	if (BytesRead != RecordSize) return ReportError("Error: Only read %u of %u bytes from image BSIF section (ending at 0x%08lX)!", BytesRead, RecordSize, ftell(pFile));

	return true;
}


bool CRedguardTextureImage::ReadIfhd(FILE* pFile, const dword RecordSize)
{
	m_HasBsifData = false;
	m_HasIfhdData = true;

	m_IfhdData.clear();

	if (RecordSize == 0) return true;

	m_IfhdData.resize(RecordSize);

	size_t BytesRead = fread(m_IfhdData.data(), 1, RecordSize, pFile);
	if (BytesRead != RecordSize) return ReportError("Error: Only read %u of %u bytes from image IFHD section (ending at 0x%08lX)!", BytesRead, RecordSize, ftell(pFile));

	return true;
}


bool CRedguardTextureImage::ReadHeader(FILE* pFile, const dword RecordSize)
{
	size_t BytesRead = fread(&m_Header, 1, REDGUARD_IMAGE_HEADER_SIZE, pFile);
	if (BytesRead != REDGUARD_IMAGE_HEADER_SIZE) return ReportError("Error: Only read %u of %u bytes from image header section (ending at 0x%08lX)!", BytesRead, REDGUARD_IMAGE_HEADER_SIZE, ftell(pFile));
	
	return true;
}


bool CRedguardTextureImage::ReadCmap(FILE* pFile, const dword RecordSize)
{
	byte Data[768] = {};
	bool ReturnResult = true;

	m_ColorMap.resize(256);

	size_t BytesRead = fread(Data, 1, 768, pFile);
	if (BytesRead != 768) ReturnResult = ReportError("Error: Only read %u of %u bytes from image CMAP section (ending at 0x%08lX)!", BytesRead, 768, ftell(pFile));

	for (int i = 0; i < 256; ++i)
	{
		m_ColorMap[i].r = Data[i * 3];
		m_ColorMap[i].g = Data[i * 3 + 1];
		m_ColorMap[i].b = Data[i * 3 + 2];
	}

	return ReturnResult;
}


bool CRedguardTextureImage::ReadData(FILE* pFile, const dword RecordSize)
{
	m_ImageData.resize(1);
	redguard_image_data_t& ImageData = m_ImageData[0];

	size_t ImageSize = (size_t)m_Header.Width * (size_t)m_Header.Height;
	if (ImageSize != RecordSize) ReportError("Warning: Image size mismatch...found %u pixels but expected %u!", RecordSize, ImageSize);

	ImageData.resize(RecordSize);

	size_t BytesRead = fread(ImageData.data(), 1, RecordSize, pFile);
	if (BytesRead != ImageSize) return ReportError("Error: Only read %u of %u bytes from image DATA section (ending at 0x%08lX)!", BytesRead, RecordSize, ftell(pFile));;

	return true;
}


bool CRedguardTextureImage::ReadAnimationData(FILE* pFile, const dword RecordSize)
{
	size_t ImageSize = (size_t)m_Header.Width * (size_t)m_Header.Height;
	size_t OffsetDataSize = m_Header.Height * m_Header.FrameCount;
	std::vector < dword > OffsetData;

	OffsetData.resize(OffsetDataSize);
	m_ImageData.resize(m_Header.FrameCount);

	long StartOffset = ftell(pFile);
	if (StartOffset <= 0) return ReportError("Error: Failed to get position in file when reading image animation data!");

	size_t BytesRead = fread(OffsetData.data(), 1, OffsetDataSize, pFile);
	if (BytesRead != OffsetDataSize) return ReportError("Error: Only read %u of %u bytes from image animation offset data (ending at 0x%08lX)!", BytesRead, OffsetDataSize, ftell(pFile));
	   
	for (size_t f = 0; f < m_Header.FrameCount; ++f)
	{
		redguard_image_data_t& ImageData = m_ImageData[f];
		ImageData.resize(ImageSize);

		for (size_t y = 0; y < m_Header.Height; ++y)
		{
			size_t i = m_Header.Height * f + y;
			size_t Offset = OffsetData[i] + (size_t) StartOffset;

			int fResult = fseek(pFile, Offset, SEEK_SET);
			if (fResult != 0) return ReportError("Error: Failed to seek to position 0x%08lX in file when reading image animation data (%u, %u)!", Offset, y, f);

			BytesRead = fread(ImageData.data() + y*m_Header.Width, 1, m_Header.Width, pFile);
			if (BytesRead != m_Header.Width) return ReportError("Error: Only read %u of %u bytes from image animation data row (%u, %u ending at 0x%08lX)!", BytesRead, m_Header.Width, y, f, ftell(pFile));
		}
	}

	int fResult = fseek(pFile, StartOffset + RecordSize, SEEK_SET);
	if (fResult != 0) return ReportError("Error: Failed to seek to end position 0x%08lX in file after reading image animation data!", StartOffset + RecordSize);

	return true;
}


bool CRedguardTextureImage::ReadSubrecord(FILE* pFile, const char RecordName[5], const dword RecordSize)
{
	if (strncmp(RecordName, "BSIF", 4) == 0) return ReadBsif(pFile, RecordSize);
	if (strncmp(RecordName, "IFHD", 4) == 0) return ReadIfhd(pFile, RecordSize);
	if (strncmp(RecordName, "BHDR", 4) == 0) return ReadHeader(pFile, RecordSize);
	if (strncmp(RecordName, "CMAP", 4) == 0) return ReadCmap(pFile, RecordSize);

	if (strncmp(RecordName, "DATA", 4) == 0) 
	{
		if (m_HasIfhdData)
			return ReadAnimationData(pFile, RecordSize);
		else if (m_HasBsifData)
			return ReadData(pFile, RecordSize);
		else
			return ReportError("Error: Image missing required BSIF or IFHD record before DATA subrecord!");
	}

	return true;
}


bool CRedguardTextureImage::ReadSubrecords(FILE* pFile)
{
	char RecordName[5] = {};
	dword RecordSize_BigEndian;
	dword RecordSize;
	size_t BytesRead;

	do
	{
		long Offset = ftell(pFile);
		
		BytesRead = fread(RecordName, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of RecordName data from texture image data (ending at 0x%08lX)!", ftell(pFile));

		ReportError("\t%08lX: Found %4.4s image section.", Offset, RecordName);

		BytesRead = fread(&RecordSize_BigEndian, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of RecordSize data from texture image data (ending at 0x%08lX)!", ftell(pFile));

		RecordSize = SwapDword(RecordSize_BigEndian);

		if (strncmp(RecordName, "END ", 4) == 0) return true;

		if (!ReadSubrecord(pFile, RecordName, RecordSize)) return false;

	} while (!feof(pFile));

	return true;
}


bool CRedguardTextureImage::Read(FILE* pFile, const std::string Name, const dword Size)
{
	m_Name = Name;
	m_Size = Size;

	if (Size == 0) return true;

	return ReadSubrecords(pFile);
}


void CRedguardTextureImage::SetDefaultPalette(const redguard_palette_t Default)
{
	m_ColorMap = Default;
	if (m_ColorMap.size() < 256) m_ColorMap.resize(256, { 0, 0, 0 });
}


bool CRedguardTextureImage::Export(const std::string BaseFilename, const bool ExportAnimatedGif)
{
	if (m_ImageData.size() == 0) return ReportError("Error: No image data to export!");
	if (m_ColorMap.size() == 0) return ReportError("Error: No image palette data has been set!");

	if (IsAnimated() && ExportAnimatedGif) 
		return ExportAnimated(BaseFilename);
	else if (IsAnimated())
		return ExportAnimatedFlat(BaseFilename);

	return ExportImage(m_ImageData[0], BaseFilename + ".png");
}


bool CRedguardTextureImage::ExportAnimated(const std::string BaseFilename)
{
	return ReportError("Error: ExportAnimated() is not yet implemented!");
	return true;
}


bool CRedguardTextureImage::ExportAnimatedFlat(const std::string BaseFilename)
{
	bool ReturnResult = true;
	int i = 0;

	for (auto& imageData : m_ImageData)
	{
		ReturnResult &= ExportImage(imageData, BaseFilename + "-" + std::to_string(i) + ".png");
		++i;
	}

	return ReturnResult;
}


bool CRedguardTextureImage::ExportImage(redguard_image_data_t& ImageData, const std::string Filename)
{
	std::vector<byte> OutputData;

	if (!ConvertPaletteImageData(m_Header.Width, m_Header.Height, ImageData, OutputData, m_ColorMap)) return false;

	return SaveImagePng(m_Header.Width, m_Header.Height, OutputData, Filename);
}


CRedguardTexBsiFile::CRedguardTexBsiFile()
{
	m_DefaultPalette.resize(256, { 0, 0, 0 });
}


CRedguardTexBsiFile::~CRedguardTexBsiFile()
{

}


bool CRedguardTexBsiFile::ExportImages(const std::string OutputPath)
{
	std::string BaseName = ExtractFilename(m_Filename);
	bool ReturnResult = true;
	int i = 0;

	BaseName.erase(std::remove(BaseName.begin(), BaseName.end(), '.'), BaseName.end());

	for (auto& image : m_Images)
	{
		std::string BaseFilename = OutputPath + BaseName + "-" + std::to_string(i);
		ReturnResult &= image.Export(BaseFilename, false);
		++i;
	}

	return ReturnResult;
}


bool CRedguardTexBsiFile::Load(const std::string Filename)
{
	FILE *pFile;

	m_Filename = Filename;

	pFile = fopen(Filename.c_str(), "rb");
	if (pFile == nullptr) return ReportError("Error: Failed to open texture file '%s' for reading!", Filename.c_str());

	while (!feof(pFile))
	{
		bool Result = ReadImage(pFile);
	}

	fclose(pFile);
	return true;
}


bool CRedguardTexBsiFile::ReadImage(FILE* pFile)
{
	char ImageName[10] = {};
	dword ImageSize = 0;
	size_t BytesRead;
	long CurOffset = ftell(pFile);

	BytesRead = fread(ImageName, 1, 9, pFile);
	if (feof(pFile)) return true;
	if (BytesRead != 9) return ReportError("Error: Failed to read 9 bytes of ImageName data from texture image data (ending at 0x%08lX)!", ftell(pFile));

	if (strncmp(ImageName, "", 9) == 0) return true;

	BytesRead = fread(&ImageSize, 1, 4, pFile);
	if (feof(pFile)) return true;
	if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of ImageSize data from texture image data (ending at 0x%08lX)!", ftell(pFile));

	ReportError("\t%08lX: Starting BSI image read.", CurOffset);

	m_Images.push_back({});
	m_Images[m_Images.size() - 1].SetDefaultPalette(m_DefaultPalette);
	return m_Images[m_Images.size() - 1].Read(pFile, ImageName, ImageSize);
}


void CRedguardTexBsiFile::SetDefaultPalette(const redguard_palette_t Default)
{
	m_DefaultPalette = Default;
	if (m_DefaultPalette.size() < 256) m_DefaultPalette.resize(256, { 0, 0, 0 });
}