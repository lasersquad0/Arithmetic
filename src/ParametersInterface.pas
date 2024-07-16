unit ParametersInterface;

interface

type

  TCoderType = (NONE, HUFFMAN, AHUFFMAN, RLE, ARITHMETIC, ARITHMETIC32, ARITHMETIC64, AARITHMETIC, AARITHMETIC32, AARITHMETIC64, ABITARITHMETIC );

  TModelType = (UNKNOWN, O0, O1, O2, O3, MIXO3, FO1, BITO1 );

  TParametersInterface = class
  protected
    function GetThreads: uint32; virtual; abstract;
    procedure SetThreads(thrds: uint32); virtual; abstract;
    function GetModelType: TModelType; virtual; abstract;
    procedure SetModelType(mtype: TModelType); virtual; abstract;
    function GetCoderType: TCoderType; virtual; abstract;
    procedure SetCoderType(ctype: TCoderType); virtual; abstract;
    function GetVerbose: Boolean; virtual; abstract;
    procedure SetVerbose(vbrs: Boolean); virtual; abstract;
    function GetBlockMode: Boolean; virtual; abstract;
    procedure SetBlockMode(th: Boolean); virtual; abstract;
    function GetBlockSize: uint32; virtual; abstract;
    procedure SetBlockSize(bsize: uint32); virtual; abstract;
    function GetOutputDir: string; virtual; abstract;
    procedure SetOutputDir(odir: string); virtual; abstract;

  public
   	//property Threads: uint32 read GetThreads write SetThreads; // = 1;
	  property OutputDir: string read GetOutputDir write SetOutputDir; // = ".\\";
	  property BlockSize: uint32 read GetBlockSize write SetBlockSize; //= 1 << 16;
	  property BlockMode: Boolean read GetBlockMode write SetBlockMode; // = true;  // using block mode by default, to back to 'stream' mode use -sm cli option
	  //property Verbose: Boolean read GetVerbose write SetVerbose; // = false;
	  property ModelType: TModelType read GetModelType write SetModelType; //= ModelType::O2;
	  property CoderType: TCoderType read GetCoderType write SetCoderType; // = CoderType::AARITHMETIC;

   	property THREADS: uint32 read GetThreads write SetThreads; // = 1;
    //property OUTPUT_DIR: AnsiString read GetOutputDir write SetOutputDir; // = ".\\";
	  property BLOCK_SIZE: uint32 read GetBlockSize write SetBlockSize; //= 1 << 16;
	  property BLOCK_MODE: Boolean read GetBlockMode write SetBlockMode; // = true;  // using block mode by default, to back to 'stream' mode use -sm cli option
	  property VERBOSE: Boolean read GetVerbose write SetVerbose; // = false;
	  property MODEL_TYPE: TModelType read GetModelType write SetModelType; //= ModelType::O2;
	  property CODER_TYPE: TCoderType read GetCoderType write SetCoderType; // = CoderType::AARITHMETIC;
  end;


implementation


end.
