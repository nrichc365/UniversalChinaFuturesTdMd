#ifndef _DATAINTOFILES_H_
#define _DATAINTOFILES_H_
#pragma once

#ifdef DataIntoFiles_EXE
#define DataIntoFiles_EXPORT
#else
#ifdef DataIntoFiles_EXP
#define DataIntoFiles_EXPORT __declspec(dllexport)
#else
#define DataIntoFiles_EXPORT __declspec(dllimport)
#endif DataIntoFiles_EXP
#endif DataIntoFiles_EXE

#ifndef MaxDataFileSize
#define MaxDataFileSize 5
#endif MaxDataFileSize

#ifndef MaxFilenameCharacterCount
#define MaxFilenameCharacterCount 50
#endif

#include <stdio.h>

namespace axapi{
	struct DataFile
	{
		FILE* pDataFile;
		char strDataFilename[MaxFilenameCharacterCount];
	};

	class DataIntoFiles
	{
	private:
		DataFile m_objDataFile;
		int m_nCurrentDataFileSize;
		int m_nMaxDataFileSize;
		char m_strDataFilenamePrefix[MaxFilenameCharacterCount];
		char m_strListFilename[MaxFilenameCharacterCount];
	public:
		DataIntoFiles();
		DataIntoFiles(char* in_strFilenamePrefix, int in_nMaxDataFileSize, char* in_strListFilename);
		~DataIntoFiles();
		int writeData2File(char* in_strData);
	private:
		void changeDataFile();
		void writeFile2List();
		void openDataFile();
		void closeDataFile();
	};
}
#endif _DATAINTOFILES_H_