// 3DFileTest.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "pch.h"
#include "Common/Redguard3dFile.h"
#include "Common/RedguardTexBsiFile.h"
#include "Common/RedguardFbx.h"

#include <fbxsdk.h>

#include "il/il.h"
#include "il/ilu.h"


using std::string;
using std::vector;
using namespace uesp;


const string REDGUARD_FILE_PATH = "D:\\Redguard\\Redguard\\fxart\\";
//const string REDGUARD_FILE_PATH = "D:\\EGD\\uesp\\Redguard\\3dart\\";

const dword REDGUARD_HEADER_SIZE = 64;

const string OUTPUT_FBX_PATH = "c:\\Temp\\fbx3\\";
const string OUTPUT_TEXTURE_PATH = "c:\\Temp\\texture\\";

const string REDGUARD_WORLD_FILE1 = "D:\\Redguard\\Redguard\\maps\\ISLAND.WLD";
const string REDGUARD_WORLD_FILE2 = "D:\\Redguard\\Redguard\\maps\\HIDEOUT.WLD";
const string REDGUARD_WORLD_FILE3 = "D:\\Redguard\\Redguard\\maps\\EXTPALAC.WLD";
const string REDGUARD_WORLD_FILE4 = "D:\\Redguard\\Redguard\\maps\\NECRISLE.WLD";


struct rg3d_header_t 
{
	dword Version;
	dword NumVertices;
	dword NumFaces;
	dword Radius;
	dword NumFrames;
	dword Offset3;
	dword Unk2;
	dword Offset4;
	dword Unk3;
	dword Unk4;
	dword Offset5;
	dword Offset6;
	dword Offset1;
	dword Offset2;
	dword Unk5;
	dword Offset0;
};


struct rg3d_file_t 
{
	string FullName;
	string Name;
	dword Size;
	rg3d_header_t Header;

	CRedguard3dFile File;
};

std::vector<rg3d_file_t> g_FileInfos;



bool Parse3DFile(const string Filename)
{
	rg3d_file_t FileInfo;
	FILE* pFile;
	PrintLog("\tParsing file %s...", Filename.c_str());

	pFile = fopen(Filename.c_str(), "rb");
	if (pFile == nullptr) return ReportError("Error: Failed to open file '%s' for reading!", Filename.c_str());

	FileInfo.FullName = Filename;
	FileInfo.Name = Filename;

	FileInfo.Name = ExtractFilename(Filename);

	fseek(pFile, 0, SEEK_END);
	FileInfo.Size = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	
	size_t BytesRead = fread(&FileInfo.Header, 1, REDGUARD_HEADER_SIZE, pFile);

	if (BytesRead != REDGUARD_HEADER_SIZE)
	{
		ReportError("Error: Only read %u of %u bytes from header in file '%s'!", BytesRead, REDGUARD_HEADER_SIZE, Filename.c_str());
	}
	
	fclose(pFile);

	std::string FbxFilename;

	FbxFilename = OUTPUT_FBX_PATH + FileInfo.Name + ".fbx";

	FileInfo.File.Load(Filename);
	FileInfo.File.SaveAsFbx(FbxFilename);

	g_FileInfos.push_back(FileInfo);

	return true;
}


bool ParseAll3DFiles(const string RootPath)
{
	string FileSpec = RootPath + "*.3D";
	WIN32_FIND_DATA FindData;
	HANDLE hFind;
	int FileCount = 0;

	PrintLog("Parsing all Redguard 3D files in %s...", RootPath.c_str());
	
	hFind = FindFirstFile(FileSpec.c_str(), &FindData);
	if (hFind == INVALID_HANDLE_VALUE) return ReportError("Error: Failed to find 3D files in path '%s'!", FileSpec);

	do
	{
		string Filename = RootPath + FindData.cFileName;
		Parse3DFile(Filename);
		++FileCount;
	}
	while (FindNextFile(hFind, &FindData));
	
	FindClose(hFind);

	PrintLog("Found %d 3D files!", FileCount);

	return true;
}


bool ParseAll3DCFiles(const string RootPath)
{
	string FileSpec = RootPath + "*.3DC";
	WIN32_FIND_DATA FindData;
	HANDLE hFind;
	int FileCount = 0;

	PrintLog("Parsing all Redguard 3DC files in %s...", RootPath.c_str());

	hFind = FindFirstFile(FileSpec.c_str(), &FindData);
	if (hFind == INVALID_HANDLE_VALUE) return ReportError("Error: Failed to find 3DC files in path '%s'!", FileSpec);

	do
	{
		string Filename = RootPath + FindData.cFileName;
		Parse3DFile(Filename);
		++FileCount;
	} while (FindNextFile(hFind, &FindData));

	FindClose(hFind);

	PrintLog("Found %d 3DC files!", FileCount);

	return true;
}


