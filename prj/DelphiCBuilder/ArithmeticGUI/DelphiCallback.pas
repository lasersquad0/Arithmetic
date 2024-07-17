unit DelphiCallback;

interface

uses
  ICallback;

type

 TDelphiCallback = class(TICallback)
 public
  procedure start(); override;
  procedure finish(); override;

    {* Return values:
    * 0 - user aborted the process. need to stop/abort current operation
    * 1 - everything is ok, continue operation
    * 2 - pause operation (call method progess() with the last parameter every 1 second until any other value than 2 returned)
    *}
   function progress(prgrs: uint64): Integer; override;

 end;

implementation

uses
  SysUtils, MainForm;

{ TDelphiCallback }

const
  CALLBACK_ABORT = 0;
  CALLBACK_OK = 1;
  CALLBACK_PAUSE = 2;

procedure TDelphiCallback.finish;
begin
  inherited;
  Form1.Memo1.Lines.Add('Finish');
end;

function TDelphiCallback.progress(prgrs: uint64): Integer;
begin
  Form1.Memo1.Lines.Add(IntToStr(prgrs));
  Result := CALLBACK_OK;
end;

procedure TDelphiCallback.start;
begin
  inherited;
  Form1.Memo1.Lines.Add('Start');
end;

end.
