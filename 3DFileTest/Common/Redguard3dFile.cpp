
#include "Redguard3dFile.h"
#include "RedguardFbx.h"


using std::string;
using namespace uesp;


std::unordered_map<int, int> g_FaceVectorSizes;


const float CRedguard3dFile::COOR_TRANSFORM_FACTOR = 256.0f;
const float CRedguard3dFile::NORMAL_TRANSFORM_FACTOR = 256.0f;


CRedguard3dFile::CRedguard3dFile() : 
	m_TotalFaceVertexes(0), m_FileSize(0)
{

}


CRedguard3dFile::~CRedguard3dFile()
{

}


bool CRedguard3dFile::LoadFaceData(FILE* pFile)
{
	size_t BytesRead;
	size_t DataSize = m_Header.OffsetVertexCoors - m_Header.OffsetFaceData;

	m_FaceData.clear();
	m_FaceData.resize(m_Header.NumFaces);
	if (m_Header.NumFaces <= 0) return true;

	if (fseek(pFile, m_Header.OffsetFaceData, SEEK_SET) != 0) return ReportError("Error: Failed to seek to face data offset 0x%08lX in 3D file!", m_Header.OffsetFaceData);

	for (dword i = 0; i < m_Header.NumFaces; ++i)
	{
		rd_3dfile_facedata_t& FaceData = m_FaceData[i];

		BytesRead = fread(&FaceData.VertexCount, 1, 1, pFile);
		if (BytesRead != 1) return ReportError("Error: Failed to read 1 byte of VertexCount data from face data section in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		BytesRead = fread(&FaceData.Flags, 1, 1, pFile);
		if (BytesRead != 1) return ReportError("Error: Failed to read 1 byte of Flags data from face data section in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		BytesRead = fread(&FaceData.Data1, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Unknown1 data from face data section in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		BytesRead = fread(&FaceData.Data2, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Unknown2 data from face data section in 3D file (face %u, ending at offset 0x%08lX)!", i, ftell(pFile));

		FaceData.VertexData.resize(FaceData.VertexCount);

		g_FaceVectorSizes[FaceData.VertexCount] += 1;
		m_TotalFaceVertexes += FaceData.VertexCount;

		for (dword j = 0; j < FaceData.VertexCount; ++j)
		{
			rg_3dfile_facevertexdata_t& VertexData = FaceData.VertexData[j];

			BytesRead = fread(&VertexData.VertexIndex, 1, 4, pFile);
			if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of VertexIndex data from face data section in 3D file (face %u, vertex %u, ending at offset 0x%08lX)!", i, j, ftell(pFile));

			BytesRead = fread(&VertexData.Flags, 1, 4, pFile);
			if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of VertexFlags data from face data section in 3D file (face %u, vertex %u, ending at offset 0x%08lX)!", i, j, ftell(pFile));
		}
	}

	size_t EndOffset = ftell(pFile);
	size_t ReadSize = EndOffset - m_Header.OffsetFaceData;

	if (ReadSize != DataSize) 
	{
		ReportError("Warning: Only read %u bytes of %u bytes from 3D file face data section!", ReadSize, DataSize);
	}

	return true;
}


bool CRedguard3dFile::LoadVertexCoordinates(FILE* pFile)
{
	size_t DataSize = m_Header.OffsetFaceNormals - m_Header.OffsetVertexCoors;
	size_t BytesRead;

	m_VertexCoordinates.clear();
	m_VertexCoordinates.resize(m_Header.NumVertices);
	if (m_Header.NumVertices <= 0) return true;

	if (fseek(pFile, m_Header.OffsetVertexCoors, SEEK_SET) != 0) return ReportError("Error: Failed to seek to Vertex coordinates in 3D file at 0x%08lX!", m_Header.OffsetVertexCoors);

	for (dword i = 0; i < m_Header.NumVertices; ++i)
	{
		rd_3dfile_coor_t& Coor = m_VertexCoordinates[i];

		BytesRead = fread(&Coor.x, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of X vertex data in 3D file (vertex %u, ending at offset 0x%08lX)!", i, ftell(pFile));;

		BytesRead = fread(&Coor.y, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Y vertex data in 3D file (vertex %u, ending at offset 0x%08lX)!", i, ftell(pFile));;

		BytesRead = fread(&Coor.z, 1, 4, pFile);
		if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Z vertex data in 3D file (vertex %u, ending at offset 0x%08lX)!", i, ftell(pFile));;
	}

	size_t EndOffset = ftell(pFile);
	size_t ReadSize = EndOffset - m_Header.OffsetVertexCoors;

	if (ReadSize != DataSize)
	{
		ReportError("Warning: Only read %u bytes of %u bytes from 3D file vertex coordinates section!", ReadSize, DataSize);
	}

	return true;
}


bool CRedguard3dFile::LoadFaceNormals(FILE* pFile)
{
	size_t DataSize = m_Header.OffsetSection3 - m_Header.OffsetFaceNormals;
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
		ReportError("Warning: Only read %u bytes of %u bytes from 3D file face normal section!", ReadSize, DataSize);
	}

	return true;
}


bool CRedguard3dFile::LoadSection3(FILE* pFile)
{
	size_t DataSize = m_Header.OffsetSection4 - m_Header.OffsetSection3;
	size_t BytesRead;

	if (m_Header.OffsetSection4 == 0) 
	{
		if (m_Header.OffsetUVOffsets == 0)
			DataSize = m_Header.OffsetUVData - m_Header.OffsetSection3;
		else
			DataSize = m_Header.OffsetUVOffsets - m_Header.OffsetSection3;
	}	

	if (fseek(pFile, m_Header.OffsetSection3, SEEK_SET) != 0) return ReportError("Error: Failed to seek to Section3 in 3D file at 0x%08lX!", m_Header.OffsetSection3);

	BytesRead = fread(&m_Section3.u1, 1, 4, pFile);
	if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Unknown1 section3 data in 3D file (ending at offset 0x%08lX)!", ftell(pFile));

	BytesRead = fread(&m_Section3.u2, 1, 4, pFile);
	if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Unknown2 section3 data in 3D file (ending at offset 0x%08lX)!", ftell(pFile));

	BytesRead = fread(&m_Section3.u3, 1, 4, pFile);
	if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Unknown3 section3 data in 3D file (ending at offset 0x%08lX)!", ftell(pFile));

	BytesRead = fread(&m_Section3.u4, 1, 4, pFile);
	if (BytesRead != 4) return ReportError("Error: Failed to read 4 bytes of Unknown4 section3 data in 3D file (ending at offset 0x%08lX)!", ftell(pFile));

	size_t EndOffset = ftell(pFile);
	size_t ReadSize = EndOffset - m_Header.OffsetSection3;

	if (ReadSize != DataSize)
	{
		ReportError("Warning: Only read %u bytes of %u bytes from 3D file section 3 data!", ReadSize, DataSize);
	}

	return true;
}


bool CRedguard3dFile::LoadSection4(FILE* pFile)
{
	size_t DataSize = m_Header.OffsetUVOffsets - m_Header.OffsetSection4;
	//size_t BytesRead;
	
	if (m_Header.OffsetSection4 == 0) return true;

	if (fseek(pFile, m_Header.OffsetSection4, SEEK_SET) != 0) return ReportError("Error: Failed to seek to Section4 in 3D file at 0x%08lX!", m_Header.OffsetSection4);

	return true;

		//TODO

	size_t EndOffset = ftell(pFile);
	size_t ReadSize = EndOffset - m_Header.OffsetSection4;

	if (ReadSize != DataSize)
	{
		ReportError("Warning: Only read %u bytes of %u bytes from 3D file section 3 data!", ReadSize, DataSize);
	}

	return true;
}


bool CRedguard3dFile::LoadUVOffsets(FILE* pFile)
{
	size_t DataSize = m_Header.OffsetUVData - m_Header.OffsetUVOffsets;
	size_t BytesRead;

	if (m_Header.OffsetUVOffsets == 0) return true;

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


bool CRedguard3dFile::LoadUVCoordinates(FILE* pFile)
{
	size_t DataSize = m_FileSize - m_Header.OffsetUVData;
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
		ReportError("Warning: Extra %u bytes left over at end of file!", m_FileSize - EndOffset);
	}
	else if (EndOffset > (size_t)m_FileSize)
	{
		ReportError("Warning: Read %u bytes past end of file!", EndOffset - m_FileSize);
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

	fseek(pFile, 0, SEEK_END);
	m_FileSize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	size_t BytesRead = fread(&m_Header, 1, REDGUARD_HEADER_SIZE, pFile);
	if (BytesRead != REDGUARD_HEADER_SIZE) return ReportError("Error: Only read %u of %u bytes of Header data from 3D file (ending at offset 0x%08lX)!", BytesRead, REDGUARD_HEADER_SIZE, ftell(pFile));

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

	if (!LoadFaceNormals(pFile))
	{
		fclose(pFile);
		return false;
	}

	if (!LoadSection3(pFile))
	{
		fclose(pFile);
		return false;
	}

	if (!LoadSection4(pFile))
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


