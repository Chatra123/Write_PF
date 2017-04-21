/*
���z�̂s�r�t�@�C������Ǎ�
�i �s�r�����p�f�[�^�Ǝ��t�@�C�� �j
�i FileBlockBuff     *fp   �j

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
  __int64 fpos_read = 0;         //���z�t�@�C���Ǎ��݈ʒu
  __int64 fpos_write = 0;        //���t�@�C�������݂̐�[
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
    // N :�q�v���Z�X�Ɍp�����Ȃ�
    //"N"���Ȃ���AnonPipe��client_1���J���Ƃ���fp���n���Ă��܂��B
    //��{�I��client_1�����s���邱�Ƃ͂Ȃ��Aclient_1��pfAdapter���I������܂�filepath�̈ړ����ł��Ȃ��Ȃ邾���B
    //�Ȃ��Ă��傫�Ȗ��ɂ͂Ȃ邱�Ƃ͂Ȃ������Ă����B
    //NamedPipe�Ȃ�"N"�͂Ȃ��Ă����Ȃ��B
  }
  void Close()
  {
    if (fp)
      fclose(fp);
  }


  ///=============================
  ///�o�b�t�@�Ƀf�[�^�ǉ�
  ///=============================
public:
  void Append(BYTE* data, const DWORD size)
  {
    //���C���X���b�h����Ă΂��̂� timeout
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
  ///�s�r�t�@�C�������݂̊����ʒm���󂯎��
  ///=============================
public:
  void Notify_TSFinishWrite()
  {
    //�h�t�@�C���Ǎ���Buff fpos�̎�O�܂Łh�̐���������
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
  ///�f�[�^�擾
  ///=============================
public:
  void GetNext(shared_ptr<BYTE> &ret_data, shared_ptr<DWORD> &ret_size)
  {
    //���t�@�C���̓Ǎ���������ő�ʒu�@�@buff�̎�O�܂ŁB
    __int64 readable_fpos;

    //Buff
    {//lock scope
      lock_guard<timed_mutex> lock(sync);
      //buff is empty ?
      //  �o�b�t�@�����������Ƃ��ɂ����Ƀt�@�C���Ǎ��݂����Ȃ��悤�ɐ���
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
  ///���t�@�C���Ǎ�
  ///=============================
  //�h�t�@�C���Ǎ���Buff fpos�̎�O�܂Łh�ɐ���
  //Buff fpos�Əd�Ȃ�Ȃ��悤�ɂ���B
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
      //�s�r�����������Ȃ� readable_fpos < 0
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



