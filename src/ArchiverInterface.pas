unit ArchiverInterface;

interface

uses
  ParametersInterface, ICallback;

type

  TArchiverInterface = class
public
	procedure InitLogger(const LogCfgFile: string); virtual; abstract;
	procedure AddCallback(cb: TICallback); virtual; abstract;
	procedure RemoveCallback(cb: TICallback); virtual; abstract;

  // Files - list of file names separated by colon ':'
	procedure CompressFiles(ArchiveFileName: string; const Files: string; params: TParametersInterface); virtual; abstract;
	procedure CompressFile(ArchiveFileName, FileName: string; params: TParametersInterface); virtual; abstract;

	//procedure UncompressFiles(std::ifstream* fin, Parameters params); virtual; abstract;

  // Files - list of file names separated by colon ':'
	procedure ExtractFiles(ArchiveFileName, FilesToExtract, ExtractDir: string{, Parameters params = Parameters()}); virtual; abstract;
	procedure ExtractFile(ArchiveFileName, FileToExtract: string; ExtractDir: string{, Parameters params = Parameters()}); virtual; abstract;

	procedure RemoveFile(ArchiveFileName, FileToRemove: string); virtual; abstract;
	procedure RemoveFiles(ArchiveFileName, FilesToRemove: string); virtual; abstract; // list of file names separated by colon ':'

  procedure UncompressFiles(ArchiveFileName: string; params: TParametersInterface); virtual; abstract;
end;


implementation


end.
