#pragma once
#include "../stdafx.h"
#include "PF_Util.h"
#include "Setting.h"
#include "FileReader/FileReader.h"
#include "Pipe/NamedPipe.h"
#include "Pipe/AnonPipe.h"


///=============================
///PipeServer
///=============================
class PipeServer
{
private:
  shared_ptr<SettingA> setting;
  shared_ptr<SettingA::Setting_Client> setting_client;
public:
  PipeServer::PipeServer(
    shared_ptr<SettingA> setting,
    shared_ptr<SettingA::Setting_Client> setting_client);
  wstring ReplaceMacro(
    const wstring baseArgs,
    const wstring pipeName,
    const wstring filePath);
  void Append(BYTE* data, const DWORD size);
  void Stop();

  static void SendMain_Invoker(void* param);
  void SendMain();

private:
  bool TS_FinishWrite = false;
  unique_ptr<FileReader> reader = nullptr;
  unique_ptr<PipeWriter> writer = nullptr;
  unique_ptr<thread> hSenderThread = nullptr;
  HANDLE hQuitOrder = INVALID_HANDLE_VALUE;
};


///=============================
///PipeServer�̍쐬
///=============================
class ServerManager
{
private:
  unique_ptr<PipeServer> PipeServer_0, PipeServer_1;
  shared_ptr<SettingA::Setting_Client> client_0, client_1;

public:
  void Init(shared_ptr<SettingA> &setting)
  {
    client_0 = make_shared<SettingA::Setting_Client>(setting->Client[0]);
    client_1 = make_shared<SettingA::Setting_Client>(setting->Client[1]);
    PipeServer_0 = make_unique<PipeServer>(setting, client_0);
    PipeServer_1 = make_unique<PipeServer>(setting, client_1);
  }
  void Append(BYTE* data, const DWORD size)
  {
    if (size == 0)
      return;
    PipeServer_0->Append(data, size);
    PipeServer_1->Append(data, size);
  }
  void Stop()
  {
    PipeServer_0->Stop();
    PipeServer_1->Stop();
  }

};



