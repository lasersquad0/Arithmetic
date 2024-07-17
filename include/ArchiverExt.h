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
	void __fastcall InitLogger(const System::UnicodeString LogCfgFile) override;

	void __fastcall AddCallback(TICallback* cb) override;
	void __fastcall RemoveCallback(TICallback* cb) override;

	void __fastcall CompressFiles(const System::UnicodeString ArchiveFileName, const System::UnicodeString FilesToCompress, TParametersInterface* paramsp) override;
	void __fastcall CompressFile(const System::UnicodeString ArchiveFileName, const System::UnicodeString FileName, TParametersInterface* paramsp) override;
	void __fastcall ExtractFile(const System::UnicodeString ArchiveFileName, const System::UnicodeString FileToExtract, const System::UnicodeString ExtractDir) override;
	void __fastcall ExtractFiles(const System::UnicodeString ArchiveFileName, const System::UnicodeString FilesToExtract, const System::UnicodeString ExtractDir) override;
	void __fastcall RemoveFile(const System::UnicodeString ArchiveFileName, const System::UnicodeString FileToRemove) override;
	void __fastcall RemoveFiles(const System::UnicodeString ArchiveFileName, const System::UnicodeString FileToRemove) override;

	void __fastcall UncompressFiles(const System::UnicodeString ArchiveFileName, TParametersInterface* paramsp) override;
};

#endif


