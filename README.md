Dave Humphrey -- dave@uesp.net -- Decemeber 2020

This is a test project used for deciphering the various Redguard 3D and image file formats. It is not 
intended to be a well designed API library or standalone command line program. Ideally the source code
in the "Common" subdirectory can be copied and modified to fit your desired usage.

For basic usage modify the input and output paths at the top of 3DFileTest.cpp, compile and run.

Basic Features:
	- Export 3D/3DC files to FBX files.
	- Export TEXBSI.* files to PNG files.
	- Output information on the 3D/3DC files to console.

Developed On:
    - Windows 10
	- Visual Studio 2017

Libraries Used:
   - Devil (included)
   - Autodesk FBX SDK 2020-1 (https://www.autodesk.com/developer-network/platform-technologies/fbx-sdk-2020-1)
             Will need to be installed and project include paths and LIB files updated to the correct location.