bool ParseAllTextureFiles(const string RootPath)
{
	CRedguardColFile DefaultPalette;
	string FileSpec = RootPath + "texbsi.*";
	WIN32_FIND_DATA FindData;
	HANDLE hFind;
	int FileCount = 0;
	int ErrorCount = 0;

	PrintLog("Parsing all Redguard Texture files in %s...", RootPath.c_str());

	if (!DefaultPalette.Load(RootPath + "REDGUARD.COL")) ReportError("Error: Failed to load the default palette!");

	hFind = FindFirstFile(FileSpec.c_str(), &FindData);
	if (hFind == INVALID_HANDLE_VALUE) return ReportError("Error: Failed to find texture files in path '%s'!", FileSpec);

	do
	{
		CRedguardTexBsiFile TextureFile;
		string Filename = RootPath + FindData.cFileName;

		TextureFile.SetDefaultPalette(DefaultPalette);

		PrintLog("Loading texture file %s...", FindData.cFileName);

		bool Result = TextureFile.Load(Filename);
		
		if (Result)
		{
			PrintLog("\tLoaded %s with %u images!", Filename.c_str(), TextureFile.m_Filename.size());
			Result = TextureFile.ExportImages(OUTPUT_TEXTURE_PATH);
			if (!Result) PrintLog("\tError: Failed to export image(s) to PNG!");
		}
		else
		{
			PrintLog("\tError: Failed to load %s!", Filename.c_str());
			++ErrorCount;
		}

		++FileCount;

	} while (FindNextFile(hFind, &FindData));

	FindClose(hFind);

	PrintLog("Found %d Texture files with %d errors!", FileCount, ErrorCount);

	return true;
}


void ShowFileInfos()
{

	PrintLog("Showing information for %u 3D files:", g_FileInfos.size());

	for (const auto& Info : g_FileInfos)
	{
		PrintLog("\t%s: %d bytes", Info.Name.c_str(), Info.Size);

				/* Make sure offsets are in order */
		if (Info.Header.Offset0 != 0x40) ReportError("\tOffset0 is not 0x40 (0x%04X)!", Info.Header.Offset0);
		if (Info.Header.Offset1 <= Info.Header.Offset0) ReportError("\tOffset1 is too small (0x%04X)!", Info.Header.Offset1);
		if (Info.Header.Offset1 >= Info.Header.Offset2) ReportError("\tOffset1 is too large (0x%04X)!", Info.Header.Offset1);
		if (Info.Header.Offset2 >= Info.Header.Offset3) ReportError("\tOffset2 is too large (0x%04X)!", Info.Header.Offset2);

		if (Info.Header.Offset4 == 0)
		{
			if (Info.Header.Offset5 == 0)
			{
				if (Info.Header.Offset3 >= Info.Header.Offset6) ReportError("\tOffset3 is too large (0x%04X, 0x%04X)!", Info.Header.Offset3, Info.Header.Offset5);
			}
			else
			{
				if (Info.Header.Offset3 >= Info.Header.Offset5) ReportError("\tOffset3 is too large (0x%04X, 0x%04X)!", Info.Header.Offset3, Info.Header.Offset5);
				if (Info.Header.Offset5 >= Info.Header.Offset6) ReportError("\tOffset5 is too large (0x%04X)!", Info.Header.Offset5);
			}
		}
		else
		{
			if (Info.Header.Offset3 >= Info.Header.Offset4) ReportError("\tOffset3 is too large (0x%04X, 0x%04X)!", Info.Header.Offset3, Info.Header.Offset4);

			if (Info.Header.Offset5 == 0)
			{

			}
			else
			{
				if (Info.Header.Offset4 >= Info.Header.Offset5) ReportError("\tOffset4 is too large (0x%04X)!", Info.Header.Offset4);
				if (Info.Header.Offset5 >= Info.Header.Offset6) ReportError("\tOffset5 is too large (0x%04X)!", Info.Header.Offset5);
			}
		}
		
		if (Info.Header.Offset6 >= Info.Size) ReportError("\tOffset6 is too large (0x%04X)!", Info.Header.Offset6);
	}


	PrintLog("Name, Size, Version, NumVert, NumFace, Radius, NumFrames, Offset3, Unk2, Offset4, Unk3, Unk4, Offset5, Offset6, Offset1, Offset2, Unk5, Offset0, 0Size, 1Size, 2Size, 3Size, 4Size, 5Size, 6Size");

	for (const auto& Info : g_FileInfos)
	{
		int rec1size = 0;
		int rec2size = 0;
		int rec3size = 0;
		int rec4size = 0;
		int rec5size = 0;
		int rec0size = 0;
		int rec6size = 0;

		rec0size = Info.Header.Offset1 - Info.Header.Offset0;
		rec1size = Info.Header.Offset2 - Info.Header.Offset1;
		rec2size = Info.Header.Offset3 - Info.Header.Offset2;
		rec3size = Info.Header.Offset4 - Info.Header.Offset3;
		rec4size = Info.Header.Offset5 - Info.Header.Offset4;
		rec5size = Info.Header.Offset6 - Info.Header.Offset5;
		rec6size = Info.Size - Info.Header.Offset6;

		if (Info.Header.Offset4 == 0)
		{
			rec3size = Info.Header.Offset5 - Info.Header.Offset3;
			rec4size = 0;
		}

		PrintLog("%12.12s, %6d, %4.4s, %4d, %4d, %7d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d, %5d,  %5d, %5d, %5d, %5d, %5d, %5d", Info.Name.c_str(), Info.Size, (const char *)&Info.Header.Version, Info.Header.NumVertices, Info.Header.NumFaces, Info.Header.Radius,
					Info.Header.NumFrames, Info.Header.Offset3, Info.Header.Unk2, Info.Header.Offset4, Info.Header.Unk3, Info.Header.Unk4, Info.Header.Offset5, Info.Header.Offset6,
					Info.Header.Offset1, Info.Header.Offset2, Info.Header.Unk5, Info.Header.Offset0, rec0size, rec1size, rec2size, rec3size, rec4size, rec5size, rec6size);
	}


	CRedguard3dFile::ReportInfo();
}


