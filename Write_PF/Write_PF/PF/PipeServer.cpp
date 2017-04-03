/*
�f�o�b�O�p�R�}���h���C��
�f�o�b�O  -->  �R�}���h
$(Platform)\$(Configuration)\WriteDllTester_edcb.exe
$(Platform)\$(Configuration)\e.exe

�f�o�b�O  -->  ����
args[1]               [2]              [3]                 [4]           [5]                [6]
"input file"          "output folder"  "output_foldersub"  limit_MiBsec  dll_name           overwrite
"D:\input.ts"         "D:\\"           "E:\\"              0             Write_Default.dll  1
"E:\TS_Samp\t2s.ts"   "E:\pf_out"      "D:"                1.9           Write_Default.dll  1
"E:\TS_Samp\t2s.ts"   "E:\pf_out"      "D:"                20.0          Write_PF.dll       1

�E�o�̓t�H���_�����݂��Ȃ��ƍ쐬�����B
�E�t�H���_���̍Ō��\�͂��Ȃ��Ă������B
�Ec++  �Ō��\������ꍇ��\\�ɂ���B\�������Ɠ��ꕶ�� \"�ƔF�������B
�E�����ԂɑS�p�X�y�[�X������ƃ_��
*/
/*
Write_Default --> Write_PF  �̕ύX�ӏ�
�E.sln .vcxproj .vcxproj.filters .vcxproj.user �t�@�C�����ύX
�EWrite_Default.cpp  line18  PLUGIN_NAME
�EWrite_PlugIn.def�@ line11  LIBRARY��
�Estdafx.h��include�ǉ�
�EWriteMain.h�AWriteMain.cpp�ɃR�[�h�ǉ�
�E#include "stdafx.h" �ŃG���[�ɂȂ�Ƃ��́A
PipeServer.cpp --> �v���p�e�B --> �v���R���p�C���ς݃w�b�_�[���g�p���Ȃ�

*/
#pragma once
#include "stdafx.h"
#include "PipeServer.h"


///=============================
///PipeServer
///=============================
PipeServer::PipeServer(
  const shared_ptr<SettingA> setting,
  const shared_ptr<SettingA::Setting_Client> client)
{
  /* test */
  //setting->Mode_NamedPipe = false;  //  true  false
  //setting->Mode_NamedPipe = true;

  this->setting = setting;
  this->setting_client = client;
  if (client->Enable == false) return;

  //create pipe
  wstring pipeName = L"WritePF_" + PF_Util::GetUUID();
  if (setting->Mode_NamedPipe)
  {
    hQuitOrder = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hQuitOrder == NULL)
      return;
    writer = make_unique<NamedPipe>(pipeName, hQuitOrder);
  }
  else
    writer = make_unique<AnonPipe>();

  //writer
  wstring args = ReplaceMacro(client->Args, pipeName, setting->TsPath);
  wstring command = L"\"" + client->Path + L"\"  " + args;
  writer->RunClient(command, client->Hide);
  //reader
  int buffsize = static_cast<int>(setting->Pipe_Buff_MiB * 1024 * 1024);
  reader = make_unique<FileReader>(setting->TsPath, buffsize, client->No);
  //sender
  hSenderThread = make_unique<thread>(SendMain_Invoker, (LPVOID)this);
}


///=============================
///�N���C�A���g����  �u��
///=============================
wstring PipeServer::ReplaceMacro(
  const wstring baseArgs,
  const wstring pipeName,
  const wstring tsPath)
{
  wstring args = baseArgs;
  wregex regex1(L"\\$PipeName\\$", regex_constants::icase);
  args = regex_replace(args, regex1, pipeName);
  wregex regex2(L"\\$FilePath\\$", regex_constants::icase);
  args = regex_replace(args, regex2, tsPath);
  return args;
}


///=============================
///reader��buff�Ƀf�[�^�ǉ�
///=============================
void PipeServer::Append(BYTE* data, const DWORD size)
{
  if (reader != nullptr)
    reader->Append(data, size);
}


///=============================
///���M�X���b�h��~
///=============================
/*
WriteDllTester_edcb.exe�͏����I���� 15sec�ŃX���b�h���I�����Ȃ���terminate()����B
�X�e�b�v���s�Ŏ��Ԃ������Ă���ƃX���b�h�֘A�̃G���[������
- hSenderThread.join();���ł��Ȃ�
- abort() has been called
- mutex�����̃A�N�Z�X�ᔽ�@���X
�ʏ���s�ł͂����ɃX���b�h���I������̂Ŗ��Ȃ��B
*/
void PipeServer::Stop()
{
  if (hSenderThread == nullptr)
    return;

  //��~��v��
  //  �S�Ẵf�[�^�𑗐M�ł��Ȃ��Ă������B Client���̏������x���ƑS�đ��M�ł��Ȃ��B
  //  �c���Client���Ńt�@�C����ǂ�ŏ������Ă��炤�B
  //  ���M�X���b�h�Ɂh�s�r�t�@�C���������݊����h��ʒm
  this->TS_FinishWrite = true;

  //��~����
  if (hQuitOrder) {
    Sleep(100);
    SetEvent(hQuitOrder);
  }
  if (hSenderThread->joinable())
    hSenderThread->join();
  if (hQuitOrder) {
    CloseHandle(hQuitOrder);
    hQuitOrder = nullptr;
  }
}


///=============================
///�f�[�^���M
///=============================
void PipeServer::SendMain_Invoker(void *param)
{
  PipeServer *pThis = static_cast<PipeServer *>(param);
  pThis->SendMain();
}
void PipeServer::SendMain()
{
  if (writer == nullptr || reader == nullptr)
    return;

  bool connect = writer->Connect();
  if (connect == false)
    return;// hQuitOrder or error

  bool quit = false;
  while (quit == false)
  {
    if (this->TS_FinishWrite)
    {
      //�S�f�[�^���M��ɃX���b�h��~
      reader->Notify_TSFinishWrite();
      if (reader->IsEOF())
        break;
    }

    //read
    shared_ptr<BYTE> data = nullptr;
    shared_ptr<DWORD> size = nullptr;
    reader->GetNext(data, size);
    //write
    if (data != nullptr)
    {
      DWORD written = 0;
      BOOL success = writer->Write(data, size, written);
      if (success)
        reader->Seek(written);
      else
        quit = true;// hQuitOrder or error
    }
    else
    {
      Sleep(50);
    }
  }//end while
  reader->Close();
  writer->Close();
}


