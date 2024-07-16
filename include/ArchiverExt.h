#pragma once

#ifdef EXPORT_FOR_DELPHI

#include "Archiver.h"
#include "DynamicArrays.h"
#include "ICallback.hpp"
#include "ArchiverInterface.hpp"


class __declspec(delphiclass) ArchiverExt : public TArchiverInterface
{
private:
	Archiver FArchiver;
	THash<TICallback*, ICallback*> FCallbacks;

public:
	void __fastcall InitLogger(const System::UnicodeString LogCfgFile) override { std::string str(LogCfgFile.begin(), LogCfgFile.end()); LogEngine::InitFromFile(str); }

	void __fastcall AddCallback(TICallback* cb) override;
	void __fastcall RemoveCallback(TICallback* cb) override;

	void __fastcall CompressFiles(const System::UnicodeString Files, const System::UnicodeString ArchiveFileName, TParametersInterface* paramsp) override;
	void __fastcall CompressFile(const System::UnicodeString FileName, const System::UnicodeString ArchiveFileName, TParametersInterface* paramsp) override;
	void __fastcall ExtractFile(const System::UnicodeString ArchiveFile, const System::UnicodeString FileToExtract, const System::UnicodeString ExtractDir/*, Parameters params = Parameters()*/) override;
	void __fastcall RemoveFile(const System::UnicodeString ArchiveFile, const System::UnicodeString FileToDelete) override;
};

#endif


