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
		dword NumFrames;
		dword OffsetFrameData;
		dword NumUVOffsets;
		dword OffsetSection4;
		dword Section4Count;
		dword Unknown4;			// 0
		dword OffsetUVOffsets;
		dword OffsetUVData;
		dword OffsetVertexCoors;
		dword OffsetFaceNormals;
		dword NumUVOffsets2;
		dword OffsetFaceData;
	};


	struct rg_3dfile_framedataheader_t 
	{
		dword u1;
		dword u2;
		dword u3;
		dword u4;
	};


	struct rg_3dfile_facevertexdata_t
	{
		dword VertexIndex;
		int16_t U;
		int16_t V;
	};


	struct rd_3dfile_facedata_t
	{
		byte VertexCount;
		word U1;
		word U2;
		byte U3;
		dword U4;
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


	class CRedguard3dFile
	{
	public:
		static const dword REDGUARD_HEADER_SIZE = 64;
		static const float COOR_TRANSFORM_FACTOR;
		static const float NORMAL_TRANSFORM_FACTOR;
		static const float UV_TRANSFORM_FACTOR;

			/* Some old 3DC files have a different vertex start offset for some unknown reason. Use this to determine whether to try and reload 
			 * the vertex data if "invalid" coordinates are found. */
		static const int32_t MAX_COORVALUE_FOROLD3DCRELOAD = 1090519040;


	public:
		rg_3dfile_header_t m_Header;

			/* Set to true in some old v2.6 3DC files in order to correctly load the vertex data */
		bool m_UseAltVertexOffset;

			/* This doesn't seem to always work  (only works with CV_ROPE.3DC and CV_SKUL3.3DC */
		bool m_TryReloadOld3dcFileVertex;

		dword m_OffsetUVZeroes;
		dword m_OffsetUnknown27;

		rd_3dfile_coor_t m_MinCoor;
		rd_3dfile_coor_t m_MaxCoor;

		std::vector<rd_3dfile_facedata_t> m_FaceData;
		std::vector<rd_3dfile_coor_t> m_VertexCoordinates;
		std::vector<rd_3dfile_coor_t> m_FaceNormals;
		//rd_3dfile_framedata_t m_FrameData;
		//Section4?
		std::vector<dword> m_UVOffsets;
		std::vector<rd_3dfile_fcoor_t> m_UVCoordinates;
		rg_3dfile_framedataheader_t m_FrameDataHeader;

		std::string m_FullName;
		std::string m_Name;
		bool m_Is3DCFile;
		long m_FileSize;
		dword m_EndFaceDataOffset;
		int m_TotalFaceVertexes;
		int m_Version;


	protected:

		dword FindNextOffsetAfter(const dword Offset);

		bool ConvertVersion();
		bool ConvertHeader27();

		bool LoadHeader(FILE* pFile);
		bool LoadFaceData(FILE* pFile);
		bool LoadVertexCoordinates(FILE* pFile);
		bool LoadFaceNormals(FILE* pFile);
		bool LoadUVOffsets(FILE* pFile);
		bool LoadUVCoordinates(FILE* pFile);
		bool LoadFrameData(FILE* pFile);


	public:
		CRedguard3dFile();
		~CRedguard3dFile();

		bool Load(const std::string Filename);

		bool SaveAsFbx(const std::string Filename);

		static void ReportInfo();

	};


	static_assert(sizeof(rg_3dfile_header_t) == CRedguard3dFile::REDGUARD_HEADER_SIZE, "Redguard 3D header struct is not the correct size!");

};