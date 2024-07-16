

#ifdef EXPORT_FOR_DELPHI

#include "ArchiverExt.h"
#include <System.SysUtils.hpp>
#include "ParametersInterface.hpp"

extern "C" __declspec(dllexport) TArchiverInterface* __stdcall CreateArchiver()
{
   return new ArchiverExt();
}

extern "C" __declspec(dllexport) TParametersInterface* __stdcall CreateParames()
{
   return new Parameters();
}


void ArchiverExt::CompressFiles(const System::UnicodeString Files, const System::UnicodeString ArchiveFileName,
									TParametersInterface* paramsp)
{

}

void ArchiverExt::CompressFile(const System::UnicodeString FileName, const System::UnicodeString ArchiveFileName,
								TParametersInterface* paramsp)
{
	FArchiver.CompressFileW(FileName.w_str(), ArchiveFileName.w_str(), *static_cast<Parameters*>(paramsp));
}

void ArchiverExt::ExtractFile(const System::UnicodeString ArchiveFile, const System::UnicodeString FileToExtract,
							  const System::UnicodeString ExtractDir)
{
	std::string tmp1(ArchiveFile.begin(), ArchiveFile.end());
	std::string tmp2(FileToExtract.begin(), FileToExtract.end());
	std::string tmp3(ExtractDir.begin(), ExtractDir.end());
	Parameters* params = new Parameters;
	params->OUTPUT_DIR = tmp3;
	FArchiver.ExtractFile(tmp1, tmp2, tmp3, *params);
}

void ArchiverExt::RemoveFile(const System::UnicodeString ArchiveFile, const System::UnicodeString FileToDelete)
{
	std::string tmp1(ArchiveFile.begin(), ArchiveFile.end());
	std::string tmp2(FileToDelete.begin(), FileToDelete.end());
	FArchiver.RemoveFile(tmp1, tmp2);
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