void ShowFileVertexInfos()
{
	PrintLog("Name, MinX, MinY, MinZ, MaxX, MaxY, MaxZ");

	for (const auto& Info : g_FileInfos)
	{
		PrintLog("%12.12s, %d, %d, %d, %d, %d, %d", Info.Name.c_str(), Info.File.m_MinCoor.x, Info.File.m_MinCoor.y, Info.File.m_MinCoor.z, Info.File.m_MaxCoor.x, Info.File.m_MaxCoor.y, Info.File.m_MaxCoor.z);
	}
}


void ShowFileModelInfos()
{
	std::unordered_map< byte, dword > u1map, u2map, u3map, u4map, u5map;

	PrintLog("Name, Face, Model1, Model2, Model3, Model4, Model5");

	for (const auto& Info : g_FileInfos)
	{
		dword i = 0;

		for (const auto &faceData : Info.File.m_FaceData)
		{
			++i;
			//word U1;
			//word U2;
			//byte U3;

			++u1map[faceData.U1 & 0xff];
			++u2map[(faceData.U1 >> 8) & 0xff];
			++u3map[faceData.U2 & 0xff];
			++u4map[(faceData.U2 >> 8) & 0xff];
			++u5map[faceData.U3 & 0xff];

			PrintLog("%12.12s, %4d, %02X, %02X, %02X, %02X, %02X", Info.Name.c_str(), i, faceData.U1 & 0xff, (faceData.U1>>8) & 0xff, faceData.U2 & 0xff, (faceData.U2 >> 8) & 0xff, faceData.U3);
		}
	}

	PrintLog("Model U1 has %d unique values.", u1map.size());

	for (const auto& value : u1map) 
	{	
		PrintLog("\t0x%02X (%3d) = %d times", (int) value.first, (int)value.first, value.second);
	}

	PrintLog("Model U2 has %d unique values.", u2map.size());

	for (const auto& value : u2map)
	{
		PrintLog("\t0x%02X (%3d) = %d times", (int)value.first, (int)value.first, value.second);
	}

	PrintLog("Model U3 has %d unique values.", u3map.size());

	for (const auto& value : u3map)
	{
		PrintLog("\t0x%02X (%3d) = %d times", (int)value.first, (int)value.first, value.second);
	}

	PrintLog("Model U4 has %d unique values.", u4map.size());

	for (const auto& value : u4map)
	{
		PrintLog("\t0x%02X (%3d) = %d times", (int)value.first, (int)value.first, value.second);
	}

	PrintLog("Model U5 has %d unique values.", u5map.size());

	for (const auto& value : u5map)
	{
		PrintLog("\t0x%02X (%3d) = %d times", (int)value.first, (int)value.first, value.second);
	}

}

static const char* gDiffuseElementName = "DiffuseUV";
static const char* gAmbientElementName = "AmbientUV";
static const char* gEmissiveElementName = "EmissiveUV";


