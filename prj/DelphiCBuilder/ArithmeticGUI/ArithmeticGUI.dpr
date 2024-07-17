program ArithmeticGUI;

uses
  Vcl.Forms,
  MainForm in 'MainForm.pas' {Form1},
  ArchiverInterface in '..\..\..\src\ArchiverInterface.pas',
  ParametersInterface in '..\..\..\src\ParametersInterface.pas',
  ICallback in '..\..\..\src\ICallback.pas',
  DelphiCallback in 'DelphiCallback.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.MainFormOnTaskbar := True;
  Application.CreateForm(TForm1, Form1);
  Application.Run;
end.
