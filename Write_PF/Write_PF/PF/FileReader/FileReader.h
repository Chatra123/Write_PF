/*
仮想のＴＳファイルから読込
（ ＴＳ書込用データと実ファイル ）
（ FileBlockBuff     *fp   ）

*/
#pragma once
#include "../../stdafx.h"
#include "FileBlockBuff.h"
#include "FileReader_Log.h"


class FileReader
{
private:
  timed_mutex sync;
  unique_ptr<FileBlockBuff> buff;
  shared_ptr<FileReader_Log> log;
  FILE *fp;
  __int64 fpos_read = 0;         //仮想ファイル読込み位置
  __int64 fpos_write = 0;        //実ファイル書込みの先端
  bool TS_FinishWrite = false;


  ///=============================
  /// Constructor
  ///=============================
public:
  FileReader(
    const wstring filepath,
    const int buffsize,
    const int no)
  {
    wstring logpath = filepath + L".wrpf-" + to_wstring(no) + L".log";
    log = make_shared<FileReader_Log>(logpath);
    buff = make_unique<FileBlockBuff>(buffsize, log);
    fp = _wfsopen(filepath.c_str(), L"rbN", _SH_DENYNO);
    // N :子プロセスに継承しない
    //"N"がないとAnonPipeでclient_1を開くときにfpも渡してしまう。
    //基本的にclient_1を実行することはなく、client_1のpfAdapterが終了するまでfilepathの移動ができなくなるだけ。
    //なくても大きな問題にはなることはないがつけておく。
    //NamedPipeなら"N"はなくても問題ない。
  }
  void Close()
  {
    if (fp)
      fclose(fp);
  }


  ///=============================
  ///バッファにデータ追加
  ///=============================
public:
  void Append(BYTE* data, const DWORD size)
  {
    //メインスレッドから呼ばれるので timeout
    unique_lock<timed_mutex> lock(sync, chrono::milliseconds(10));
    if (lock)
    {
      //copy data
      auto append_fpos = make_shared<__int64>(fpos_write);
      shared_ptr<BYTE> append_data(new BYTE[size]);
      auto append_size = make_shared<DWORD>(size);
      memcpy(append_data.get(), data, size);
      buff->Append(append_fpos, append_data, append_size);
      fpos_write += size;
    }
    else
    {
      log->FailLock(fpos_write, size);
      fpos_write += size;
    }
  }


  ///=============================
  ///ＴＳファイル書込みの完了通知を受け取る
  ///=============================
public:
  void Notify_TSFinishWrite()
  {
    //”ファイル読込はBuff fposの手前まで”の制限を解除
    TS_FinishWrite = true;
  }


  ///=============================
  ///IsEOF
  ///=============================
public:
  bool IsEOF()
  {
    if (fp)
      return ferror(fp) || feof(fp);
    else
      return true;
  }


  ///=============================
  ///データ取得
  ///=============================
public:
  void GetNext(shared_ptr<BYTE> &ret_data, shared_ptr<DWORD> &ret_size)
  {
    //実ファイルの読込を許可する最大位置　　buffの手前まで。
    __int64 readable_fpos;

    //Buff
    {//lock scope
      lock_guard<timed_mutex> lock(sync);
      //buff is empty ?
      //  バッファ消化が早いときにすぐにファイル読込みをしないように制限
      bool buffIsEmpty = buff->GetTopPos() < 0;
      if (buffIsEmpty && TS_FinishWrite == false)
        return;
      readable_fpos = TS_FinishWrite ? -1 : buff->GetTopPos() - 1;
      buff->GetData(fpos_read, ret_data, ret_size);
    }
    if (ret_data != nullptr)
    {
      log->GetData_fromBuff(fpos_read, *ret_size);
      return;
    }

    //File
    Get_fromFile(fpos_read, readable_fpos, ret_data, ret_size);
    if (ret_data != nullptr)
    {
      log->GetData_fromFile(fpos_read, *ret_size);
      return;
    }
    return;
  }



  ///=============================
  ///実ファイル読込
  ///=============================
  //”ファイル読込はBuff fposの手前まで”に制限
  //Buff fposと重ならないようにする。
private:
  void Get_fromFile(
    const __int64 req_fpos, const __int64 readable_fpos,
    shared_ptr<BYTE> &ret_data, shared_ptr<DWORD> &ret_size)
  {
    if (!fp || ferror(fp) || feof(fp))
      return;

    const UINT ReadMax = 770048;
    __int64 req_size;
    {
      //ＴＳ書込が完了なら readable_fpos < 0
      __int64 req_pos_last = req_fpos + ReadMax - 1;
      if (0 <= readable_fpos
        && readable_fpos < req_pos_last)
        req_size = readable_fpos - req_fpos + 1;
      else
        req_size = ReadMax;
    }
    if (req_size <= 0)
      return;

    shared_ptr<BYTE> data(new BYTE[(UINT)req_size]);
    _fseeki64(fp, req_fpos, SEEK_SET);
    size_t read_count = fread((char *)data.get(), sizeof(char), (size_t)req_size, fp);
    auto size = make_shared<DWORD>((DWORD)read_count);
    if (read_count == 0)
      return;
    ret_data = data;
    ret_size = size;
  }



  ///=============================
  ///Seek
  ///=============================
public:
  void Seek(const DWORD seek)
  {
    fpos_read += seek;
    log->Seek(fpos_read, seek);
  }



};



