unit ParametersInterface;

interface

type

  TCoderType = (NONE, HUFFMAN, AHUFFMAN, RLE, ARITHMETIC, ARITHMETIC32, ARITHMETIC64, AARITHMETIC, AARITHMETIC32, AARITHMETIC64, ABITARITHMETIC );

  TModelType = (UNKNOWN, O0, O1, O2, O3, MIXO3, FO1, BITO1 );

  TParametersInterface = class
  protected
    function GetThreads: uint32; virtual; abstract;
    procedure SetThreads(thrds: uint32); virtual; abstract;
  public
   	property Threads: uint32 read GetThreads write SetThreads; // = 1;
	  //property OutputDir: string read GetOutputDir write SetOutputDir; // = ".\\";
	  //property BlockSize: uint32 read GetBlockSize write SetBlockSize; //= 1 << 16;
	  //property BlockMode: Boolean read GetBlockMode write SetBlockMode; // = true;  // using block mode by default, to back to 'stream' mode use -sm cli option
	  //property Verbose: Boolean read GetVerbose write SetVerbose; // = false;
	  //property ModelType: TModelType read GetModelType write SetModelType; //= ModelType::O2;
	  //property CoderType: TCoderType read GetCoderType write SetCoderType; // = CoderType::AARITHMETIC;
	//static const inline std::string CoderNames[] = { "NONE", "HUF", "AHUF", "RLE", "ARI", "ARI32", "ARI64", "AARI", "AARI32", "AARI64", "BITARI" };
	//static const inline std::string ModelTypeCode[] = { "UNKNOWN", "O0", "O1", "O2", "O3", "MIXO3", "FO1", "BITO1" };
  end;


function CreateParams: TParametersInterface; stdcall; external 'ArithmeticBPL.bpl';


implementation


end.
