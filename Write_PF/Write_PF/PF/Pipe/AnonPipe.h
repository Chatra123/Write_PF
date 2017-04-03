#pragma once
#include "../../stdafx.h"
#include "PipeWriter.h"


///=============================
///　Anonymous Pipe
///=============================
class AnonPipe : public PipeWriter
{
private:
  HANDLE hWritePipe;

public:
  AnonPipe()
  {
    hWritePipe = INVALID_HANDLE_VALUE;
  }


  ///=============================
  ///　クライアント起動、パイプ接続
  ///=============================
public:
  void RunClient(wstring command, bool hide)
  {
    //パイプ作成
    HANDLE hReadPipe;
    {
      HANDLE hTempPipe;
      if (CreatePipe(&hTempPipe, &hWritePipe, NULL, 1024 * 128) == FALSE)
        return;
      BOOL ret = DuplicateHandle(
        GetCurrentProcess(),
        hTempPipe,
        GetCurrentProcess(),
        &hReadPipe,
        0,
        TRUE,
        DUPLICATE_SAME_ACCESS);
      if (ret == FALSE)
        return;
      CloseHandle(hTempPipe);
    }

    //プロセス起動
    SECURITY_ATTRIBUTES sa = {};
    STARTUPINFO si = {};
    PROCESS_INFORMATION pi = {};
    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdInput = hReadPipe;
    //標準(エラー)出力はnulデバイスに捨てる
    si.hStdOutput = CreateFile(L"nul", GENERIC_WRITE, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    si.hStdError = CreateFile(L"nul", GENERIC_WRITE, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    BOOL ret = CreateProcess(
      NULL,
      (LPWSTR)command.c_str(),
      NULL,
      NULL,
      TRUE,
      CREATE_NO_WINDOW,
      NULL,
      NULL,
      &si,
      &pi);
    CloseHandle(hReadPipe);
    CloseHandle(si.hStdOutput);
    CloseHandle(si.hStdError);
    if (ret) {
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
    }
  }

  ///=============================
  ///　パイプ接続
  ///=============================
public:
  bool Connect()
  {
    /* do nothing */
    return true;
  }


  ///=============================
  ///　パイプ書き込み
  ///=============================
public:
  BOOL Write(shared_ptr<BYTE> &data, shared_ptr<DWORD> &size, DWORD& written)
  {
    if (hWritePipe == INVALID_HANDLE_VALUE)
      return FALSE;
    BOOL success = WriteFile(hWritePipe, data.get(), *size, &written, NULL);
    return success;
  }


  ///=============================
  ///　送信終了
  ///=============================
public:
  void Close()
  {
    CloseHandle(hWritePipe);
  }



};



