#pragma once
#include "../../stdafx.h"

///=============================
/// FileReaderのデータ入出ログ
///=============================
class FileReader_Log
{
#ifdef NDEBUG  // Release
  const bool Enable = false;           // fix false
#else
  const bool Enable = false;           // true  or  false
#endif

  mutex sync;
  wofstream logfile;
  __int64 TotalAppendTry = 0;          //BuffにAppendを試みたサイズ


///=============================
/// Constructor
///=============================
public:
  FileReader_Log(const wstring path)
  {
    if (Enable == false) return;

    lock_guard<mutex> lock(sync);
    locale::global(locale("japanese"));  //locale設定しないと日本語が出力されない。
    logfile = wofstream(path);
  }
  ~FileReader_Log() { Close(); }

  ///=============================
  ///Close
  ///=============================
private:
  void Close()
  {
    WriteLine(L"");
    WriteLine(close, std::this_thread::get_id(), TotalAppendTry);
    lock_guard<mutex> lock(sync);
    if (logfile)
      logfile.close();
  }

  ///=============================
  /// WriteLine
  ///=============================
#pragma warning(disable:4996)   //_snwprintf
public:
  template <typename ... Args>
  void WriteLine(const wstring format, Args const & ... args)
  {
    if (Enable == false) return;
    wstring line;
    {
      wchar_t buf[256];
      _snwprintf(buf, 256, format.c_str(), args ...);
      line = wstring(buf);
    }
    lock_guard<mutex> lock(sync);
    if (wcerr)
      wcerr << line << endl;
    if (logfile)
      logfile << line << endl;
  }
#pragma warning(default:4996)


  ///=============================
  ///log text
  ///=============================
  const wstring appe = L" %08x    ++ buff           fpos = %10llu                           size = %6d    total_try = %10llu";
  const wstring fail = L" %08x    .. fail           fpos = %10llu                           size = %6d    total_try = %10llu";
  const wstring rele = L" %08x    -- buff           fpos = %10llu                           size = %6d";
  const wstring fromBuff = L" %08x    ( )from Buff      fpos = %10llu                           size = %6d ";
  const wstring fromFile = L" %08x     $ from file      fpos = %10llu                           size = %6d";
  const wstring seek = L" %08x       Seek_Read        fpos = %10llu    next = %10llu    size = %6d";
  const wstring close = L" %08x                                                                     TotalAppendTry = %10llu";


  ///=============================
  ///AppendBuff
  ///=============================
public:
  void AppendBuff(const __int64 fpos, const DWORD size)
  {
    TotalAppendTry += size;
    WriteLine(appe, std::this_thread::get_id(), fpos, size, TotalAppendTry);
  }

  ///=============================
  ///FailLock
  ///=============================
public:
  void FailLock(const __int64 fpos, const DWORD size)
  {
    TotalAppendTry += size;
    WriteLine(fail, std::this_thread::get_id(), fpos, size, TotalAppendTry);
  }

  ///=============================
  ///ReleaseBuff
  ///=============================
public:
  void ReleaseBuff(const __int64 fpos, const DWORD size)
  {
    WriteLine(rele, std::this_thread::get_id(), fpos, size);
  }

  ///=============================
  ///GetData_fromBuff
  ///=============================
public:
  void GetData_fromBuff(const __int64 fpos, const DWORD size)
  {
    WriteLine(fromBuff, std::this_thread::get_id(), fpos, size);
  }

  ///=============================
  ///GetData_fromFile
  ///=============================
public:
  void GetData_fromFile(const __int64 fpos, const DWORD size)
  {
    WriteLine(fromFile, std::this_thread::get_id(), fpos, size);
  }

  ///=============================
  ///Seek
  ///=============================
public:
  void Seek(const __int64 fpos_read, const DWORD size)
  {
    __int64 before = fpos_read - size;
    WriteLine(seek, std::this_thread::get_id(), before, fpos_read, size);
  }

};




