#pragma once
#include "../stdafx.h"


class PF_Util
{
  ///=============================
  ///アプリフォルダをカレントに設定
  ///=============================
public:
  static void SetCurrent_AppFolder()
  {
    wchar_t appPath[_MAX_PATH];
    GetModuleFileName(NULL, appPath, _MAX_PATH);
    // folder path
    wchar_t root[_MAX_PATH], dir[_MAX_PATH];
    _wsplitpath_s(appPath, root, _MAX_PATH, dir, _MAX_PATH, NULL, 0, NULL, 0);
    wstring appDir = (wstring)root + (wstring)dir;
    SetCurrentDirectory(appDir.c_str());
  }


  ///=============================
  ///UUID作成
  ///=============================
public:
  static wstring GetUUID()
  {
    UUID uuid;
    UuidCreate(&uuid);
    RPC_WSTR lpszUuid = NULL;
    UuidToString(&uuid, &lpszUuid);
    wstring strId = (LPCTSTR)lpszUuid;
    RpcStringFree(&lpszUuid);
    return strId;
  }

};




class Ini_Util
{
  ///=============================
  /// ini --> string
  ///=============================
public:
  static wstring GetString(wstring key, wstring value, wstring defalut, wstring inipath)
  {
    wchar_t buff[_MAX_PATH] = {};
    GetPrivateProfileString(key.c_str(), value.c_str(), defalut.c_str(), buff, _MAX_PATH, inipath.c_str());
    return wstring(buff);
  }


  ///=============================
  /// ini --> int
  ///=============================
public:
  static int GetInt(wstring key, wstring value, int defalut, wstring inipath)
  {
    wstring text = GetString(key, value, L"none", inipath);
    try               { return std::stoi(text); }
    catch (exception) { return defalut; }
  }


  ///=============================
  /// ini --> double
  ///=============================
public:
  static double GetDouble(wstring key, wstring value, double defalut, wstring inipath)
  {
    wstring text = GetString(key, value, L"none", inipath);
    try               { return std::stod(text); }
    catch (exception) { return defalut; }
  }


  ///=============================
  /// ini --> bool
  ///=============================
public:
  static bool GetBool(wstring key, wstring value, bool defalut, wstring inipath)
  {
    wstring text = GetString(key, value, L"none", inipath);
    try               { return std::stoi(text) != 0; }
    catch (exception) { return defalut; }
  }
};



class PF_ini
{
  ///=============================
  /// Create ini
  ///=============================
public:
  static void Create(wstring inipath)
  {
    bool exist = _waccess_s(inipath.c_str(), 0) == 0;
    if (exist) return;

    const wstring Client_Path = L".\\Write\\Write_PF\\pfAdapter\\pfAdapter.exe";
    const wstring Client_Args = L"-npipe $PipeName$  -file \"$FilePath$\"";

    WritePrivateProfileString(L"SET", L"Size", L"770048", inipath.c_str());
    WritePrivateProfileString(L"Pipe", L"Enable", L"1", inipath.c_str());
    WritePrivateProfileString(L"Pipe", L"Buff_MiB", L"3", inipath.c_str());
    WritePrivateProfileString(L"Client", L"Enable", L"1", inipath.c_str());
    WritePrivateProfileString(L"Client", L"Path", Client_Path.c_str(), inipath.c_str());
    WritePrivateProfileString(L"Client", L"Arguments", Client_Args.c_str(), inipath.c_str());
    WritePrivateProfileString(L"Client", L"Hide", L"1", inipath.c_str());
  }
};
















