#pragma once
#include "../../stdafx.h"
#include "PipeWriter.h"


///=============================
///　Named Pipe         non-blocking I/O
///=============================
class NamedPipe : public PipeWriter
{
private:
  wstring PipeName;
  HANDLE hQuitOrder;
  HANDLE hWritePipe;

public:
  NamedPipe(wstring pipename, HANDLE hquit)
  {
    PipeName = pipename;
    hQuitOrder = hquit;
    hWritePipe = INVALID_HANDLE_VALUE;
  }
  ~NamedPipe(){}



  ///=============================
  ///　クライアント起動
  ///=============================
public:
  void RunClient(wstring command, bool hideClient)
  {
    DWORD dwCreationFlags = hideClient
      ? NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW
      : NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE;

    STARTUPINFO si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);

    BOOL ret = CreateProcess(
      NULL,                          //実行可能モジュールの名前
      (LPWSTR)command.c_str(),       //コマンドラインの文字列
      NULL,                          //セキュリティ記述子
      NULL,                          //セキュリティ記述子
      FALSE,                         //ハンドルの継承オプション
      dwCreationFlags,               //作成のフラグ
      NULL,                          //新しい環境ブロック
      NULL,                          //カレントディレクトリの名前
      &si,                           //スタートアップ情報
      &pi);                          //プロセス情報

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
    //作成
    wstring fullname = L"\\\\.\\pipe\\" + this->PipeName;
    hWritePipe = CreateNamedPipe(
      fullname.c_str(),                             //パイプ名
      PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,  //パイプを開くモード
      PIPE_TYPE_BYTE,                               //パイプ固有のモード
      1,                                            //インスタンスの最大数
      1024 * 128,                                   //出力バッファのサイズ
      0,                                            //入力バッファのサイズ
      1000,                                         //規定の接続タイムアウト
      NULL);                                        //セキュリティ記述子
    if (hWritePipe == INVALID_HANDLE_VALUE) return false;


    //接続
    HANDLE hPipeEvent = INVALID_HANDLE_VALUE;
    hPipeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hPipeEvent == INVALID_HANDLE_VALUE) return false;

    OVERLAPPED ovl = {};
    ovl.hEvent = hPipeEvent;


    bool connect;
    if (ConnectNamedPipe(hWritePipe, &ovl))
    {
      // Overlapped ConnectNamedPipe should return zero. 
      connect = false;
    }
    else
    {
      DWORD lastErr = GetLastError();
      if (lastErr == ERROR_PIPE_CONNECTED)
      {
        // success
        //   client connects before the function is called.
        //   there is a good connection between client and server.
        connect = true;
      }
      else if (lastErr == ERROR_IO_PENDING)
      {
        //待機
        if (WaitEvent(hPipeEvent, hQuitOrder))
        {
          DWORD lpTransferred;
          if (GetOverlappedResult(hWritePipe, &ovl, &lpTransferred, FALSE))
          {
            // success
            connect = true;
          }
          else
          {
            connect = false;
          }
        }
        else//hQuitOrder event is fired
          connect = false;
      }
      else// If an error occurs during the connect operation... 
        connect = false;
    }

    if (hPipeEvent != INVALID_HANDLE_VALUE)
    {
      CloseHandle(hPipeEvent);
      hPipeEvent = INVALID_HANDLE_VALUE;
    }
    return connect;
  }



  ///=============================
  ///　パイプ書き込み
  ///=============================
public:
  BOOL Write(shared_ptr<BYTE> &data, shared_ptr<DWORD> &size, DWORD &written)
  {
    HANDLE hPipeEvent;
    hPipeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hPipeEvent == INVALID_HANDLE_VALUE)
      return FALSE;

    OVERLAPPED ovl = {};
    ovl.hEvent = hPipeEvent;

    //書
    BOOL success = WriteFile(hWritePipe, data.get(), *size, &written, &ovl);
    if (success)
    {
      //success
    }
    else
    {
      //待機
      if (WaitEvent(hPipeEvent, hQuitOrder))
      {
        success = GetOverlappedResult(hWritePipe, &ovl, &written, FALSE);
      }
      else
      {
        //hQuitOrder event is fired
        success = FALSE;
      }
    }

    if (hPipeEvent != INVALID_HANDLE_VALUE)
    {
      CloseHandle(hPipeEvent);
      hPipeEvent = INVALID_HANDLE_VALUE;
    }
    return success;
  }



  ///=============================
  ///　送信終了
  ///=============================
public:
  void Close()
  {
    if (hWritePipe != INVALID_HANDLE_VALUE)
    {
      FlushFileBuffers(hWritePipe);
      Sleep(300);

      DisconnectNamedPipe(hWritePipe);
      CloseHandle(hWritePipe);
      hWritePipe = INVALID_HANDLE_VALUE;
    }
  }



  ///=============================
  ///　イベント待機
  ///=============================
  // some event are fired
  //   hEvent               -->  return true;
  //   hQuitEvent           -->  return false;
  //   hQuitEvent & hEvent  -->  return false;
private:
  bool WaitEvent(HANDLE hEvent, HANDLE hQuitEvent)
  {
    HANDLE hWaits[2] = { hQuitEvent, hEvent };  //hQuitEventが優先

    //WaitForMultipleObjects
    //  複数のオブジェクトがシグナル状態になった場合は、最小のインデックス番号が返ります。
    //  hEvent               -->  ret = 1;
    //  hQuitEvent           -->  ret = 0;
    //  hQuitEvent & hEvent  -->  ret = 0;
    DWORD ret = WaitForMultipleObjects(2, hWaits, FALSE, INFINITE);
    return ret == WAIT_OBJECT_0 + 1;
  }


  
};