// Create texture for cube.
void CreateTexture(FbxScene* pScene, FbxMesh* pMesh)
{
	// A texture need to be connected to a property on the material,
	// so let's use the material (if it exists) or create a new one
	FbxSurfacePhong* lMaterial = NULL;

	//get the node of mesh, add material for it.
	FbxNode* lNode = pMesh->GetNode();
	if (lNode)
	{
		lMaterial = lNode->GetSrcObject<FbxSurfacePhong>(0);
		if (lMaterial == NULL)
		{
			FbxString lMaterialName = "toto";
			FbxString lShadingName = "Phong";
			FbxDouble3 lBlack(0.0, 0.0, 0.0);
			FbxDouble3 lRed(1.0, 0.0, 0.0);
			FbxDouble3 lDiffuseColor(0.75, 0.75, 0.0);

			FbxLayer* lLayer = pMesh->GetLayer(0);

			// Create a layer element material to handle proper mapping.
			FbxLayerElementMaterial* lLayerElementMaterial = FbxLayerElementMaterial::Create(pMesh, lMaterialName.Buffer());

			// This allows us to control where the materials are mapped.  Using eAllSame
			// means that all faces/polygons of the mesh will be assigned the same material.
			lLayerElementMaterial->SetMappingMode(FbxLayerElement::eAllSame);
			lLayerElementMaterial->SetReferenceMode(FbxLayerElement::eIndexToDirect);

			// Save the material on the layer
			lLayer->SetMaterials(lLayerElementMaterial);

			// Add an index to the lLayerElementMaterial.  Since we have only one, and are using eAllSame mapping mode,
			// we only need to add one.
			lLayerElementMaterial->GetIndexArray().Add(0);

			lMaterial = FbxSurfacePhong::Create(pScene, lMaterialName.Buffer());

			// Generate primary and secondary colors.
			lMaterial->Emissive.Set(lBlack);
			lMaterial->Ambient.Set(lRed);
			lMaterial->AmbientFactor.Set(1.);
			// Add texture for diffuse channel
			lMaterial->Diffuse.Set(lDiffuseColor);
			lMaterial->DiffuseFactor.Set(1.);
			lMaterial->TransparencyFactor.Set(0.4);
			lMaterial->ShadingModel.Set(lShadingName);
			lMaterial->Shininess.Set(0.5);
			lMaterial->Specular.Set(lBlack);
			lMaterial->SpecularFactor.Set(0.3);
			lNode->AddMaterial(lMaterial);
		}
	}

	FbxFileTexture* lTexture = FbxFileTexture::Create(pScene, "Diffuse Texture");

	// Set texture properties.
	lTexture->SetFileName("c:\\Temp\fbx3\\scene03.jpg"); // Resource file is in current directory.
	lTexture->SetTextureUse(FbxTexture::eStandard);
	lTexture->SetMappingType(FbxTexture::eUV);
	lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
	lTexture->SetSwapUV(false);
	lTexture->SetTranslation(0.0, 0.0);
	lTexture->SetScale(1.0, 1.0);
	lTexture->SetRotation(0.0, 0.0);
	lTexture->UVSet.Set(FbxString(gDiffuseElementName)); // Connect texture to the proper UV


	// don't forget to connect the texture to the corresponding property of the material
	if (lMaterial)
		lMaterial->Diffuse.ConnectSrcObject(lTexture);

	lTexture = FbxFileTexture::Create(pScene, "Ambient Texture");

	// Set texture properties.
	lTexture->SetFileName("c:\\Temp\fbx3\\gradient.jpg"); // Resource file is in current directory.
	lTexture->SetTextureUse(FbxTexture::eStandard);
	lTexture->SetMappingType(FbxTexture::eUV);
	lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
	lTexture->SetSwapUV(false);
	lTexture->SetTranslation(0.0, 0.0);
	lTexture->SetScale(1.0, 1.0);
	lTexture->SetRotation(0.0, 0.0);
	lTexture->UVSet.Set(FbxString(gAmbientElementName)); // Connect texture to the proper UV

	// don't forget to connect the texture to the corresponding property of the material
	if (lMaterial)
		lMaterial->Ambient.ConnectSrcObject(lTexture);

	lTexture = FbxFileTexture::Create(pScene, "Emissive Texture");

	// Set texture properties.
	lTexture->SetFileName("c:\\Temp\fbx3\\spotty.jpg"); // Resource file is in current directory.
	lTexture->SetTextureUse(FbxTexture::eStandard);
	lTexture->SetMappingType(FbxTexture::eUV);
	lTexture->SetMaterialUse(FbxFileTexture::eModelMaterial);
	lTexture->SetSwapUV(false);
	lTexture->SetTranslation(0.0, 0.0);
	lTexture->SetScale(1.0, 1.0);
	lTexture->SetRotation(0.0, 0.0);
	lTexture->UVSet.Set(FbxString(gEmissiveElementName)); // Connect texture to the proper UV

	// don't forget to connect the texture to the corresponding property of the material
	if (lMaterial)
		lMaterial->Emissive.ConnectSrcObject(lTexture);
}


void SetCubeDefaultPosition(FbxNode* pCube)
{
	pCube->LclTranslation.Set(FbxVector4(-75.0, -50.0, 0.0));
	pCube->LclRotation.Set(FbxVector4(0.0, 0.0, 0.0));
	pCube->LclScaling.Set(FbxVector4(1.0, 1.0, 1.0));
}


