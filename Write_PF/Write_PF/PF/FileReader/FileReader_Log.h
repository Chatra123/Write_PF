#pragma once
#include "../../stdafx.h"

///=============================
/// FileReaderのデータ入出ログ
///=============================
class FileReader_Log
{
#ifdef NDEBUG  // Release
  const bool Enable = false;       // fix false
#else
  const bool Enable = false;       // true  or  false
#endif

  mutex sync;
  wofstream logfile;

  __int64 TotalAppendTry = 0;      //BuffにAppendを試みたサイズ　＜成功、失敗、ロック失敗の合計＞
 

  ///=============================
  /// Constructor
  ///=============================
public:
  FileReader_Log(const wstring basepath)
  {
    if (Enable == false) return;

    wstring logpath = basepath + L".wrpf.log";
    //logpath = L"E:\\pf_out\\test.wrpf.log";

    lock_guard<mutex> lock(sync);
    locale::global(locale("japanese"));  //locale設定しないと日本語が出力されない。
    logfile = wofstream(logpath);
  }
  ~FileReader_Log(){ }


  ///=============================
  ///Close
  ///=============================
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
  void WriteLine(const wstring line)
  {
    WriteLine(line.c_str());
  }


  ///=============================
  ///AppendBuff
  ///=============================
  void AppendBuff(const __int64 fpos, const DWORD size)
  {
    TotalAppendTry += size;

    WriteLine(L"  ++ append  buff         fpos = %10llu                           size = %6d    total_try = %10llu",
      fpos, size, TotalAppendTry);
  }


  ///=============================
  ///AppendBuff    Fail to append  
  ///=============================
  void AppendBuff_FailAppend(const __int64 fpos, const DWORD size)
  {
    TotalAppendTry += size;

    WriteLine(L"++.. fail append buff     fpos = %10llu                           size = %6d    total_try = %10llu",
      fpos, size, TotalAppendTry);
  }


  ///=============================
  ///AppendBuff    Fail to Lock
  ///=============================
  void AppendBuff_FailLock(const __int64 fpos, const DWORD size)
  {
    TotalAppendTry += size;

    WriteLine(L"  .. fail append lock     fpos = %10llu                           size = %6d    total_try = %10llu",
      fpos, size, TotalAppendTry);
  }


  ///=============================
  ///ReleaseBuff
  ///=============================
  void ReleaseBuff(const __int64 fpos, const DWORD size)
  {
    WriteLine(L"  -- release buff         fpos = %10llu                           size = %6d",
      fpos, size);
  }


  ///=============================
  ///GetData_fromBuff
  ///=============================
  void GetData_fromBuff(const __int64 fpos, const DWORD size)
  {
    WriteLine(L"          ( )from Buff      fpos = %10llu                         size = %6d ",
      fpos, size);
  }


  ///=============================
  ///GetData_fromFile
  ///=============================
  void GetData_fromFile(const __int64 fpos, const DWORD size)
  {
    WriteLine(L"           $ from file      fpos = %10llu                         size = %6d",
      fpos, size);
  }


  ///=============================
  ///Seek
  ///=============================
  void Seek_Read_fpos(const __int64 fpos_read_before, const __int64 fpos_read, const DWORD  size)
  {
    WriteLine(L"             Seek_Read      fpos = %10llu    next = %10llu    size = %6d",
      fpos_read_before, fpos_read, size);
  }





};




