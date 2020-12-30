
#include "Redguard3dFile.h"
#include "RedguardFbx.h"


using std::string;
using namespace uesp;


std::unordered_map<int, int> g_FaceVectorSizes;


const float CRedguard3dFile::COOR_TRANSFORM_FACTOR = 256.0f;
const float CRedguard3dFile::NORMAL_TRANSFORM_FACTOR = 256.0f;
const float CRedguard3dFile::UV_TRANSFORM_FACTOR = 4096.0f;


CRedguard3dFile::CRedguard3dFile() : 
	m_TotalFaceVertexes(0), m_FileSize(0), m_Version(0), m_OffsetUVZeroes(0), m_OffsetUnknown27(0), m_Is3DCFile(false), m_EndFaceDataOffset(0), m_FrameDataHeader({ 0 }), m_UseAltVertexOffset(false),
	m_MinCoor({ 0 }), m_MaxCoor({ 0 }), m_TryReloadOld3dcFileVertex(true)
{

}


CRedguard3dFile::~CRedguard3dFile()
{

}


bool CRedguard3dFile::ConvertVersion()
{

	if (strncmp((const char *)&m_Header.Version, "v2.6", 4) == 0)
	{
		m_Version = 26;
		return true;
	}
	else if (strncmp((const char *)&m_Header.Version, "v2.7", 4) == 0)
	{
		m_Version = 27;
		return true;
	}
	else if (strncmp((const char *)&m_Header.Version, "v4.0", 4) == 0)
	{
		m_Version = 40;
		return true;
	}
	else if (strncmp((const char *)&m_Header.Version, "v5.0", 4) == 0)
	{
		m_Version = 50;
		return true;
	}

	m_Version = 0;
	return ReportError("Error: Unknown version '%4.4s' found!", m_Header.Version);
}


dword CRedguard3dFile::FindNextOffsetAfter(const dword Offset)
{
	dword MinOffset = m_FileSize;

	if (Offset >= (dword) m_FileSize) return  (dword)m_FileSize;

	if (m_Header.OffsetFaceData > Offset && m_Header.OffsetFaceData < MinOffset) MinOffset = m_Header.OffsetFaceData;
	if (m_Header.OffsetFaceNormals > Offset && m_Header.OffsetFaceNormals < MinOffset) MinOffset = m_Header.OffsetFaceNormals;
	if (m_Header.OffsetFrameData > Offset && m_Header.OffsetFrameData < MinOffset) MinOffset = m_Header.OffsetFrameData;
	if (m_Header.OffsetSection4 > Offset && m_Header.OffsetSection4 < MinOffset) MinOffset = m_Header.OffsetSection4;
	if (m_Header.OffsetUVData > Offset && m_Header.OffsetUVData < MinOffset) MinOffset = m_Header.OffsetUVData;
	if (m_Header.OffsetUVOffsets > Offset && m_Header.OffsetUVOffsets < MinOffset) MinOffset = m_Header.OffsetUVOffsets;
	if (m_Header.OffsetVertexCoors > Offset && m_Header.OffsetVertexCoors < MinOffset) MinOffset = m_Header.OffsetVertexCoors;

	return MinOffset;
}


