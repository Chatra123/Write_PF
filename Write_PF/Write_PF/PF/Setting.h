#pragma once
#include "../stdafx.h"



class IniFile
{
private:
  wstring Path;

  ///=============================
  /// constructor
  ///=============================
public:
  IniFile(wstring path)
  {
    Path = path;
  }

  ///=============================
  /// IsExist
  ///=============================
public:
  bool IsExist()
  {
    bool exist = _waccess_s(Path.c_str(), 0) == 0;
    return exist;
  }

  ///=============================
  /// ini --> string
  ///=============================
public:
  wstring GetString(wstring section, wstring key, wstring defalut)
  {
    wchar_t value[_MAX_PATH] = {};
    GetPrivateProfileString(section.c_str(), key.c_str(), defalut.c_str(), value, _MAX_PATH, Path.c_str());
    return wstring(value);
  }

  ///=============================
  /// ini --> int
  ///=============================
public:
  int GetInt(wstring section, wstring key, int defalut)
  {
    wstring value = GetString(section, key, L"none");
    try { return std::stoi(value); }
    catch (exception) { return defalut; }
  }

  ///=============================
  /// ini --> double
  ///=============================
public:
  double GetDouble(wstring section, wstring key, double defalut)
  {
    wstring value = GetString(section, key, L"none");
    try { return std::stod(value); }
    catch (exception) { return defalut; }
  }

  ///=============================
  /// ini --> bool
  ///=============================
public:
  bool GetBool(wstring section, wstring key, bool defalut)
  {
    wstring value = GetString(section, key, L"none");
    try { return std::stoi(value) != 0; }
    catch (exception) { return defalut; }
  }

  ///=============================
  /// string --> ini
  ///=============================
public:
  bool WriteString(wstring section, wstring key, wstring value)
  {
    BOOL ret = WritePrivateProfileString(section.c_str(), key.c_str(), value.c_str(), Path.c_str());
    return ret != 0;
  }

};




///=============================
///  設定
///=============================
class SettingA
{
public:
  class Setting_Client
  {
  public:
    int No;//logファイル名の連番用
    bool Enable;
    wstring Path;
    wstring Args;
    bool Hide;
  };


private:
  unique_ptr<IniFile> iniFile;

public:
  wstring TsPath;
  int Write_Buff;
  bool Mode_NamedPipe;
  double Pipe_Buff_MiB;
  Setting_Client Client[2];

public:
  SettingA(wstring dllpath)
  {
    iniFile = make_unique<IniFile>(dllpath + L".ini");
    if (iniFile->IsExist() == false)
      Write();
    Read();
  }

  ///=============================
  /// Read file
  ///=============================
private:
  void Read()
  {
    Write_Buff = iniFile->GetInt(L"Set", L"Size", 770048);
    Write_Buff = iniFile->GetBool(L"Pipe", L"NamedPipe", true);
    Pipe_Buff_MiB = iniFile->GetDouble(L"Pipe", L"Buff_MiB", 3);
    wstring section;
    section = L"Client_0";
    Client[0].No = 0;
    Client[0].Enable = iniFile->GetBool(section, L"Enable", false);
    Client[0].Path = iniFile->GetString(section, L"Path", L"");
    Client[0].Args = iniFile->GetString(section, L"Args", L"");
    Client[0].Hide = iniFile->GetBool(section, L"Hide", false);
    section = L"Client_1";
    Client[1].No = 1;
    Client[1].Enable = iniFile->GetBool(section, L"Enable", false);
    Client[1].Path = iniFile->GetString(section, L"Path", L"");
    Client[1].Args = iniFile->GetString(section, L"Args", L"");
    Client[1].Hide = iniFile->GetBool(section, L"Hide", false);
  }


  ///=============================
  /// Create file
  ///=============================
private:
  void Write()
  {
    //const wstring pfA_Path = L".\\Write\\Write_PF\\pfAdapter\\pfAdapter.exe";
    //const wstring pfA_Args1 = L"-npipe $PipeName$  -file \"$FilePath$\"";
    //const wstring pfA_Args2 = L"-npipe $PipeName$  -file \"$FilePath$\"  -xml pfAdapter2.xml ";
    //const wstring p2f_Path = L".\\Write\\Write_PF\\Pipe2File.exe";
    //const wstring p2f_Args = L"\"$FilePath$.p2f.ts\" a";//  a が無いとFilePathの””が自動ではずされてしまう

    const wstring pfA_Path = L".\\Write\\Write_PF\\pfAdapter\\pfAdapter.exe";
    const wstring pfA_Args = L"-npipe $PipeName$  -file \"$FilePath$\"";
    iniFile->WriteString(L"SET", L"Size", L"770048");
    iniFile->WriteString(L"Pipe", L"NamedPipe", L"1");
    iniFile->WriteString(L"Pipe", L"Buff_MiB", L"3");
    wstring section;
    section = L"Client_0";
    iniFile->WriteString(section, L"Enable", L"1");
    iniFile->WriteString(section, L"Path", pfA_Path);
    iniFile->WriteString(section, L"Args", pfA_Args);
    iniFile->WriteString(section, L"Hide", L"1");
  }



};




