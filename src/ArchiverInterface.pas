unit ArchiverInterface;

interface

type

  TArchiverInterface = class
  {private

	void SaveBlock(std::ostringstream& fblock, std::ofstream* fout, uint32_t uBlockSize, int32_t lineNum);
	void LoadBlock(std::istringstream& fb, std::ifstream* fin, uint32_t& uBlockSize, uint32_t& cBlockSize, int32_t& lineNum);

	/** Return values for next 4 methods:
	* 0 - user aborted the process. need to stop/abort current operation
	* 1 and any value except 0 - everything was ok, move on to the next file
	*/
	int CompressFile(std::ofstream* fout, std::ifstream* fin, FileRecord& fr, IModel* model);
	int CompressFileBlock(std::ofstream* fout, std::ifstream* fin, FileRecord& fr, IModel* model);
	int UncompressFile(std::ifstream* fin, std::ofstream* fout, FileRecord& fr, IModel* model);
	int UncompressFileBlock(std::ifstream* fin, std::ofstream* fout, FileRecord& fr, IModel* model);

	void PrintCompressionStart(Parameters params);
	void PrintUncompressionStart(FileRecord& fr, Parameters params);
	void PrintFileCompressionDone(FileRecord& fr);

	bool BypassFile(std::ifstream* fin, FileRecord& fr);
	bool CopyFileData(std::ifstream* fin, std::ofstream* fout, FileRecord& fr);
 }
public
	//void AddCallback(ICallback* cb);
	//void RemoveCallback(ICallback* cb);

	procedure CompressFiles(const Files: string; ArchiveFileName: string{; params: TParametersInterface}); virtual; abstract;
	procedure CompressFile(FileName, ArchiveFileName: string{, Parameters params}); virtual; abstract;

	//procedure UncompressFiles(std::ifstream* fin, Parameters params); virtual; abstract;

	//procedure ExtractFiles(ArchiveFile: string, vector_string_t FilesToExtract, ExtractDir: string, Parameters params = Parameters()); virtual; abstract;
	procedure ExtractFile(ArchiveFile, FileToExtract: string; ExtractDir: string{, Parameters params = Parameters()}); virtual; abstract;

	procedure RemoveFile(ArchiveFile, FileToDelete: string); virtual; abstract;
	//procedure RemoveFiles(ArchiveFile: string, vector_string_t& flist); virtual; abstract;

end;


implementation


end.
