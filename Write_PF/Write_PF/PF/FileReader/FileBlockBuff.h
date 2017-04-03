#pragma once
#include "../../stdafx.h"
#include "FileReader_Log.h"


///=============================
///Data��Size�P�ʂ�Buff�ɕێ�
///=============================
class FileBlockBuff
{
private:
  __int64 BuffMax;
  deque<shared_ptr<__int64>> Que_fpos;
  deque<shared_ptr<BYTE>> Que_Data;
  deque<shared_ptr<DWORD>> Que_Size;
  shared_ptr<FileReader_Log> log;

  ///=============================
  ///constructor
  ///=============================
public:
  FileBlockBuff(const int buffsize,
    shared_ptr<FileReader_Log> log)
  {
    this->BuffMax = buffsize;
    this->log = log;
  }

  ///=============================
  ///���T�C�Y�擾
  ///=============================
private:
  DWORD GetCurSize()
  {
    DWORD sum = 0;
    for (auto pSize : Que_Size)
      sum += *pSize;
    return sum;
  }


  ///=============================
  /// �o�b�t�@�擪�̃t�@�C���ʒu���擾�@�@�ŏ��̃t�@�C���ʒu
  ///=============================
public:
  __int64 GetTopPos()
  {
    if (Que_fpos.empty())
      return -1;
    else
      return *Que_fpos.front();
  }


  ///=============================
  ///�o�b�t�@�Ƀf�[�^�ǉ�
  ///=============================
public:
  void Append(
    shared_ptr<__int64> &fpos,
    shared_ptr<BYTE> &data,
    shared_ptr<DWORD> &size)
  {
    //dequeue
    while (0 < Que_fpos.size()
      && BuffMax < GetCurSize() + *size)
    {
      log->ReleaseBuff(*Que_fpos.front(), *Que_Size.front());
      Que_fpos.pop_front();
      Que_Data.pop_front();
      Que_Size.pop_front();
    }
    //enqueue
    if (GetCurSize() + *size <= BuffMax)
    {
      log->AppendBuff(*fpos, *size);
      Que_fpos.emplace_back(fpos);
      Que_Data.emplace_back(data);
      Que_Size.emplace_back(size);
    }
  }


  ///=============================
  /// �o�b�t�@����f�[�^�擾
  ///=============================
public:
  void GetData(const __int64 req_fpos,
    shared_ptr<BYTE> &ret_data, shared_ptr<DWORD> &ret_size)
  {
    //find req_fpos at Que
    int idx_found = -1;
    for (size_t i = 0; i < Que_fpos.size(); i++)
    {
      if (*Que_fpos[i] == req_fpos)
      {
        idx_found = static_cast<int>(i);
        break;
      }
    }
    if (idx_found == -1)
      return;// not found

             //share
    ret_data = Que_Data[idx_found];
    ret_size = Que_Size[idx_found];

    //dequeue
    for (int i = 0; i <= idx_found; i++)
    {
      log->ReleaseBuff(*Que_fpos.front(), *Que_Size.front());
      Que_fpos.pop_front();
      Que_Data.pop_front();
      Que_Size.pop_front();
    }
  }

};