bool CRedguard3dFile::LoadFaceData(FILE* pFile)
{
	size_t BytesRead;
	size_t DataSize = FindNextOffsetAfter(m_Header.OffsetFaceData) - m_Header.OffsetFaceData;

	m_FaceData.clear();
	m_FaceData.resize(m_Header.NumFaces);
	if (m_Header.NumFaces <= 0) return true;

	if (fseek(pFile, m_Header.OffsetFaceData, SEEK_SET) != 0) return ReportError("Error: Failed to seek to face data offset 0x%08lX in 3D file!", m_Header.OffsetFaceData);

	for (dword i = 0; i < m_Header.NumFaces; ++i)
	{
		rd_3dfile_facedata_t& FaceData = m_FaceData[i];

		BytesRead = fread(&FaceData.VertexCount, 1, 1, pFile);
		if (BytesRead != 1) return ReportError("Error: Failed to read 1 byte of VertexCount data from face data section in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		if (m_Version > 27)
		{
			BytesRead = fread(&FaceData.U1, 1, 2, pFile);
			if (BytesRead != 2) return ReportError("Error: Failed to read 2 bytes of U1 data from face data section in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));
		}
		else
		{
			FaceData.U1 = 0;
			BytesRead = fread(&FaceData.U1, 1, 1, pFile);
			if (BytesRead != 1) return ReportError("Error: Failed to read 1 bytes of U1 data from face data section in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));
		}

		BytesRead = fread(&FaceData.U2, 1, 2, pFile);
		if (BytesRead != 2) return ReportError("Error: Failed to read 2 bytes of U2 data from face data section in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		if (m_Version > 27)
		{
			BytesRead = fread(&FaceData.U3, 1, 1, pFile);
			if (BytesRead != 1) return ReportError("Error: Failed to read 1 byte of U3 data from face data section in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));
		}
		else
		{
			FaceData.U3 = 0;
		}

		BytesRead = fread(&FaceData.U4, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of U4 data from face data section in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		FaceData.VertexData.resize(FaceData.VertexCount);

		g_FaceVectorSizes[FaceData.VertexCount] += 1;
		m_TotalFaceVertexes += FaceData.VertexCount;

		for (dword j = 0; j < FaceData.VertexCount; ++j)
		{
			rg_3dfile_facevertexdata_t& VertexData = FaceData.VertexData[j];

			BytesRead = fread(&VertexData.VertexIndex, 1, 4, pFile);
			if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of VertexIndex data from face data section in 3D file (face %u, vertex %u, ending at offset 0x%08lX)!", i, j, ftell(pFile));

					//Convert vertex offsets to indexes in older files
			if (m_Version <= 27)
			{
				VertexData.VertexIndex = VertexData.VertexIndex / 12;
			}

			BytesRead = fread(&VertexData.U, 1, 2, pFile);
			if (BytesRead != 2) return ReportError("Error: Failed to read 2 bytes of Vertex U data from face data section in 3D file (face %u, vertex %u, ending at offset 0x%08lX)!", i, j, ftell(pFile));

			BytesRead = fread(&VertexData.V, 1, 2, pFile);
			if (BytesRead != 2) return ReportError("Error: Failed to read 2 bytes of Vertex V data from face data section in 3D file (face %u, vertex %u, ending at offset 0x%08lX)!", i, j, ftell(pFile));
		}
	}

	size_t EndOffset = ftell(pFile);
	size_t ReadSize = EndOffset - m_Header.OffsetFaceData;

	m_EndFaceDataOffset = EndOffset;

	if (ReadSize != DataSize) 
	{
		if (m_Is3DCFile && m_Version <= 27) return true;
		ReportError("Warning: Only read %u bytes of %u bytes from 3D file face data section!", ReadSize, DataSize);
	}

	return true;
}


bool CRedguard3dFile::LoadVertexCoordinates(FILE* pFile)
{
	size_t DataSize = FindNextOffsetAfter(m_Header.OffsetVertexCoors) - m_Header.OffsetVertexCoors;
	size_t BytesRead;

	m_VertexCoordinates.clear();
	m_VertexCoordinates.resize(m_Header.NumVertices);
	if (m_Header.NumVertices <= 0) return true;

	if (m_Is3DCFile && m_Version <= 27)
	{
		if (m_Header.OffsetFrameData == 0) return ReportError("Error: No frame data header found in old 3DC file data!");
		if (m_EndFaceDataOffset == 0) return ReportError("Error: No vertex data offset found for old 3DC file data!");

		dword Offset = m_EndFaceDataOffset + m_FrameDataHeader.u3;
		if (m_UseAltVertexOffset) Offset = m_EndFaceDataOffset;

		if (Offset >= (dword) m_FileSize) return ReportError("Error: Failed to seek to Vertex coordinates in 3D file at 0x%08lX (past end of file)!", Offset);

		if (fseek(pFile, Offset, SEEK_SET) != 0) return ReportError("Error: Failed to seek to Vertex coordinates in 3D file at 0x%08lX!", Offset);
	}
	else
	{
		if (fseek(pFile, m_Header.OffsetVertexCoors, SEEK_SET) != 0) return ReportError("Error: Failed to seek to Vertex coordinates in 3D file at 0x%08lX!", m_Header.OffsetVertexCoors);
	}

	for (dword i = 0; i < m_Header.NumVertices; ++i)
	{
		rd_3dfile_coor_t& Coor = m_VertexCoordinates[i];

		BytesRead = fread(&Coor.x, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of X vertex data in 3D file (vertex %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		BytesRead = fread(&Coor.y, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Y vertex data in 3D file (vertex %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		BytesRead = fread(&Coor.z, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Z vertex data in 3D file (vertex %u, ending at offset 0x%08lX)!", i, ftell(pFile));


		if (i == 0)
		{
			m_MinCoor = Coor;
			m_MaxCoor = Coor;
		}
		else
		{
			if (m_MinCoor.x > Coor.x) m_MinCoor.x = Coor.x;
			if (m_MinCoor.y > Coor.y) m_MinCoor.y = Coor.y;
			if (m_MinCoor.z > Coor.z) m_MinCoor.z = Coor.z;
			if (m_MaxCoor.x < Coor.x) m_MaxCoor.x = Coor.x;
			if (m_MaxCoor.y < Coor.y) m_MaxCoor.y = Coor.y;
			if (m_MaxCoor.z < Coor.z) m_MaxCoor.z = Coor.z;
		}
	}

	size_t EndOffset = ftell(pFile);
	size_t ReadSize = EndOffset - m_Header.OffsetVertexCoors;
	
	if (ReadSize != DataSize)
	{
		if (m_Is3DCFile && m_Version <= 27) return true;
		ReportError("Warning: Only read %u bytes of %u bytes from 3D file vertex coordinates section!", ReadSize, DataSize);
	}

	return true;
}


bool CRedguard3dFile::LoadFaceNormals(FILE* pFile)
{
	size_t DataSize = FindNextOffsetAfter(m_Header.OffsetFaceNormals) - m_Header.OffsetFaceNormals;
	size_t BytesRead;

	m_FaceNormals.clear();
	m_FaceNormals.resize(m_Header.NumFaces);
	if (m_Header.NumFaces <= 0) return true;

	if (fseek(pFile, m_Header.OffsetFaceNormals, SEEK_SET) != 0) return ReportError("Error: Failed to seek to face normals in 3D file at 0x%08lX!", m_Header.OffsetFaceNormals);

	for (dword i = 0; i < m_Header.NumFaces; ++i)
	{
		rd_3dfile_coor_t& Coor = m_FaceNormals[i];

		BytesRead = fread(&Coor.x, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of X face normal data in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		BytesRead = fread(&Coor.y, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Y face normal data in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		BytesRead = fread(&Coor.z, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Z face normal data in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));
	}

	size_t EndOffset = ftell(pFile);
	size_t ReadSize = EndOffset - m_Header.OffsetFaceNormals;

	if (ReadSize != DataSize)
	{
		if (m_Is3DCFile && m_Version <= 27) return true;
		ReportError("Warning: Only read %u bytes of %u bytes from 3D file face normal section!", ReadSize, DataSize);
	}

	return true;
}


bool CRedguard3dFile::LoadUVOffsets(FILE* pFile)
{
	if (m_Header.OffsetUVOffsets == 0) return true;

	size_t DataSize = FindNextOffsetAfter(m_Header.OffsetUVOffsets) - m_Header.OffsetUVOffsets;
	size_t BytesRead;
	size_t NumUVOffsets = m_Header.NumUVOffsets;

	m_UVOffsets.clear();
	m_UVOffsets.resize(m_Header.NumUVOffsets);
	if (m_Header.NumUVOffsets <= 0) return true;

	if (fseek(pFile, m_Header.OffsetUVOffsets, SEEK_SET) != 0) return ReportError("Error: Failed to seek to UVOffsets in 3D file at 0x%08lX!", m_Header.OffsetUVOffsets);

	for (dword i = 0; i < m_Header.NumUVOffsets; ++i)
	{
		dword Offset;

		BytesRead = fread(&Offset, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of UVOffset data in 3D file (# %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		m_UVOffsets[i] = Offset;
	}

	size_t EndOffset = ftell(pFile);
	size_t ReadSize = EndOffset - m_Header.OffsetUVOffsets;

	if (ReadSize != DataSize)
	{
		ReportError("Warning: Only read %u bytes of %u bytes from 3D file UV offset data!", ReadSize, DataSize);
	}

	return true;
}


bool CRedguard3dFile::LoadFrameData(FILE* pFile)
{
	if (m_Header.OffsetFrameData == 0) return true;
	if (m_Header.OffsetFrameData >= (dword) m_FileSize) return true;

	if (fseek(pFile, m_Header.OffsetFrameData, SEEK_SET) != 0) return ReportError("Error: Failed to seek to FrameData in 3D file at 0x%08lX!", m_Header.OffsetFrameData);

	size_t BytesRead = fread(&m_FrameDataHeader, 1, 16, pFile);
	if (BytesRead != 16) return ReportError("Error: Only read %u of %u bytes of frame data header from 3D file (ending at offset 0x%08lX)!", BytesRead, 16, ftell(pFile));

	return true;
}


bool CRedguard3dFile::LoadUVCoordinates(FILE* pFile)
{
	if (m_Header.OffsetUVData == 0) return true;
	if (m_Header.OffsetUVData >= (dword)m_FileSize) return true;

	size_t DataSize = FindNextOffsetAfter(m_Header.OffsetUVData) - m_Header.OffsetUVData;
	size_t BytesRead;
	size_t NumUVCoordinates = DataSize / 12;

	m_UVCoordinates.clear();
	m_UVCoordinates.resize(NumUVCoordinates);
	if (NumUVCoordinates <= 0) return true;

	if (fseek(pFile, m_Header.OffsetUVData, SEEK_SET) != 0) return ReportError("Error: Failed to seek to UVCoordinates in 3D file at 0x%08lX!", m_Header.OffsetUVData);

	for (dword i = 0; i < NumUVCoordinates; ++i)
	{
		rd_3dfile_fcoor_t& Coor = m_UVCoordinates[i];

		BytesRead = fread(&Coor.x, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of X UVCoordinate data in 3D file (# %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		BytesRead = fread(&Coor.y, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Y UVCoordinate data in 3D file (# %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		BytesRead = fread(&Coor.z, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Z UVCoordinate data in 3D file (# %u, ending at offset 0x%08lX)!", i, ftell(pFile));
	}

	size_t EndOffset = ftell(pFile);
	size_t ReadSize = EndOffset - m_Header.OffsetUVData;

	if (ReadSize != DataSize)
	{
		ReportError("Warning: Only read %u bytes of %u bytes from 3D file UV coordinate data!", ReadSize, DataSize);
	}

	if (EndOffset < (size_t)m_FileSize)
	{
		//ReportError("Warning: Extra %u bytes left over at end of file!", m_FileSize - EndOffset);
	}
	else if (EndOffset > (size_t)m_FileSize)
	{
		ReportError("Warning: Read %u bytes past end of file!", EndOffset - m_FileSize);
	}

	return true;
}


bool CRedguard3dFile::ConvertHeader27()
{
	m_OffsetUVZeroes = m_Header.NumUVOffsets;
	m_Header.NumUVOffsets = 0;

	m_OffsetUnknown27 = m_Header.OffsetUVData;
	m_Header.OffsetUVData = m_OffsetUVZeroes;

	return true;
}


bool CRedguard3dFile::LoadHeader(FILE* pFile)
{
	size_t BytesRead = fread(&m_Header, 1, REDGUARD_HEADER_SIZE, pFile);
	if (BytesRead != REDGUARD_HEADER_SIZE) return ReportError("Error: Only read %u of %u bytes of Header data from 3D file (ending at offset 0x%08lX)!", BytesRead, REDGUARD_HEADER_SIZE, ftell(pFile));

	ConvertVersion();

	if (m_Version <= 27)
	{
		ConvertHeader27();
	}

	return true;
}


bool CRedguard3dFile::Load(const string Filename)
{
	FILE* pFile;

	pFile = fopen(Filename.c_str(), "rb");
	if (pFile == nullptr) return ReportError("Error: Failed to open 3D file '%s' for reading!", Filename.c_str());

	m_FullName = Filename;
	m_Name = Filename;

	std::size_t dirPos = Filename.find_last_of("\\");
	if (dirPos != std::string::npos) m_Name = Filename.substr(dirPos + 1, Filename.length() - dirPos - 1);

	m_Is3DCFile = StringEndsWithI(Filename, ".3dc");

	fseek(pFile, 0, SEEK_END);
	m_FileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	if (!LoadHeader(pFile))
	{
		fclose(pFile);
		return false;
	}

	if (!LoadFrameData(pFile))
	{
		fclose(pFile);
		return false;
	}

	if (!LoadFaceData(pFile))
	{
		fclose(pFile);
		return false;
	}

	if (!LoadVertexCoordinates(pFile))
	{
		fclose(pFile);
		return false;
	}

		/* See if we need to reload an old 3DC vertex data based on if the coordinate data looks "bad" or not.
		 * This doesn't seem to work. */	 
	if (m_Is3DCFile && m_Version <= 27 && !m_UseAltVertexOffset && m_TryReloadOld3dcFileVertex)
	{
		bool reloadFile = false;
		printf("\tReloading vertex data...\n");

		if (m_MinCoor.x < -MAX_COORVALUE_FOROLD3DCRELOAD) reloadFile = true;
		if (m_MinCoor.y < -MAX_COORVALUE_FOROLD3DCRELOAD) reloadFile = true;
		if (m_MinCoor.z < -MAX_COORVALUE_FOROLD3DCRELOAD) reloadFile = true;
		if (m_MaxCoor.x > MAX_COORVALUE_FOROLD3DCRELOAD) reloadFile = true;
		if (m_MaxCoor.y > MAX_COORVALUE_FOROLD3DCRELOAD) reloadFile = true;
		if (m_MaxCoor.z > MAX_COORVALUE_FOROLD3DCRELOAD) reloadFile = true;
		
		if (reloadFile)
		{
			m_UseAltVertexOffset = true;

			if (!LoadVertexCoordinates(pFile))
			{
				fclose(pFile);
				return false;
			}
		}
	}

	if (!LoadFaceNormals(pFile))
	{
		fclose(pFile);
		return false;
	}

	if (!LoadUVOffsets(pFile))
	{
		fclose(pFile);
		return false;
	}

	if (!LoadUVCoordinates(pFile))
	{
		fclose(pFile);
		return false;
	}

	fclose(pFile);
	return true;
}


bool CRedguard3dFile::SaveAsFbx(const string Filename)
{
	return Save3DFileAsFbx(*this, Filename);
}


void CRedguard3dFile::ReportInfo()
{
	printf("Face Vector Sizes\n");

	for (auto& it : g_FaceVectorSizes)
	{
		printf("\t%d Vertices: %d\n", it.first, it.second);
	}

}


