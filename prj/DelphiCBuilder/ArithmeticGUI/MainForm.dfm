object Form1: TForm1
  Left = 0
  Top = 0
  Caption = 'Form1'
  ClientHeight = 442
  ClientWidth = 852
  Color = clBtnFace
  Font.Charset = DEFAULT_CHARSET
  Font.Color = clWindowText
  Font.Height = -12
  Font.Name = 'Segoe UI'
  Font.Style = []
  OnCreate = FormCreate
  TextHeight = 15
  object SpeedButton1: TSpeedButton
    Left = 134
    Top = 63
    Width = 21
    Height = 18
    Caption = '...'
    OnClick = SpeedButton1Click
  end
  object SpeedButton2: TSpeedButton
    Left = 134
    Top = 113
    Width = 21
    Height = 18
    Caption = '...'
    OnClick = SpeedButton2Click
  end
  object SpeedButton3: TSpeedButton
    Left = 133
    Top = 160
    Width = 21
    Height = 18
    Caption = '...'
    OnClick = SpeedButton3Click
  end
  object CompressButton: TButton
    Left = 170
    Top = 60
    Width = 96
    Height = 25
    Caption = 'Compress File'
    TabOrder = 0
    OnClick = CompressButtonClick
  end
  object Memo1: TMemo
    Left = 0
    Top = 240
    Width = 852
    Height = 202
    Align = alBottom
    TabOrder = 1
  end
  object ArchiveEdit: TEdit
    Left = 8
    Top = 8
    Width = 121
    Height = 23
    TabOrder = 2
    Text = 'archive_file.ar'
  end
  object LabeledEdit1: TLabeledEdit
    Left = 8
    Top = 61
    Width = 121
    Height = 23
    EditLabel.Width = 24
    EditLabel.Height = 15
    EditLabel.Caption = 'File1'
    TabOrder = 3
    Text = ''
  end
  object LabeledEdit2: TLabeledEdit
    Left = 8
    Top = 109
    Width = 121
    Height = 23
    EditLabel.Width = 24
    EditLabel.Height = 15
    EditLabel.Caption = 'File2'
    TabOrder = 4
    Text = ''
  end
  object LabeledEdit3: TLabeledEdit
    Left = 8
    Top = 157
    Width = 121
    Height = 23
    EditLabel.Width = 24
    EditLabel.Height = 15
    EditLabel.Caption = 'File3'
    TabOrder = 5
    Text = ''
  end
  object ArchiveEdit2: TEdit
    Left = 294
    Top = 8
    Width = 121
    Height = 23
    TabOrder = 6
    Text = 'archive_file.ar'
  end
  object LabeledEdit4: TLabeledEdit
    Left = 294
    Top = 61
    Width = 121
    Height = 23
    EditLabel.Width = 24
    EditLabel.Height = 15
    EditLabel.Caption = 'File1'
    TabOrder = 7
    Text = ''
  end
  object LabeledEdit5: TLabeledEdit
    Left = 294
    Top = 109
    Width = 121
    Height = 23
    EditLabel.Width = 24
    EditLabel.Height = 15
    EditLabel.Caption = 'File2'
    TabOrder = 8
    Text = ''
  end
  object LabeledEdit6: TLabeledEdit
    Left = 294
    Top = 157
    Width = 121
    Height = 23
    EditLabel.Width = 24
    EditLabel.Height = 15
    EditLabel.Caption = 'File3'
    TabOrder = 9
    Text = ''
  end
  object ExtractButton: TButton
    Left = 454
    Top = 60
    Width = 75
    Height = 25
    Caption = 'Extract'
    TabOrder = 10
    OnClick = ExtractButtonClick
  end
  object CompressFilesButton: TButton
    Left = 170
    Top = 108
    Width = 96
    Height = 25
    Caption = 'Compress FileS'
    TabOrder = 11
    OnClick = CompressFilesButtonClick
  end
  object ExtractALLButton: TButton
    Left = 454
    Top = 108
    Width = 75
    Height = 25
    Caption = 'Extract ALL'
    TabOrder = 12
    OnClick = ExtractALLButtonClick
  end
  object OpenDialog1: TOpenDialog
    Options = [ofReadOnly, ofPathMustExist, ofFileMustExist, ofEnableSizing]
    Left = 632
    Top = 352
  end
end