// Create a cube with a texture. 
FbxNode* CreateCubeWithTexture(FbxScene* pScene, const char* pName)
{
	int i, j;
	FbxMesh* lMesh = FbxMesh::Create(pScene, pName);

	FbxVector4 lControlPoint0(-50, 0, 50);
	FbxVector4 lControlPoint1(50, 0, 50);
	FbxVector4 lControlPoint2(50, 100, 50);
	FbxVector4 lControlPoint3(-50, 100, 50);
	FbxVector4 lControlPoint4(-50, 0, -50);
	FbxVector4 lControlPoint5(50, 0, -50);
	FbxVector4 lControlPoint6(50, 100, -50);
	FbxVector4 lControlPoint7(-50, 100, -50);

	FbxVector4 lNormalXPos(1, 0, 0);
	FbxVector4 lNormalXNeg(-1, 0, 0);
	FbxVector4 lNormalYPos(0, 1, 0);
	FbxVector4 lNormalYNeg(0, -1, 0);
	FbxVector4 lNormalZPos(0, 0, 1);
	FbxVector4 lNormalZNeg(0, 0, -1);

	// Create control points.
	lMesh->InitControlPoints(24);
	FbxVector4* lControlPoints = lMesh->GetControlPoints();

	lControlPoints[0] = lControlPoint0;
	lControlPoints[1] = lControlPoint1;
	lControlPoints[2] = lControlPoint2;
	lControlPoints[3] = lControlPoint3;
	lControlPoints[4] = lControlPoint1;
	lControlPoints[5] = lControlPoint5;
	lControlPoints[6] = lControlPoint6;
	lControlPoints[7] = lControlPoint2;
	lControlPoints[8] = lControlPoint5;
	lControlPoints[9] = lControlPoint4;
	lControlPoints[10] = lControlPoint7;
	lControlPoints[11] = lControlPoint6;
	lControlPoints[12] = lControlPoint4;
	lControlPoints[13] = lControlPoint0;
	lControlPoints[14] = lControlPoint3;
	lControlPoints[15] = lControlPoint7;
	lControlPoints[16] = lControlPoint3;
	lControlPoints[17] = lControlPoint2;
	lControlPoints[18] = lControlPoint6;
	lControlPoints[19] = lControlPoint7;
	lControlPoints[20] = lControlPoint1;
	lControlPoints[21] = lControlPoint0;
	lControlPoints[22] = lControlPoint4;
	lControlPoints[23] = lControlPoint5;


	// We want to have one normal for each vertex (or control point),
	// so we set the mapping mode to eByControlPoint.
	FbxGeometryElementNormal* lGeometryElementNormal = lMesh->CreateElementNormal();

	lGeometryElementNormal->SetMappingMode(FbxGeometryElement::eByControlPoint);

	// Here are two different ways to set the normal values.
	bool firstWayNormalCalculations = true;
	if (firstWayNormalCalculations)
	{
		// The first method is to set the actual normal value
		// for every control point.
		lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eDirect);

		lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);
	}
	else
	{
		// The second method is to the possible values of the normals
		// in the direct array, and set the index of that value
		// in the index array for every control point.
		lGeometryElementNormal->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

		// Add the 6 different normals to the direct array
		lGeometryElementNormal->GetDirectArray().Add(lNormalZPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalXPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalZNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalXNeg);
		lGeometryElementNormal->GetDirectArray().Add(lNormalYPos);
		lGeometryElementNormal->GetDirectArray().Add(lNormalYNeg);

		// Now for each control point, we need to specify which normal to use
		lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(0); // index of lNormalZPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(1); // index of lNormalXPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(2); // index of lNormalZNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(3); // index of lNormalXNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(4); // index of lNormalYPos in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
		lGeometryElementNormal->GetIndexArray().Add(5); // index of lNormalYNeg in the direct array.
	}

	// Array of polygon vertices.
	int lPolygonVertices[] = { 0, 1, 2, 3,
		4, 5, 6, 7,
		8, 9, 10, 11,
		12, 13, 14, 15,
		16, 17, 18, 19,
		20, 21, 22, 23 };

	// Create UV for Diffuse channel
	FbxGeometryElementUV* lUVDiffuseElement = lMesh->CreateElementUV(gDiffuseElementName);
	FBX_ASSERT(lUVDiffuseElement != NULL);
	lUVDiffuseElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	lUVDiffuseElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	FbxVector2 lVectors0(0, 0);
	FbxVector2 lVectors1(1, 0);
	FbxVector2 lVectors2(1, 1);
	FbxVector2 lVectors3(0, 1);

	lUVDiffuseElement->GetDirectArray().Add(lVectors0);
	lUVDiffuseElement->GetDirectArray().Add(lVectors1);
	lUVDiffuseElement->GetDirectArray().Add(lVectors2);
	lUVDiffuseElement->GetDirectArray().Add(lVectors3);


	// Create UV for Ambient channel
	FbxGeometryElementUV* lUVAmbientElement = lMesh->CreateElementUV(gAmbientElementName);

	lUVAmbientElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	lUVAmbientElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	lVectors0.Set(0, 0);
	lVectors1.Set(1, 0);
	lVectors2.Set(0, 0.418586879968643);
	lVectors3.Set(1, 0.418586879968643);

	lUVAmbientElement->GetDirectArray().Add(lVectors0);
	lUVAmbientElement->GetDirectArray().Add(lVectors1);
	lUVAmbientElement->GetDirectArray().Add(lVectors2);
	lUVAmbientElement->GetDirectArray().Add(lVectors3);

	// Create UV for Emissive channel
	FbxGeometryElementUV* lUVEmissiveElement = lMesh->CreateElementUV(gEmissiveElementName);

	lUVEmissiveElement->SetMappingMode(FbxGeometryElement::eByPolygonVertex);
	lUVEmissiveElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

	lVectors0.Set(0.2343, 0);
	lVectors1.Set(1, 0.555);
	lVectors2.Set(0.333, 0.999);
	lVectors3.Set(0.555, 0.666);

	lUVEmissiveElement->GetDirectArray().Add(lVectors0);
	lUVEmissiveElement->GetDirectArray().Add(lVectors1);
	lUVEmissiveElement->GetDirectArray().Add(lVectors2);
	lUVEmissiveElement->GetDirectArray().Add(lVectors3);

	//Now we have set the UVs as eIndexToDirect reference and in eByPolygonVertex  mapping mode
	//we must update the size of the index array.
	lUVDiffuseElement->GetIndexArray().SetCount(24);
	lUVAmbientElement->GetIndexArray().SetCount(24);
	lUVEmissiveElement->GetIndexArray().SetCount(24);



	// Create polygons. Assign texture and texture UV indices.
	for (i = 0; i < 6; i++)
	{
		//we won't use the default way of assigning textures, as we have
		//textures on more than just the default (diffuse) channel.
		lMesh->BeginPolygon(-1, -1, false);



		for (j = 0; j < 4; j++)
		{
			//this function points 
			lMesh->AddPolygon(lPolygonVertices[i * 4 + j] // Control point index. 
			);
			//Now we have to update the index array of the UVs for diffuse, ambient and emissive
			lUVDiffuseElement->GetIndexArray().SetAt(i * 4 + j, j);
			lUVAmbientElement->GetIndexArray().SetAt(i * 4 + j, j);
			lUVEmissiveElement->GetIndexArray().SetAt(i * 4 + j, j);

		}

		lMesh->EndPolygon();
	}

	FbxNode* lNode = FbxNode::Create(pScene, pName);

	lNode->SetNodeAttribute(lMesh);
	lNode->SetShadingMode(FbxNode::eTextureShading);

	CreateTexture(pScene, lMesh);

	return lNode;
}


