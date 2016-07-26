#pragma once
#include "../stdafx.h"
#include "PF_Util.h"
#include "FileReader/FileReader.h"
#include "Pipe/NamedPipe.h"
#include "Pipe/AnonPipe.h"


class PipeServer
{
public:
  PipeServer();
  ~PipeServer();

  //メインスレッド
  void Initialize(wstring filepath, wstring inipath);
  void AppendData(BYTE* data, const DWORD size);
  void AppendData(vector<BYTE> &data, const DWORD size);
  void Stop();

  wstring ReplaceMacro(
    const wstring baseArgs,
    const wstring pipeName, const wstring filePath);

  //送信スレッド
  static void SendMain_Invoker(void* param);
  void SendMain();


private:
  bool Initialized = false;
  bool TS_FinishWrite = false;

  bool Pipe_Enable = false;
  unique_ptr<PipeWriter> pipe;
  unique_ptr<FileReader> reader;

  thread hSenderThread;
  HANDLE hQuitOrder;


};



