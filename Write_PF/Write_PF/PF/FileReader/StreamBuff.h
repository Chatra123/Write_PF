#pragma once
#include "../../stdafx.h"
#include "FileReader_Log.h"


///=============================
///ＴＳ書き込み用のデータをバッファに保持
///=============================
//  thread safe
class StreamBuff
{
private:
  mutex sync;
  __int64 BuffMax;
  deque<shared_ptr<__int64>> Que_fpos;
  deque<shared_ptr<BYTE>> Que_Data;
  deque<shared_ptr<DWORD>> Que_Size;
  __int64 TotalSize = 0;

  shared_ptr<FileReader_Log> log;


public:
  StreamBuff(int _buffmax, shared_ptr<FileReader_Log> &_log)
    : BuffMax(_buffmax), log(_log) { }
  ~StreamBuff() { }



  ///=============================
  ///バッファにデータ追加
  ///=============================
  bool AppendData(
    shared_ptr<__int64> &append_fpos,
    shared_ptr<BYTE> &append_data,
    shared_ptr<DWORD> &append_size)
  {
    lock_guard<mutex> lock(sync);

    //dequeue
    while (0 < Que_fpos.size()
      && BuffMax < TotalSize + *append_size)
    {
      log->ReleaseBuff(*Que_fpos.front(), *Que_Size.front());

      TotalSize -= *Que_Size.front();
      Que_fpos.pop_front();
      Que_Data.pop_front();
      Que_Size.pop_front();
    }

    //enqueue
    if (TotalSize + *append_size <= BuffMax)
    {
      log->AppendBuff(*append_fpos, *append_size);

      TotalSize += *append_size;
      Que_fpos.emplace_back(append_fpos);
      Que_Data.emplace_back(append_data);
      Que_Size.emplace_back(append_size);
      return true;
    }
    else
    {
      /*
        this->BuffMaxが小さい  or  ファイル書込用バッファが大きい
        WriteBuffよりBuffMaxを十分大きくすること。
      */
      log->AppendBuff_FailAppend(*append_fpos, *append_size);
      return false;
    }
  }


  ///=============================
  /// バッファ先頭のファイル位置を取得
  ///=============================
  __int64 GetTopPos()
  {
    lock_guard<mutex> lock(sync);
    if (0 < Que_fpos.size())
      return *Que_fpos.front();
    else
      return -1;
  }


  ///=============================
  /// バッファからデータ取得
  ///=============================
  void GetData(const __int64 req_fpos,
    shared_ptr<BYTE> &retData, shared_ptr<DWORD> &retSize)
  {
    lock_guard<mutex> lock(sync);

    // find req_fpos at que
    int index = -1;
    for (size_t i = 0; i < Que_fpos.size(); i++)
    {
      if (*Que_fpos[i] == req_fpos)
      {
        //found
        index = static_cast<int>(i);
        break;
      }
    }
    if (index == -1)
      return;// not found

    //share
    retData = Que_Data[index];
    retSize = Que_Size[index];

    //dequeue
    for (int i = 0; i <= index; i++)
    {
      log->ReleaseBuff(*Que_fpos.front(), *Que_Size.front());

      TotalSize -= *Que_Size.front();
      Que_fpos.pop_front();
      Que_Data.pop_front();
      Que_Size.pop_front();
    }
  }




};











