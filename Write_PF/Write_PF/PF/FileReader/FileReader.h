/*
  ���z�̂s�r�t�@�C������Ǎ�
    �i �s�r�����p�f�[�^�Ǝ��t�@�C�� �j
    �i StreamBuff        ifstream   �j

*/
#pragma once
#include "../../stdafx.h"
#include "StreamBuff.h"
#include "FileReader_Log.h"



///=============================
///�t�@�C���Ǎ�
///=============================
class FileReader
{
private:
  timed_mutex sync;
  unique_ptr<StreamBuff> buff;
  ifstream ifile;

  __int64 fpos_read = 0;               //�t�@�C���Ǎ��݈ʒu
  __int64 fpos_write = 0;              //���t�@�C�������݂̐�[�ʒu
  bool TS_FinishWrite = false;

  shared_ptr<FileReader_Log> log;


  ///=============================
  /// Constructor
  ///=============================
public:
  FileReader(wstring filepath, int streamBuffSize)
  {
    log = make_shared<FileReader_Log>(filepath);

    //file
    this->ifile.open(filepath, ios_base::in | ios_base::binary, _SH_DENYNO);
    //StreamBuff
    this->buff = make_unique<StreamBuff>(streamBuffSize, log);
  }
  ~FileReader() { }

  void Close()
  {
    lock_guard<timed_mutex> lock(sync);
    log->Close();
    if (ifile)
      ifile.close();
  }



  ///=============================
  ///�o�b�t�@�Ƀf�[�^�ǉ�
  ///=============================
public:
  void AppendBuff(BYTE* data, const DWORD size)
  {
    if (size == 0) return;

    //���C���X���b�h����Ă΂��̂� timeout
    unique_lock<timed_mutex> lock(sync, chrono::milliseconds(0));
    if (lock)
    {
      //copy data
      auto append_fpos = make_shared<__int64>(fpos_write);
      shared_ptr<BYTE> append_data(new BYTE[size]);
      auto append_size = make_shared<DWORD>(size);
      memcpy(append_data.get(), data, size);

      buff->AppendData(append_fpos, append_data, append_size);
      fpos_write += size;
    }
    else
    {
      log->AppendBuff_FailLock(fpos_write, size);
      fpos_write += size;
    }
  }


  ///=============================
  ///�s�r�t�@�C�������݂̊����ʒm���󂯎��
  ///=============================
public:
  void Notify_TSFinishWrite()
  {
    //�h�o�b�t�@�O�̂Ƃ��Ƀt�@�C���Ǎ��݋֎~�h������
    TS_FinishWrite = true;
  }


  ///=============================
  ///Is_EOF
  ///=============================
public:
  bool IsEOF()
  {
    if (ifile)
      return ifile.fail() || ifile.eof();
    else
      return true;
  }


  ///=============================
  ///�f�[�^�擾
  ///=============================
public:
  void GetNext(shared_ptr<BYTE> &retData, shared_ptr<DWORD> &retSize)
  {
    if (!ifile) return;

    __int64 readable_fpos;  //�t�@�C���Ǎ��\�ȍő�ʒu

    //Stream Buff
    {//lock scope
      lock_guard<timed_mutex> lock(sync);

      // buff is empty ?
      //�@�o�b�t�@�����������Ƃ��ɂ����Ƀt�@�C���Ǎ��݂����Ȃ��悤�ɐ���
      bool buffIsEmpty = buff->GetTopPos() < 0;
      if (buffIsEmpty && TS_FinishWrite == false)
        return;                                //�o�b�t�@�����܂�܂� return sleep()
      readable_fpos = TS_FinishWrite ? -1 : buff->GetTopPos() - 1;

      buff->GetData(fpos_read, retData, retSize);
      if (retData != nullptr)
      {
        //success read
        log->GetData_fromBuff(fpos_read, *retSize);
        return;
      }
    }//lock scope


    //File
    GetData_fromFile(fpos_read, readable_fpos, retData, retSize);
    if (retData != nullptr)
    {
      //success read
      log->GetData_fromFile(fpos_read, *retSize);
      return;
    }

    return;
  }



  ///=============================
  ///�f�[�^�擾�@�t�@�C���Ǎ�
  ///=============================
  //  �t�@�C���ǂݍ��݂� StreamBuff���̃f�[�^�ʒu�Əd�Ȃ�Ȃ��悤�ɂ���B
  //  ��� StreamBuff fpos�̎�O�łƂ߂�B
private:
  void GetData_fromFile(
    const __int64 req_fpos, const __int64 readable_fpos,
    shared_ptr<BYTE> &retData, shared_ptr<DWORD> &retSize)
  {
    if (ifile.fail() || ifile.eof())
      return;

    const UINT ReadMax = 770048;
    /*
      �t�@�C������ ReadMax [byte] �Ǎ��ނ��т�100ms��Sleep������
      �Ă���̂ōő呬�x�͐��������B
      ReadMax������������̂̓_���B�^�摬�x�ȉ����ƃo�b�t�@��
      �ǂ����Ȃ��Ȃ�B
        ( 770,048 / 1 ) [byte/times]  *  10 [times/sec]  =  7.7 [MB/sec]
        ( 770,048 / 2 ) [byte/times]  *  10 [times/sec]  =  3.9 [MB/sec]
    */

    __int64 req_size;
    {
      //req_size��StreamBuff�̎�O�܂ŁB
      //�s�r�����������Ȃ� readable_fpos < 0
      if (0 <= readable_fpos
        && readable_fpos < req_fpos + ReadMax)
        req_size = readable_fpos - req_fpos + 1;
      else
        req_size = ReadMax;
    }
    if (req_size <= 0)
      return;

    shared_ptr<BYTE> data(new BYTE[ReadMax]);
    ifile.seekg(req_fpos, ios_base::beg);
    __int64 gcount = ifile.read((char *)data.get(), sizeof(char) * req_size).gcount();
    auto size = make_shared<DWORD>((DWORD)gcount);
    if (gcount == 0)
      return;

    retData = data;
    retSize = size;
  }


  ///=============================
  ///Seek
  ///=============================
public:
  void Seek_Read_fpos(const DWORD size)
  {
    fpos_read += size;

    __int64 fpos_read_before = fpos_read - size;
    log->Seek_Read_fpos(fpos_read_before, fpos_read, size);
  }



};

















