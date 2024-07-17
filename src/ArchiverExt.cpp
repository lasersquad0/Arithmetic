

#ifdef EXPORT_FOR_DELPHI

#include "ArchiverExt.h"
#include "CommonFunctions.h"
#include <System.SysUtils.hpp>
#include "ParametersInterface.hpp"

extern "C" __declspec(dllexport) TArchiverInterface* __stdcall CreateArchiver()
{
   return new ArchiverExt();
}

extern "C" __declspec(dllexport) TParametersInterface* __stdcall CreateParames()
{
   return new DelphiParameters();
}

void ArchiverExt::InitLogger(const System::UnicodeString LogCfgFile)
{
	std::string cfg(LogCfgFile.begin(), LogCfgFile.end());
	LogEngine::InitFromFile(cfg);
}

void ArchiverExt::CompressFiles(const System::UnicodeString ArchiveFileName, const System::UnicodeString FilesToCompress,
									TParametersInterface* paramsp)
{
	std::string aFileName(ArchiveFileName.begin(), ArchiveFileName.end());

	Parameters params(paramsp);

	std::string files(FilesToCompress.begin(), FilesToCompress.end());
	vector_string_t arr;
	StringToArray(files, arr, '|');

	FArchiver.CompressFiles(aFileName, arr, params);
}

void ArchiverExt::CompressFile(const System::UnicodeString ArchiveFileName, const System::UnicodeString FileName,
								TParametersInterface* paramsp)
{
	Parameters params(paramsp);
	vector_string_t files;
	std::string tmpFileName(FileName.begin(), FileName.end());
	files.push_back(tmpFileName);
	std::string tmpAFileName(ArchiveFileName.begin(), ArchiveFileName.end());
	FArchiver.CompressFiles(tmpAFileName, files, params);
}

void ArchiverExt::ExtractFile(const System::UnicodeString ArchiveFileName, const System::UnicodeString FileToExtract,
							  const System::UnicodeString ExtractDir)
{
	std::string tmp1(ArchiveFileName.begin(), ArchiveFileName.end());
	std::string tmp2(FileToExtract.begin(), FileToExtract.end());
	std::string tmp3(ExtractDir.begin(), ExtractDir.end());
	Parameters params;
	params.OUTPUT_DIR = tmp3;
	FArchiver.ExtractFile(tmp1, tmp2, params);
}

void ArchiverExt::ExtractFiles(const System::UnicodeString ArchiveFileName, const System::UnicodeString FilesToExtract,
							  const System::UnicodeString ExtractDir)
{
	std::string aFileName(ArchiveFileName.begin(), ArchiveFileName.end());
	std::string oDir(ExtractDir.begin(), ExtractDir.end());

	Parameters params;
	params.OUTPUT_DIR = oDir;

	vector_string_t arr;
	std::string files(FilesToExtract.begin(), FilesToExtract.end());
	StringToArray(files, arr, '|');

	FArchiver.ExtractFiles(aFileName, arr, params);
}

void ArchiverExt::UncompressFiles(const System::UnicodeString ArchiveFileName, TParametersInterface* paramsp)
{
	std::string aFileName(ArchiveFileName.begin(), ArchiveFileName.end());
	Parameters params(paramsp);
	FArchiver.UncompressFiles(aFileName, params);
}

void ArchiverExt::RemoveFile(const System::UnicodeString ArchiveFileName, const System::UnicodeString FileToRemove)
{
	std::string tmp1(ArchiveFileName.begin(), ArchiveFileName.end());
	std::string tmp2(FileToRemove.begin(), FileToRemove.end());
	FArchiver.RemoveFile(tmp1, tmp2);
}

void ArchiverExt::RemoveFiles(const System::UnicodeString ArchiveFileName, const System::UnicodeString FilesToRemove)
{

	std::string aFileName(ArchiveFileName.begin(), ArchiveFileName.end());
	std::string rFiles(FilesToRemove.begin(), FilesToRemove.end());
	vector_string_t arr;
	StringToArray(rFiles, arr, '|');
	FArchiver.RemoveFiles(aFileName, arr);
}

class DelphiCallback: public ICallback
{
private:
	TICallback* FCB;
public:
	DelphiCallback(TICallback* cb) { FCB = cb; }
	void start() override { FCB->start(); }
	void finish() override { FCB->finish(); }
	int progress(uint64_t prgrs) override {	return FCB->progress(prgrs); }
};


void ArchiverExt::AddCallback(TICallback* cb)
{
	if (!FCallbacks.IfExists(cb))
	{
		auto newCB = new DelphiCallback(cb);
		FCallbacks.SetValue(cb, newCB);
		FArchiver.AddCallback(newCB);
	}
}

void ArchiverExt::RemoveCallback(TICallback* cb)
{
	if (FCallbacks.IfExists(cb))
	{
		auto oldCB = FCallbacks[cb];
		FArchiver.RemoveCallback(oldCB);
		FCallbacks.Delete(cb);
		delete oldCB;
	}
}


#endif


