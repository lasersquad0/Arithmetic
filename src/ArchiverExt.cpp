

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
	std::string cfg = convert_string<std::string::value_type>(LogCfgFile.w_str());
	LogEngine::InitFromFile(cfg);
}

void ArchiverExt::CompressFiles(const System::UnicodeString ArchiveFileName, const System::UnicodeString FilesToCompress,
									TParametersInterface* paramsp)
{
	Parameters params(paramsp);

	string_t files = FilesToCompress.w_str();
	vect_string_t arr;
	StringToArray(files, arr, '|');

	FArchiver.CompressFiles(ArchiveFileName.w_str(), arr, params);
}

void ArchiverExt::CompressFile(const System::UnicodeString ArchiveFileName, const System::UnicodeString FileName,
								TParametersInterface* paramsp)
{
	Parameters params(paramsp);
	vect_string_t files;
   //	string_t tmpFileName = FileName.w_str();
	files.push_back(FileName.w_str());
	FArchiver.CompressFiles(ArchiveFileName.w_str(), files, params);
}

void ArchiverExt::ExtractFile(const System::UnicodeString ArchiveFileName, const System::UnicodeString FileToExtract,
							  const System::UnicodeString ExtractDir)
{
	//std::string tmp1(ArchiveFileName.begin(), ArchiveFileName.end());
	//std::string tmp2(FileToExtract.begin(), FileToExtract.end());
	//std::string tmp3(ExtractDir.begin(), ExtractDir.end());
	Parameters params;
	params.OUTPUT_DIR = ExtractDir.w_str();
	FArchiver.ExtractFile(ArchiveFileName.w_str(), FileToExtract.w_str(), params);
}

void ArchiverExt::ExtractFiles(const System::UnicodeString ArchiveFileName, const System::UnicodeString FilesToExtract,
							  const System::UnicodeString ExtractDir)
{
	//std::string aFileName(ArchiveFileName.begin(), ArchiveFileName.end());
	//std::string oDir(ExtractDir.begin(), ExtractDir.end());

	Parameters params;
	params.OUTPUT_DIR = ExtractDir.w_str();

	vect_string_t arr;
	//std::string files(FilesToExtract.begin(), FilesToExtract.end());
	StringToArray<string_t>(FilesToExtract.w_str(), arr, L'|');

	FArchiver.ExtractFiles(ArchiveFileName.w_str(), arr, params);
}

void ArchiverExt::UncompressFiles(const System::UnicodeString ArchiveFileName, TParametersInterface* paramsp)
{
   //	std::string aFileName(ArchiveFileName.begin(), ArchiveFileName.end());
	Parameters params(paramsp);
	FArchiver.UncompressFiles(ArchiveFileName.w_str(), params);
}

void ArchiverExt::RemoveFile(const System::UnicodeString ArchiveFileName, const System::UnicodeString FileToRemove)
{
	//std::string tmp1(ArchiveFileName.begin(), ArchiveFileName.end());
	//std::string tmp2(FileToRemove.begin(), FileToRemove.end());
	FArchiver.RemoveFile(ArchiveFileName.w_str(), FileToRemove.w_str());
}

void ArchiverExt::RemoveFiles(const System::UnicodeString ArchiveFileName, const System::UnicodeString FilesToRemove)
{

	//std::string aFileName(ArchiveFileName.begin(), ArchiveFileName.end());
	//std::string rFiles(FilesToRemove.begin(), FilesToRemove.end());
	vect_string_t arr;
	StringToArray<string_t>(FilesToRemove.w_str(), arr, '|');
	FArchiver.RemoveFiles(ArchiveFileName.w_str(), arr);
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


