/*
デバッグ用コマンドライン
デバッグ  -->  コマンド
  $(Platform)\$(Configuration)\WriteDllTester_edcb.exe
  $(Platform)\$(Configuration)\e.exe


デバッグ  -->  引数
  args[1]               [2]              [3]                 [4]           [5]                [6]
  "input file"          "output folder"  "output_foldersub"  limit_MiBsec  dll_name           overwrite
  "D:\input.ts"         "D:\\"           "E:\\"              0             Write_Default.dll  1
  "D:\input.ts"         "D:\rec\\"       "E:\\"              -1
  "E:\TS_Samp\t2s.ts"   "E:\pf_out"      "D:"                20.0          Write_PF.dll       1
  "E:\TS_Samp\t60s.ts"  "E:\pf_out"      "D:"                1.9           Write_PF.dll       1
  "E:\TS_Samp\t60s.ts"  "E:\pf_out"      "D:"                1.9           Write_Default.dll  1


  ・出力フォルダが存在しないと作成される。
  ・フォルダ名の最後に\はつけなくてもいい。
  ・c++  最後に\をつける場合は\\にする。\だけだと特殊文字 \"と認識される。
  ・引数間に全角スペースがあるとダメ
*/
/*
Write_Default --> Write_PF  の変更箇所

・.sln .vcxproj .vcxproj.filters .vcxproj.user ファイル名変更
・Write_Default.cpp  line18  PLUGIN_NAME
・Write_PlugIn.def　 line11  LIBRARY名

・stdafx.hにinclude追加
・WriteMain.h、WriteMain.cppにコード追加

・#include "stdafx.h" でエラーになるときは、
  PipeServer.cpp --> プロパティ --> プリコンパイル済みヘッダーを使用しない

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
///クライアント引数  置換
///=============================
wstring PipeServer::ReplaceMacro(
  const wstring baseArgs,
  const wstring pipeName, const wstring filePath)
{
  wstring args = baseArgs;

  //r11まで
  wregex regex1(L"(\\$npipe\\$)", regex_constants::icase);
  args = regex_replace(args, regex1, pipeName);
  wregex regex2(L"(\\$file\\$)", regex_constants::icase);
  args = regex_replace(args, regex2, filePath);

  //r12から
  wregex regex3(L"(\\$PipeName\\$)", regex_constants::icase);
  args = regex_replace(args, regex3, pipeName);
  wregex regex4(L"(\\$FilePath\\$)", regex_constants::icase);
  args = regex_replace(args, regex4, filePath);

  return args;
}



///=============================
///送信停止
///=============================
/*
  WriteDllTester_edcb.exeは処理終了後 15secでdllが終了しないとスレッドをterminate()する。
  ステップ実行で時間をかけているとスレッド関連のエラーが発生
    - hSenderThread.join();ができない
    - abort() has been called
    - mutex処理のアクセス違反　等々

  通常実行ではすぐにスレッドが終了するので問題ない。
*/
void PipeServer::Stop()
{
  if (this->Initialized == false) return;

  //送信スレッド停止
  //　全てのデータを送信できなくてもいい。 pfAdapter側の消化が遅いと全て送信できない。
  //　残りはpfAdapter側でファイル読み込みをして処理する。

  //送信スレッドにＴＳファイル書き込み完了を通知
  this->TS_FinishWrite = true;
  Sleep(1000);


  //停止命令
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
///バッファにデータ追加
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
///データ送信
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
    // hQuitOrder or エラー
    return;
  }


  bool quit = false;
  while (true)
  {
    if (quit)
    {
      // hQuitOrder or エラー
      break;
    }

    if (this->TS_FinishWrite)
    {
      //全データ送信後にスレッド停止
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
        quit = true;// hQuitOrder or エラー

      //録画中ならsleep
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


