#pragma once


#include "RedguardCommon.h"


namespace uesp {

	struct redguard_image_header_t
	{
		word XOffset;
		word YOffset;
		word Width;
		word Height;
		byte Unknown1;
		byte Unknown2;
		word Zero1;
		word Zero2;
		word FrameCount;
		word Unknown3;
		word Zero3;
		word Zero4;
		byte Unknown4;
		byte Unknown5;
		word Unknown6;
	};


	class CRedguardColFile
	{
	public:
		dword m_FileSize;
		dword m_Unknown;
		redguard_palette_t m_Palette;

	public:
		CRedguardColFile();
		~CRedguardColFile();


		bool Load(const std::string Filename);

	};


	class CRedguardTextureImage
	{
	public:
		static const dword REDGUARD_IMAGE_HEADER_SIZE = 26;

	public:
		std::string m_Name;
		dword m_Size;

		redguard_image_header_t m_Header;
		redguard_palette_t m_ColorMap;

		std::vector< byte > m_BsifData;
		bool m_HasBsifData;

		std::vector< byte > m_IfhdData;
		bool m_HasIfhdData;

		std::vector< redguard_image_data_t > m_ImageData;


	protected:

		bool ReadSubrecord(FILE* pFile, const char RecordName[5], const dword RecordSize);
		bool ReadSubrecords(FILE* pFile);

		bool ReadBsif(FILE* pFile, const dword RecordSize);
		bool ReadIfhd(FILE* pFile, const dword RecordSize);
		bool ReadHeader(FILE* pFile, const dword RecordSize);
		bool ReadCmap(FILE* pFile, const dword RecordSize);
		bool ReadData(FILE* pFile, const dword RecordSize);
		bool ReadAnimationData(FILE* pFile, const dword RecordSize);

		bool ExportAnimated(const std::string BaseFilename);
		bool ExportAnimatedFlat(const std::string BaseFilename);
		bool ExportImage(redguard_image_data_t& ImageData, const std::string Filename);


	public:
		CRedguardTextureImage();
		~CRedguardTextureImage();

		bool Export (const std::string BaseFilename, const bool ExportAnimatedGif);

		bool IsAnimated() const { return(m_ImageData.size() > 1);  }

		void SetDefaultPalette(const CRedguardColFile ColFile) { SetDefaultPalette(ColFile.m_Palette); }
		void SetDefaultPalette(const redguard_palette_t Default);

		bool Read(FILE* pFile, const std::string Name, const dword Size);
	};


	class CRedguardTexBsiFile
	{
	public:
		std::string m_Filename;
		std::vector< CRedguardTextureImage > m_Images;

		redguard_palette_t m_DefaultPalette;

	protected:

		bool ReadImage(FILE* pFile);


	public:
		CRedguardTexBsiFile();
		~CRedguardTexBsiFile();

		bool Load(const std::string Filename);

		bool ExportImages(const std::string OutputPath);

		void SetDefaultPalette(const CRedguardColFile ColFile) { SetDefaultPalette(ColFile.m_Palette); }
		void SetDefaultPalette(const redguard_palette_t Default);

	};


	static_assert(sizeof(redguard_image_header_t) == CRedguardTextureImage::REDGUARD_IMAGE_HEADER_SIZE, "Redguard image header struct is not the correct size!");

}