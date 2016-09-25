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
  const bool Enable = false;            // true  or  false
#endif

  mutex sync;
  wofstream logfile;
  __int64 TotalAppendTry = 0;          //BuffにAppendを試みたサイズ


  ///=============================
  /// Constructor
  ///=============================
public:
  FileReader_Log(const wstring basepath)
  {
    if (Enable == false) return;

    lock_guard<mutex> lock(sync);
    locale::global(locale("japanese"));  //locale設定しないと日本語が出力されない。
    wstring path = basepath + L".wrpf.log";
    logfile = wofstream(path);
  }
  ~FileReader_Log() { }


  ///=============================
  ///Close
  ///=============================
public:
  void Close()
  {
    if (Enable == false) return;

    WriteLine(L"");
    WriteLine(L"                                                                                  TotalAppendTry = %10llu",
      TotalAppendTry);

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
  void WriteLine(const wchar_t *format, Args const & ... args)
  {
    if (Enable == false) return;

    wstring text;
    {
      wchar_t buf[256];
      _snwprintf(buf, 256, format, args ...);
      text = wstring(buf);
    }

    lock_guard<mutex> lock(sync);
    wcerr << text << endl;
    if (logfile)
      logfile << text << endl;
  }
#pragma warning(default:4996)


  ///=============================
  ///WriteLine
  ///=============================
public:
  void WriteLine(const wstring line)
  {
    WriteLine(line.c_str());
  }


  ///=============================
  ///AppendBuff
  ///=============================
public:
  void AppendBuff(const __int64 fpos, const DWORD size)
  {
    TotalAppendTry += size;
    WriteLine(L"  ++ buff                 fpos = %10llu                           size = %6d    total_try = %10llu",
      fpos, size, TotalAppendTry);
  }


  ///=============================
  ///AppendBuff    Fail to Lock
  ///=============================
public:
  void AppendBuff_FailLock(const __int64 fpos, const DWORD size)
  {
    TotalAppendTry += size;
    WriteLine(L"  .. fail append lock     fpos = %10llu                           size = %6d    total_try = %10llu",
      fpos, size, TotalAppendTry);
  }


  ///=============================
  ///ReleaseBuff
  ///=============================
public:
  void ReleaseBuff(const __int64 fpos, const DWORD size)
  {
    WriteLine(L"  -- buff                 fpos = %10llu                           size = %6d",
      fpos, size);
  }


  ///=============================
  ///GetData_fromBuff
  ///=============================
public:
  void GetData_fromBuff(const __int64 fpos, const DWORD size)
  {
    WriteLine(L"          ( )from Buff      fpos = %10llu                         size = %6d ",
      fpos, size);
  }


  ///=============================
  ///GetData_fromFile
  ///=============================
public:
  void GetData_fromFile(const __int64 fpos, const DWORD size)
  {
    WriteLine(L"           $ from file      fpos = %10llu                         size = %6d",
      fpos, size);
  }


  ///=============================
  ///Seek
  ///=============================
public:
  void Seek_fpos_Read(const __int64 fpos_read, const DWORD size)
  {
    __int64 before = fpos_read - size;
    WriteLine(L"             Seek_Read      fpos = %10llu    next = %10llu    size = %6d",
      before, fpos_read, size);
  }



};




