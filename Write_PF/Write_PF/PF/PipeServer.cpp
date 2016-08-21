/*
�f�o�b�O�p�R�}���h���C��
�f�o�b�O  -->  �R�}���h
  $(Platform)\$(Configuration)\WriteDllTester_edcb.exe
  $(Platform)\$(Configuration)\e.exe


�f�o�b�O  -->  ����
  args[1]               [2]              [3]                 [4]           [5]                [6]
  "input file"          "output folder"  "output_foldersub"  limit_MiBsec  dll_name           overwrite
  "D:\input.ts"         "D:\\"           "E:\\"              0             Write_Default.dll  1
  "D:\input.ts"         "D:\rec\\"       "E:\\"              -1
  "E:\TS_Samp\t2s.ts"   "E:\pf_out"      "D:"                20.0          Write_PF.dll       1
  "E:\TS_Samp\t60s.ts"  "E:\pf_out"      "D:"                1.9           Write_PF.dll       1
  "E:\TS_Samp\t60s.ts"  "E:\pf_out"      "D:"                1.9           Write_Default.dll  1


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
///constructor
///=============================
PipeServer::PipeServer(){ }
PipeServer::~PipeServer(){ }



///=============================
///Initialize
///=============================
void PipeServer::Initialize(wstring filepath, wstring iniPath)
{
  //read ini
  // [Pipe]
  bool pipe_Enable = Ini_Util::GetBool(L"Pipe", L"Enable", false, iniPath);
  double pipe_Buff_MiB = Ini_Util::GetDouble(L"Pipe", L"Buff_MiB", 3, iniPath);
  // [Client]
  bool client_Enable = Ini_Util::GetBool(L"Client", L"Enable", false, iniPath);
  wstring client_Path = Ini_Util::GetString(L"Client", L"Path", L"", iniPath);
  wstring client_Args = Ini_Util::GetString(L"Client", L"Arguments", L"", iniPath);
  bool client_Hide = Ini_Util::GetBool(L"Client", L"Hide", false, iniPath);


  //pipe
  this->Pipe_Enable = pipe_Enable;
  wstring pipeName = L"WritePF" + PF_Util::GetUUID();
  hQuitOrder = CreateEvent(NULL, TRUE, FALSE, NULL);
  if (hQuitOrder == INVALID_HANDLE_VALUE)
    return;

  //select one
  //pipe = make_unique<AnonPipe>();
  pipe = make_unique<NamedPipe>(pipeName, hQuitOrder);

  if (client_Enable)
  {
    wstring args = ReplaceMacro(client_Args, pipeName, filepath);
    wstring command = L"\"" + client_Path + L"\"  " + args;
    pipe->RunClient(command, client_Hide);
  }

  //FileReader
  int pipe_Buff_B = static_cast<int>(pipe_Buff_MiB * 1024 * 1024);
  reader = make_unique<FileReader>(filepath, pipe_Buff_B);

  //thread
  this->hSenderThread = thread(SendMain_Invoker, (LPVOID)this);

  this->Initialized = true;
}



///=============================
///�N���C�A���g����  �u��
///=============================
wstring PipeServer::ReplaceMacro(
  const wstring baseArgs,
  const wstring pipeName, const wstring filePath)
{
  wstring args = baseArgs;

  //r11�܂�
  wregex regex1(L"(\\$npipe\\$)", regex_constants::icase);
  args = regex_replace(args, regex1, pipeName);
  wregex regex2(L"(\\$file\\$)", regex_constants::icase);
  args = regex_replace(args, regex2, filePath);

  //r12����
  wregex regex3(L"(\\$PipeName\\$)", regex_constants::icase);
  args = regex_replace(args, regex3, pipeName);
  wregex regex4(L"(\\$FilePath\\$)", regex_constants::icase);
  args = regex_replace(args, regex4, filePath);

  return args;
}



///=============================
///���M��~
///=============================
/*
  WriteDllTester_edcb.exe�͏����I���� 15sec��dll���I�����Ȃ��ƃX���b�h��terminate()����B
  �X�e�b�v���s�Ŏ��Ԃ������Ă���ƃX���b�h�֘A�̃G���[������
    - hSenderThread.join();���ł��Ȃ�
    - abort() has been called
    - mutex�����̃A�N�Z�X�ᔽ�@���X

  �ʏ���s�ł͂����ɃX���b�h���I������̂Ŗ��Ȃ��B
*/
void PipeServer::Stop()
{
  if (this->Initialized == false) return;

  //���M�X���b�h��~
  //�@�S�Ẵf�[�^�𑗐M�ł��Ȃ��Ă������B pfAdapter���̏������x���ƑS�đ��M�ł��Ȃ��B
  //�@�c���pfAdapter���Ńt�@�C���ǂݍ��݂����ď�������B

  //���M�X���b�h�ɂs�r�t�@�C���������݊�����ʒm
  this->TS_FinishWrite = true;
  Sleep(1000);


  //��~����
  if (hQuitOrder)
    SetEvent(hQuitOrder);

  if (hSenderThread.joinable())
    hSenderThread.join();

  if (hQuitOrder)
  {
    CloseHandle(hQuitOrder);
    hQuitOrder = nullptr;
  }

}




///=============================
///�o�b�t�@�Ƀf�[�^�ǉ�
///=============================
void PipeServer::AppendData(BYTE* data, const DWORD size)
{
  if (this->Initialized == false)
    throw exception();

  reader->AppendBuff(data, size);
}

void PipeServer::AppendData(vector<BYTE> &data, const DWORD size)
{
  if (this->Initialized == false) 
    throw exception();

  reader->AppendBuff(&data.front(), size);
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
  if (Pipe_Enable == false) return;

  bool connect = pipe->Connect();
  if (connect == false)
  {
    // hQuitOrder or �G���[
    return;
  }


  bool quit = false;
  while (true)
  {
    if (quit)
    {
      // hQuitOrder or �G���[
      break;
    }

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
      BOOL success = pipe->Write(data, size, written);
      if (success)
        reader->Seek_Read_fpos(written);
      else
        quit = true;// hQuitOrder or �G���[

      //�^�撆�Ȃ�sleep
      if (this->TS_FinishWrite == false)
        Sleep(100);
    }
    else
    {
      Sleep(30);
    }

  }//end while

  pipe->Close();
  reader->Close();
  return;
}


