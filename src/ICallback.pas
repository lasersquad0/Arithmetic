unit ICallback;

interface

type

  TICallback = class
  public
    procedure start(); virtual; abstract;
    procedure finish(); virtual; abstract;

    {* Return values:
    * 0 - user aborted the process. need to stop/abort current operation
    * 1 - everything is ok, continue operation
    * 2 - pause operation (call method progess() with the last parameter every 1 second until any other value than 2 returned)
    *}
    function progress(prgrs: uint64): Integer; virtual; abstract;

  end;


implementation


end.
