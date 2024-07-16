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

	procedure CompressFiles(const Files: string; ArchiveFileName: string; params: TParametersInterface); virtual; abstract;
	procedure CompressFile(FileName, ArchiveFileName: string; params: TParametersInterface); virtual; abstract;

	//procedure UncompressFiles(std::ifstream* fin, Parameters params); virtual; abstract;

	//procedure ExtractFiles(ArchiveFile: string, vector_string_t FilesToExtract, ExtractDir: string, Parameters params = Parameters()); virtual; abstract;
	procedure ExtractFile(ArchiveFile, FileToExtract: string; ExtractDir: string{, Parameters params = Parameters()}); virtual; abstract;

	procedure RemoveFile(ArchiveFile, FileToDelete: string); virtual; abstract;
	//procedure RemoveFiles(ArchiveFile: string, vector_string_t& flist); virtual; abstract;

end;


implementation


end.