bool CreateScene(FbxScene* pScene, const char* pSampleFileName)
{
	FbxNode* lCube = CreateCubeWithTexture(pScene, "Cube");

	SetCubeDefaultPosition(lCube);

	// Build the node tree.
	FbxNode* lRootNode = pScene->GetRootNode();
	lRootNode->AddChild(lCube);
		
	//FbxGlobalSettings& lGlobalSettings = pScene->GetGlobalSettings();

	return true;
}


#ifdef IOS_REF
	#undef  IOS_REF
	#define IOS_REF (*(pManager->GetIOSettings()))
#endif

bool SaveScene1(FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat = -1, bool pEmbedMedia = false)
{
	int lMajor, lMinor, lRevision;
	bool lStatus = true;

	// Create an exporter.
	FbxExporter* lExporter = FbxExporter::Create(pManager, "");

	if (pFileFormat < 0 || pFileFormat >= pManager->GetIOPluginRegistry()->GetWriterFormatCount())
	{
		// Write in fall back format in less no ASCII format found
		pFileFormat = pManager->GetIOPluginRegistry()->GetNativeWriterFormat();

		//Try to export in ASCII if possible
		int lFormatIndex, lFormatCount = pManager->GetIOPluginRegistry()->GetWriterFormatCount();

		for (lFormatIndex = 0; lFormatIndex < lFormatCount; lFormatIndex++)
		{
			if (pManager->GetIOPluginRegistry()->WriterIsFBX(lFormatIndex))
			{
				FbxString lDesc = pManager->GetIOPluginRegistry()->GetWriterFormatDescription(lFormatIndex);
				const char *lASCII = "ascii";
				if (lDesc.Find(lASCII) >= 0)
				{
					pFileFormat = lFormatIndex;
					break;
				}
			}
		}
	}

	// Set the export states. By default, the export states are always set to 
	// true except for the option eEXPORT_TEXTURE_AS_EMBEDDED. The code below 
	// shows how to change these states.
	IOS_REF.SetBoolProp(EXP_FBX_MATERIAL, true);
	IOS_REF.SetBoolProp(EXP_FBX_TEXTURE, true);
	IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED, pEmbedMedia);
	IOS_REF.SetBoolProp(EXP_FBX_SHAPE, true);
	IOS_REF.SetBoolProp(EXP_FBX_GOBO, true);
	IOS_REF.SetBoolProp(EXP_FBX_ANIMATION, true);
	IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

	// Initialize the exporter by providing a filename.
	if (lExporter->Initialize(pFilename, pFileFormat, pManager->GetIOSettings()) == false)
	{
		FBXSDK_printf("Call to FbxExporter::Initialize() failed.\n");
		FBXSDK_printf("Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
		return false;
	}

	FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
	FBXSDK_printf("FBX file format version %d.%d.%d\n\n", lMajor, lMinor, lRevision);

	// Export the scene.
	lStatus = lExporter->Export(pScene);

	// Destroy the exporter.
	lExporter->Destroy();
	return lStatus;
}


const size_t REDGUARD_WORLD_HEADERSIZE = 1184;
const size_t REDGUARD_WORLD_IMAGEHEADERSIZE = 22;
const size_t REDGUARD_WORLD_FOOTERSIZE = 16;
const size_t REDGUARD_WORLD_EXPORTWIDTH = 128;
const size_t REDGUARD_WORLD_EXPORTHEIGHT = 128;


typedef std::vector<byte> simpleimagedata_t;
typedef std::vector< simpleimagedata_t > imagedatas_t;


