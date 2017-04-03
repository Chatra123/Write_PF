#pragma once
#include "../stdafx.h"


class PF_Util
{
  ///=============================
  ///アプリケーションフォルダをカレントに設定
  ///=============================
public:
  static void SetCurrent_AppFolder()
  {
    wchar_t appPath[_MAX_PATH];
    GetModuleFileName(NULL, appPath, _MAX_PATH);
    // full path --> folder path
    wchar_t root[_MAX_PATH], dir[_MAX_PATH];
    _wsplitpath_s(appPath, root, _MAX_PATH, dir, _MAX_PATH, NULL, 0, NULL, 0);
    wstring appDir = wstring(root) + wstring(dir);
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
    wstring strID = (LPCTSTR)lpszUuid;
    RpcStringFree(&lpszUuid);
    return strID;
  }

};







