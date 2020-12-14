#include "RedguardFbx.h"
#include <fbxsdk.h>



namespace uesp
{
	using std::string;

	
	FbxManager* g_pSdkManager = nullptr;
	

#ifdef IOS_REF
	#undef  IOS_REF
	#define IOS_REF (*(pManager->GetIOSettings()))
#endif


		/* Copied from FBX SDK samples */
	void InitializeFbxSdkObjects()
	{
		g_pSdkManager = FbxManager::Create();

		if (!g_pSdkManager)
		{
			FBXSDK_printf("Error: Unable to create FBX Manager!\n");
			exit(1);
		}
		else
		{
			FBXSDK_printf("Autodesk FBX SDK version %s\n", g_pSdkManager->GetVersion());
		}

		FbxIOSettings* ios = FbxIOSettings::Create(g_pSdkManager, IOSROOT);
		g_pSdkManager->SetIOSettings(ios);

		FbxString lPath = FbxGetApplicationDirectory();
		g_pSdkManager->LoadPluginsDirectory(lPath.Buffer());
	}


		/* Copied from FBX SDK samples */
	void DestroyFbxSdkObjects(bool pExitStatus)
	{
		if (g_pSdkManager) 
		{
			g_pSdkManager->Destroy();
			g_pSdkManager = nullptr;
		}

		if (pExitStatus) FBXSDK_printf("Program Success!\n");
	}


		/* Copied from FBX SDK samples */
	bool SaveScene(FbxManager* pManager, FbxDocument* pScene, const char* pFilename, int pFileFormat = -1, bool pEmbedMedia = false)
	{
		int lMajor, lMinor, lRevision;
		bool lStatus = true;

		FbxExporter* lExporter = FbxExporter::Create(pManager, "");

		if (pFileFormat < 0 || pFileFormat >= pManager->GetIOPluginRegistry()->GetWriterFormatCount())
		{
			pFileFormat = pManager->GetIOPluginRegistry()->GetNativeWriterFormat();
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
		IOS_REF.SetBoolProp(EXP_FBX_MATERIAL, true);
		IOS_REF.SetBoolProp(EXP_FBX_TEXTURE, true);
		IOS_REF.SetBoolProp(EXP_FBX_EMBEDDED, pEmbedMedia);
		IOS_REF.SetBoolProp(EXP_FBX_SHAPE, true);
		IOS_REF.SetBoolProp(EXP_FBX_GOBO, true);
		IOS_REF.SetBoolProp(EXP_FBX_ANIMATION, true);
		IOS_REF.SetBoolProp(EXP_FBX_GLOBAL_SETTINGS, true);

		if (lExporter->Initialize(pFilename, pFileFormat, pManager->GetIOSettings()) == false)
		{
			FBXSDK_printf("Call to FbxExporter::Initialize() failed.\n");
			FBXSDK_printf("Error returned: %s\n\n", lExporter->GetStatus().GetErrorString());
			return false;
		}

		FbxManager::GetFileFormatVersion(lMajor, lMinor, lRevision);
		//FBXSDK_printf("FBX file format version %d.%d.%d\n\n", lMajor, lMinor, lRevision);

		lStatus = lExporter->Export(pScene);

		lExporter->Destroy();
		return lStatus;
	}


	FbxNode* Convert3dFileToFbxNode(CRedguard3dFile& File, FbxScene* pScene, const string Name)
	{
		FbxMesh* lMesh = FbxMesh::Create(pScene, Name.c_str());
		std::vector< FbxVector4 > ControlPoints;
		std::vector< FbxVector4 > Normals;

		for (auto& it : File.m_VertexCoordinates)
		{
			ControlPoints.push_back(FbxVector4(it.x / File.COOR_TRANSFORM_FACTOR, it.y / File.COOR_TRANSFORM_FACTOR, it.z / File.COOR_TRANSFORM_FACTOR));
		}

		lMesh->InitControlPoints(File.m_TotalFaceVertexes);
		FbxVector4* lControlPoints = lMesh->GetControlPoints();

		int j = 0;

		for (auto& face : File.m_FaceData)
		{
			for (auto& v : face.VertexData)
			{
				lControlPoints[j] = ControlPoints[v.VertexIndex];
				++j;
			}
		}

		FbxGeometryElementNormal* lNormalElement = lMesh->CreateElementNormal();
		lNormalElement->SetMappingMode(FbxGeometryElement::eByPolygon);
		lNormalElement->SetReferenceMode(FbxGeometryElement::eDirect);

		for (auto& it : File.m_FaceNormals)
		{
			Normals.push_back(FbxVector4(it.x / File.NORMAL_TRANSFORM_FACTOR, it.y / File.NORMAL_TRANSFORM_FACTOR, it.z / File.NORMAL_TRANSFORM_FACTOR));
			lNormalElement->GetDirectArray().Add(Normals[Normals.size() - 1]);
		}

		//FbxGeometryElementMaterial* lMaterialElement = lMesh->CreateElementMaterial();
		//lMaterialElement->SetMappingMode(FbxGeometryElement::eByPolygon);
		//lMaterialElement->SetReferenceMode(FbxGeometryElement::eIndexToDirect);

		j = 0;
		int i = 0;

		for (auto& face : File.m_FaceData)
		{
			lMesh->BeginPolygon(i);

			for (auto& v : face.VertexData)
			{
				lMesh->AddPolygon(j);
				++j;
			}

			lMesh->EndPolygon();
			++i;
		}

		FbxNode* lNode = FbxNode::Create(pScene, Name.c_str());
		lNode->SetNodeAttribute(lMesh);
		//CreateMaterials(pScene, lMesh);

		return lNode;
	}


	bool Save3DFileAsFbx(CRedguard3dFile& File, const string Filename)
	{
		FbxScene* pScene = FbxScene::Create(g_pSdkManager, File.m_Name.c_str());

		if (!pScene)
		{
			FBXSDK_printf("Error: Unable to create FBX scene!\n");
			return (false);
		}

		FbxDocumentInfo* sceneInfo = FbxDocumentInfo::Create(g_pSdkManager, "SceneInfo");
		sceneInfo->mTitle = File.m_Name.c_str();
		sceneInfo->mSubject = "Converted Redguard 3D file";
		sceneInfo->mAuthor = "Redguard3dFile";
		sceneInfo->mRevision = "rev. 0.1";
		sceneInfo->mKeywords = "";
		sceneInfo->mComment = "";
		pScene->SetSceneInfo(sceneInfo);

		FbxNode* lRootNode = pScene->GetRootNode();
		FbxNode* pNode = Convert3dFileToFbxNode(File, pScene, File.m_Name);
		lRootNode->AddChild(pNode);

		bool lResult = SaveScene(g_pSdkManager, pScene, Filename.c_str());

		if (lResult == false)
		{
			FBXSDK_printf("\n\nAn error occurred while saving the scene...\n");
			return false;
		}

		pScene->Destroy();

		return true;
	}


	

};