bool TestExportWorldFile(std::string filename)
{
	FILE* pFile;
	size_t BytesRead;
	std::vector<byte> Header(REDGUARD_WORLD_HEADERSIZE, 0);
	std::vector<byte> Footer(REDGUARD_WORLD_FOOTERSIZE, 0);
	simpleimagedata_t FileData;

	std::vector < std::vector< imagedatas_t > > ImageDatas;

	ImageDatas.resize(4);

	for (auto&& i : ImageDatas)
	{
		i.resize(4);
	}

	pFile = fopen(filename.c_str(), "rb");

	if (pFile == nullptr)
	{
		PrintLog("Error: Failed to open file '%s'!\n", filename.c_str());
		return false;
	}

	if (fseek(pFile, 0, SEEK_END) != 0)
	{
		fclose(pFile);
		PrintLog("Error: Failed to find end of file '%s'!\n", filename.c_str());
		return false;
	}

	fpos_t fileSize = ftell(pFile);

	if (fileSize == -1)
	{
		fclose(pFile);
		PrintLog("Error: Failed to get size of file '%s'!\n", filename.c_str());
		return false;
	}

	fseek(pFile, 0, SEEK_SET);
		
	BytesRead = fread(Header.data(), 1, REDGUARD_WORLD_HEADERSIZE, pFile);

	if (BytesRead != REDGUARD_WORLD_HEADERSIZE)
	{
		fclose(pFile);
		PrintLog("Error: Only read %d of %d bytes from world file header '%s'!\n", BytesRead, REDGUARD_WORLD_HEADERSIZE, filename.c_str());
		return false;
	}

	int dataSize = REDGUARD_WORLD_EXPORTWIDTH * REDGUARD_WORLD_EXPORTHEIGHT;
	FileData.resize(dataSize, 0);

	for (int sectionIndex = 0; sectionIndex < 4; ++sectionIndex)
	{
		byte imageHeader[REDGUARD_WORLD_IMAGEHEADERSIZE];

		BytesRead = fread(imageHeader, 1, REDGUARD_WORLD_IMAGEHEADERSIZE, pFile);

		if (BytesRead != REDGUARD_WORLD_IMAGEHEADERSIZE)
		{
			fclose(pFile);
			PrintLog("Error: Only read %d of %d bytes from section header data '%s'!\n", BytesRead, REDGUARD_WORLD_IMAGEHEADERSIZE, filename.c_str());
			return false;
		}

		for (int imageIndex = 0; imageIndex < 4; ++imageIndex)
		{
			simpleimagedata_t HeightMapImage;
			simpleimagedata_t HeightMapFlag;

			HeightMapImage.resize(dataSize, 0);
			HeightMapFlag.resize(dataSize, 0);

			BytesRead = fread(FileData.data(), 1, dataSize, pFile);

			if (BytesRead != dataSize)
			{
				fclose(pFile);
				PrintLog("Error: Only read %d of %d bytes from image section %d:%d world file data '%s'!\n", BytesRead, dataSize, sectionIndex, imageIndex, filename.c_str());
				return false;
			}

			if (imageIndex == 0)
			{
				for (int j = 0; j < dataSize; ++j)
				{
					HeightMapImage[j] = (FileData[j] & 0x7F) * 2;
					HeightMapFlag[j] = FileData[j] & 0x80;
				}
			}
			else if (imageIndex == 2)
			{
				for (int j = 0; j < dataSize; ++j)
				{
					HeightMapImage[j] = FileData[j] & 0x3F;
					HeightMapFlag[j] = FileData[j] & 0xC0;
				}
			}
			else
			{
				HeightMapImage = FileData;
			}

			//std::string OutputFile = OUTPUT_TEXTURE_PATH + ExtractFilename(filename) + ".png";
			//char OutputFile[1000];
			//snprintf(OutputFile, 900, "%s%s-%d-%d.png", OUTPUT_TEXTURE_PATH.c_str(), ExtractFilename(filename).c_str(), sectionIndex, imageIndex);

			//ilTexImage(REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, FileData.data());
			//iluFlipImage();
			//ilSave(IL_PNG, OutputFile);

			ImageDatas[sectionIndex][imageIndex].push_back(HeightMapImage);
			ImageDatas[sectionIndex][imageIndex].push_back(HeightMapFlag);
		}
	}

	char OutputFile[1000];
	ILuint imageID = ilGenImage();
	ilBindImage(imageID);

	ilTexImage(REDGUARD_WORLD_EXPORTWIDTH*2, REDGUARD_WORLD_EXPORTHEIGHT*2, 1, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, nullptr);
	
	ilSetPixels(0, 0, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[0][0][0].data());
	ilSetPixels(REDGUARD_WORLD_EXPORTWIDTH, 0, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[1][0][0].data());
	ilSetPixels(0, REDGUARD_WORLD_EXPORTHEIGHT, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[2][0][0].data());
	ilSetPixels(REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[3][0][0].data());
	iluFlipImage();
	ilConvertImage(IL_RGB, IL_INT);
	snprintf(OutputFile, 900, "%s%s-HeightMap.png", OUTPUT_TEXTURE_PATH.c_str(), ExtractFilename(filename).c_str());
	ilSave(IL_PNG, OutputFile);

	ilSetPixels(0, 0, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[0][0][1].data());
	ilSetPixels(REDGUARD_WORLD_EXPORTWIDTH, 0, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[1][0][1].data());
	ilSetPixels(0, REDGUARD_WORLD_EXPORTHEIGHT, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[2][0][1].data());
	ilSetPixels(REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[3][0][1].data());
	iluFlipImage();
	ilConvertImage(IL_RGB, IL_INT);
	snprintf(OutputFile, 900, "%s%s-HeightFlag.png", OUTPUT_TEXTURE_PATH.c_str(), ExtractFilename(filename).c_str());
	ilSave(IL_PNG, OutputFile);

	ilSetPixels(0, 0, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[0][2][0].data());
	ilSetPixels(REDGUARD_WORLD_EXPORTWIDTH, 0, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[1][2][0].data());
	ilSetPixels(0, REDGUARD_WORLD_EXPORTHEIGHT, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[2][2][0].data());
	ilSetPixels(REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[3][2][0].data());
	iluFlipImage();
	ilConvertImage(IL_RGB, IL_INT);
	snprintf(OutputFile, 900, "%s%s-Texture.png", OUTPUT_TEXTURE_PATH.c_str(), ExtractFilename(filename).c_str());
	ilSave(IL_PNG, OutputFile);

	ilSetPixels(0, 0, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[0][2][1].data());
	ilSetPixels(REDGUARD_WORLD_EXPORTWIDTH, 0, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[1][2][1].data());
	ilSetPixels(0, REDGUARD_WORLD_EXPORTHEIGHT, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[2][2][1].data());
	ilSetPixels(REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 0, REDGUARD_WORLD_EXPORTWIDTH, REDGUARD_WORLD_EXPORTHEIGHT, 1, IL_LUMINANCE, IL_UNSIGNED_BYTE, ImageDatas[3][2][1].data());
	iluFlipImage();
	ilConvertImage(IL_RGB, IL_INT);
	snprintf(OutputFile, 900, "%s%s-TextureRotation.png", OUTPUT_TEXTURE_PATH.c_str(), ExtractFilename(filename).c_str());
	ilSave(IL_PNG, OutputFile);

	ilDeleteImage(imageID);

	if (REDGUARD_WORLD_FOOTERSIZE > 0)
	{
		BytesRead = fread(Footer.data(), 1, REDGUARD_WORLD_FOOTERSIZE, pFile);

		if (BytesRead != REDGUARD_WORLD_FOOTERSIZE)
		{
			fclose(pFile);
			PrintLog("Error: Only read %d of %d bytes from world file data footer '%s'!\n", BytesRead, REDGUARD_WORLD_FOOTERSIZE, filename.c_str());
			return false;
		}
	}

	fpos_t endPos = ftell(pFile);
	if (endPos != fileSize)	PrintLog("Warning: Didn't read entire world file data: %d / %d (%d bytes misread)!\n", (int)endPos, (int)fileSize, (int)endPos - (int)fileSize);

	fclose(pFile);

	
	return true;
}



