#pragma once


#include "RedguardCommon.h"


namespace uesp
{


	struct rg_3dfile_header_t
	{
		dword Version;
		dword NumVertices;
		dword NumFaces;
		dword Radius;
		dword Unknown1;	//1
		dword OffsetSection3;
		dword NumUVOffsets;
		dword OffsetSection4;
		dword Section4Count;
		dword Unknown4;	//0
		dword OffsetUVOffsets;
		dword OffsetUVData;
		dword OffsetVertexCoors;
		dword OffsetFaceNormals;
		dword NumUVOffsets2;
		dword OffsetFaceData;
	};


	struct rg_3dfile_facevertexdata_t
	{
		dword VertexIndex;
		dword Flags;
	};


	struct rd_3dfile_facedata_t
	{
		byte VertexCount;
		byte Flags;
		dword Data1;
		dword Data2;
		std::vector<rg_3dfile_facevertexdata_t> VertexData;
	};


	struct rd_3dfile_coor_t
	{
		int32_t x;
		int32_t y;
		int32_t z;
	};


	struct rd_3dfile_fcoor_t
	{
		float x;
		float y;
		float z;
	};


	struct rd_3dfile_section3_t
	{
		dword u1;
		dword u2;
		dword u3;
		dword u4;
	};


	class CRedguard3dFile
	{
	public:
		static const dword REDGUARD_HEADER_SIZE = 64;
		static const float COOR_TRANSFORM_FACTOR;
		static const float NORMAL_TRANSFORM_FACTOR;

	public:
		rg_3dfile_header_t m_Header;
		std::vector<rd_3dfile_facedata_t> m_FaceData;
		std::vector<rd_3dfile_coor_t> m_VertexCoordinates;
		std::vector<rd_3dfile_coor_t> m_FaceNormals;
		rd_3dfile_section3_t m_Section3;
		//Section4?
		std::vector<dword> m_UVOffsets;
		std::vector<rd_3dfile_fcoor_t> m_UVCoordinates;


		std::string m_FullName;
		std::string m_Name;
		long m_FileSize;
		int m_TotalFaceVertexes;

	protected:
		bool LoadFaceData(FILE* pFile);
		bool LoadVertexCoordinates(FILE* pFile);
		bool LoadFaceNormals(FILE* pFile);
		bool LoadSection3(FILE* pFile);
		bool LoadSection4(FILE* pFile);
		bool LoadUVOffsets(FILE* pFile);
		bool LoadUVCoordinates(FILE* pFile);


	public:
		CRedguard3dFile();
		~CRedguard3dFile();

		bool Load(const std::string Filename);

		bool SaveAsFbx(const std::string Filename);

		static void ReportInfo();

	};


	static_assert(sizeof(rg_3dfile_header_t) == CRedguard3dFile::REDGUARD_HEADER_SIZE, "Redguard 3D header struct is not the correct size!");

};