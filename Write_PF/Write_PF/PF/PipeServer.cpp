/*
デバッグ用コマンドライン
デバッグ  -->  コマンド
$(Platform)\$(Configuration)\WriteDllTester_edcb.exe
$(Platform)\$(Configuration)\e.exe

デバッグ  -->  引数
args[1]               [2]              [3]                 [4]           [5]                [6]
"input file"          "output folder"  "output_foldersub"  limit_MiBsec  dll_name           overwrite
"D:\input.ts"         "D:\\"           "E:\\"              0             Write_Default.dll  1
"E:\TS_Samp\t2s.ts"   "E:\pf_out"      "D:"                1.9           Write_Default.dll  1
"E:\TS_Samp\t2s.ts"   "E:\pf_out"      "D:"                20.0          Write_PF.dll       1

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
///クライアント引数  置換
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
///readerのbuffにデータ追加
///=============================
void PipeServer::Append(BYTE* data, const DWORD size)
{
  if (reader != nullptr)
    reader->Append(data, size);
}


///=============================
///送信スレッド停止
///=============================
/*
WriteDllTester_edcb.exeは処理終了後 15secでスレッドが終了しないとterminate()する。
ステップ実行で時間をかけているとスレッド関連のエラーが発生
- hSenderThread.join();ができない
- abort() has been called
- mutex処理のアクセス違反　等々
通常実行ではすぐにスレッドが終了するので問題ない。
*/
void PipeServer::Stop()
{
  if (hSenderThread == nullptr)
    return;

  //停止を要求
  //  全てのデータを送信できなくてもいい。 Client側の消化が遅いと全て送信できない。
  //  残りはClient側でファイルを読んで処理してもらう。
  //  送信スレッドに”ＴＳファイル書き込み完了”を通知
  this->TS_FinishWrite = true;

  //停止命令
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
///データ送信
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