int main()
{
	SetLogLineHeaderOutput(false);
	DuplicateLogToStdOut(true);
	OpenLog("3dfiletest.log");

	InitializeImageLib();
	ilEnable(IL_ORIGIN_SET);
	ilOriginFunc(IL_ORIGIN_LOWER_LEFT);

	TestExportWorldFile(REDGUARD_WORLD_FILE1);
	TestExportWorldFile(REDGUARD_WORLD_FILE2);
	TestExportWorldFile(REDGUARD_WORLD_FILE3);
	TestExportWorldFile(REDGUARD_WORLD_FILE4);
	
	return 0;

	InitializeImageLib();
	InitializeFbxSdkObjects();

	// Create the scene.
	//FbxScene* lScene = FbxScene::Create(g_pSdkManager, "c:\\Temp\\fbx3\\cube.fbx");
	//bool lResult = CreateScene(lScene, "c:\\Temp\\fbx3\\cube.fbx");
	//lResult = ::SaveScene1(g_pSdkManager, lScene, "c:\\Temp\\fbx3\\cube.fbx");

	//PrintLog("Image Header Size = %d", sizeof(redguard_image_header_t));
	//PrintLog("Header Size = %d", sizeof(rg3d_header_t));
	//PrintLog("Header Size = %d", sizeof(Redguard3dFile_Header_t));
    
	Parse3DFile(REDGUARD_FILE_PATH + "MKCRATE.3D");
	Parse3DFile(REDGUARD_FILE_PATH + "MKCRATE1.3D");
	Parse3DFile(REDGUARD_FILE_PATH + "XWANTED.3D");
	//ParseAll3DCFiles(REDGUARD_FILE_PATH);
	//ParseAll3DFiles(REDGUARD_FILE_PATH);
	//ParseAllTextureFiles(REDGUARD_FILE_PATH);

	//ShowFileInfos();
	ShowFileVertexInfos();
	//ShowFileModelInfos();

	DestroyFbxSdkObjects();

	return 0; 
}

