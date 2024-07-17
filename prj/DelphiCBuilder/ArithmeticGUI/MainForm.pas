unit MainForm;

interface

uses
  Winapi.Windows, Winapi.Messages, System.SysUtils, System.Variants, System.Classes, Vcl.Graphics,
  Vcl.Controls, Vcl.Forms, Vcl.Dialogs, Vcl.StdCtrls, Vcl.Buttons, Vcl.Mask,
  Vcl.ExtCtrls,
  ArchiverInterface, ParametersInterface, DelphiCallback;


type
  TForm1 = class(TForm)
    CompressButton: TButton;
    Memo1: TMemo;
    ArchiveEdit: TEdit;
    LabeledEdit1: TLabeledEdit;
    LabeledEdit2: TLabeledEdit;
    LabeledEdit3: TLabeledEdit;
    ArchiveEdit2: TEdit;
    LabeledEdit4: TLabeledEdit;
    LabeledEdit5: TLabeledEdit;
    LabeledEdit6: TLabeledEdit;
    ExtractButton: TButton;
    SpeedButton1: TSpeedButton;
    SpeedButton2: TSpeedButton;
    SpeedButton3: TSpeedButton;
    OpenDialog1: TOpenDialog;
    CompressFilesButton: TButton;
    ExtractALLButton: TButton;
    procedure CompressButtonClick(Sender: TObject);
    procedure SpeedButton1Click(Sender: TObject);
    procedure SpeedButton2Click(Sender: TObject);
    procedure SpeedButton3Click(Sender: TObject);
    procedure FormCreate(Sender: TObject);
    procedure CompressFilesButtonClick(Sender: TObject);
    procedure ExtractALLButtonClick(Sender: TObject);
    procedure ExtractButtonClick(Sender: TObject);
  private
    { Private declarations }
    Archiver: TArchiverInterface;
    Params: TParametersInterface;
    Callback: TDelphiCallback;
  public
    { Public declarations }
  end;

var
  Form1: TForm1;

implementation

{$R *.dfm}

function CreateParames: TParametersInterface; stdcall; external 'ArithmeticBPL.bpl';
function CreateArchiver: TArchiverInterface; stdcall; external 'ArithmeticBPL.bpl';


procedure TForm1.ExtractALLButtonClick(Sender: TObject);
begin
  try
    Archiver.UncompressFiles(ArchiveEdit2.Text, Params);
  except
    on e:Exception do MessageDlg(e.Message, TMsgDlgType.mtError, [mbOK], 0);
  else
    MessageDlg('UNKNOWN EXCEPTION', TMsgDlgType.mtError, [mbOK], 0);
  end;
end;

procedure TForm1.ExtractButtonClick(Sender: TObject);
var
  files: string;
begin
  files := LabeledEdit4.Text;
  if LabeledEdit5.Text <> '' then files := files +  '|' + LabeledEdit5.Text;
  if LabeledEdit6.Text <> '' then files := files +  '|' + LabeledEdit6.Text;
  try
    Archiver.ExtractFiles(ArchiveEdit2.Text, files, './');
  except
    on e:Exception do MessageDlg(e.Message, TMsgDlgType.mtError, [mbOK], 0);
  else
    MessageDlg('UNKNOWN EXCEPTION', TMsgDlgType.mtError, [mbOK], 0);
  end;
end;

procedure TForm1.CompressButtonClick(Sender: TObject);
var
  mt: TModelType;
begin
  try
    Archiver.CompressFile(ArchiveEdit.Text, LabeledEdit1.Text, Params);
  except
    on e:Exception do MessageDlg(e.Message, TMsgDlgType.mtError, [mbOK], 0);
  else
    MessageDlg('UNKNOWN EXCEPTION', TMsgDlgType.mtError, [mbOK], 0);
  end;
end;

procedure TForm1.CompressFilesButtonClick(Sender: TObject);
var
  files: string;
begin
  try
    files := LabeledEdit1.Text;
    if LabeledEdit2.Text <> '' then files := files +  '|' + LabeledEdit2.Text;
    if LabeledEdit3.Text <> '' then files := files +  '|' + LabeledEdit3.Text;
    Archiver.CompressFiles(ArchiveEdit.Text, files, Params);
  except
    on e:Exception do MessageDlg(e.Message, TMsgDlgType.mtError, [mbOK], 0);
  else
    MessageDlg('UNKNOWN EXCEPTION', TMsgDlgType.mtError, [mbOK], 0);
  end;
end;

procedure TForm1.FormCreate(Sender: TObject);
begin
  Params := CreateParames;
  Archiver := CreateArchiver;
  Callback := TDelphiCallback.Create;
  Archiver.InitLogger('ArithmeticLog.lfg');
  Archiver.AddCallback(Callback);
end;

procedure TForm1.SpeedButton1Click(Sender: TObject);
begin
  if OpenDialog1.Execute then LabeledEdit1.Text := OpenDialog1.FileName;
end;

procedure TForm1.SpeedButton2Click(Sender: TObject);
begin
if OpenDialog1.Execute then LabeledEdit2.Text := OpenDialog1.FileName;
end;

procedure TForm1.SpeedButton3Click(Sender: TObject);
begin
if OpenDialog1.Execute then LabeledEdit3.Text := OpenDialog1.FileName;
end;

end.
