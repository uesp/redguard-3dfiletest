#pragma once


#include "Redguard3dFile.h"
#include <fbxsdk.h>


namespace uesp
{

	void InitializeFbxSdkObjects();
	void DestroyFbxSdkObjects(bool pExitStatus = false);

	bool Save3DFileAsFbx(CRedguard3dFile& File, const std::string Filename);

	extern FbxManager* g_pSdkManager;

};
