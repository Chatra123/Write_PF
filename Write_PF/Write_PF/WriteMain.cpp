#include "StdAfx.h"
#include "WriteMain.h"
#include <process.h>
#include "../../Common/BlockLock.h"

extern HINSTANCE g_instance;


CWriteMain::CWriteMain(void)
{
  this->file = INVALID_HANDLE_VALUE;
  this->writeBuffSize = 0;
  this->teeFile = INVALID_HANDLE_VALUE;
  this->teeThread = NULL;

  //-----  Write_PF  -----
  //read ini
  WCHAR dllPath[MAX_PATH];
  DWORD ret = GetModuleFileName(g_instance, dllPath, MAX_PATH);
  this->setting = make_shared<SettingA>(dllPath);
  this->writeBuffSize = this->setting->Write_Buff;
  this->writeBuff.reserve(this->writeBuffSize);
  //tee無効化
  if (ret && ret < MAX_PATH) {
    //wstring iniPath = wstring(dllPath) + L".ini";
    //this->writeBuffSize = GetPrivateProfileInt(L"SET", L"Size", 770048, iniPath.c_str());
    //this->writeBuff.reserve(this->writeBuffSize);
    //this->teeCmd = GetPrivateProfileToString(L"SET", L"TeeCmd", L"", iniPath.c_str());
    //if( this->teeCmd.empty() == false ){
    //	this->teeBuff.resize(GetPrivateProfileInt(L"SET", L"TeeSize", 770048, iniPath.c_str()));
    //	this->teeBuff.resize(max(this->teeBuff.size(), 1));
    //	this->teeDelay = GetPrivateProfileInt(L"SET", L"TeeDelay", 0, iniPath.c_str());
    //}
  }
  InitializeCriticalSection(&this->wroteLock);
}


CWriteMain::~CWriteMain(void)
{
  Stop();
  DeleteCriticalSection(&this->wroteLock);
}

BOOL CWriteMain::Start(
  LPCWSTR fileName,
  BOOL overWriteFlag,
  ULONGLONG createSize
)
{
  Stop();

  this->savePath = fileName;
  _OutputDebugString(L"★CWriteMain::Start CreateFile:%s\r\n", this->savePath.c_str());
  this->file = _CreateDirectoryAndFile(this->savePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, overWriteFlag ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
  if (this->file == INVALID_HANDLE_VALUE) {
    _OutputDebugString(L"★CWriteMain::Start Err:0x%08X\r\n", GetLastError());
    WCHAR szPath[_MAX_PATH];
    WCHAR szDrive[_MAX_DRIVE];
    WCHAR szDir[_MAX_DIR];
    WCHAR szFname[_MAX_FNAME];
    WCHAR szExt[_MAX_EXT];
    _wsplitpath_s(fileName, szDrive, szDir, szFname, szExt);
    _wmakepath_s(szPath, szDrive, szDir, szFname, NULL);
    for (int i = 1; ; i++) {
      Format(this->savePath, L"%s-(%d)%s", szPath, i, szExt);
      this->file = CreateFile(this->savePath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, overWriteFlag ? CREATE_ALWAYS : CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
      if (this->file != INVALID_HANDLE_VALUE || i >= 999) {
        DWORD err = GetLastError();
        _OutputDebugString(L"★CWriteMain::Start CreateFile:%s\r\n", this->savePath.c_str());
        if (this->file != INVALID_HANDLE_VALUE) {
          break;
        }
        _OutputDebugString(L"★CWriteMain::Start Err:0x%08X\r\n", err);
        this->savePath = L"";
        return FALSE;
      }
    }
  }

  //ディスクに容量を確保
  if (createSize > 0) {
    LARGE_INTEGER stPos;
    stPos.QuadPart = createSize;
    SetFilePointerEx(this->file, stPos, NULL, FILE_BEGIN);
    SetEndOfFile(this->file);
    SetFilePointer(this->file, 0, NULL, FILE_BEGIN);
  }
  this->wrotePos = 0;

  //コマンドに分岐出力
  if (this->teeCmd.empty() == false) {
    this->teeFile = CreateFile(this->savePath.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (this->teeFile != INVALID_HANDLE_VALUE) {
      this->teeThreadStopFlag = FALSE;
      this->teeThread = (HANDLE)_beginthreadex(NULL, 0, TeeThread, this, 0, NULL);
    }
  }

  //-----  Write_PF  -----
#ifdef _DEBUG
  //WriteDllTester_edcb.exeのフォルダパスをカレントに変更
  //  VisualStudioだとプロジェクトフォルダが
  //  カレントになっているので exeフォルダに変更
  PF_Util::SetCurrent_AppFolder();
#endif
  this->setting->TsPath = this->savePath;
  this->server = make_unique<ServerManager>();
  this->server->Init(setting);
  //-----  Write_PF  -----


  return TRUE;
}

BOOL CWriteMain::Stop(
)
{
  if (this->file != INVALID_HANDLE_VALUE) {
    if (this->writeBuff.empty() == false) {
      DWORD write;
      if (WriteFile(this->file, &this->writeBuff.front(), (DWORD)this->writeBuff.size(), &write, NULL) == FALSE) {
        _OutputDebugString(L"★WriteFile Err:0x%08X\r\n", GetLastError());
      }
      else {
        //-----  Write_PF  -----
        //Buffの残りを転送
        this->server->Append(&this->writeBuff.front(), write);     //書き込んだ部分をパイプ転送

        this->writeBuff.erase(this->writeBuff.begin(), this->writeBuff.begin() + write);
        CBlockLock lock(&this->wroteLock);
        this->wrotePos += write;
      }
      //未出力のバッファは再Start()に備えて繰り越す
    }
    SetEndOfFile(this->file);
    CloseHandle(this->file);
    this->file = INVALID_HANDLE_VALUE;
  }
  if (this->teeThread != NULL) {
    this->teeThreadStopFlag = TRUE;
    if (WaitForSingleObject(this->teeThread, 8000) == WAIT_TIMEOUT) {
      TerminateThread(this->teeThread, 0xffffffff);
    }
    CloseHandle(this->teeThread);
    this->teeThread = NULL;
  }
  if (this->teeFile != INVALID_HANDLE_VALUE) {
    CloseHandle(this->teeFile);
    this->teeFile = INVALID_HANDLE_VALUE;
  }

  //-----  Write_PF  -----
  if (this->server != nullptr)
    this->server->Stop();                   //サーバー送信停止

  return TRUE;
}

wstring CWriteMain::GetSavePath(
)
{
  return this->savePath;
}

BOOL CWriteMain::Write(
  BYTE* data,
  DWORD size,
  DWORD* writeSize
)
{
  if (this->file != INVALID_HANDLE_VALUE && data != NULL && size > 0) {
    *writeSize = 0;
    if (this->writeBuff.empty() == false) {
      //できるだけバッファにコピー。コピー済みデータは呼び出し側にとっては「保存済み」となる
      *writeSize = min(size, this->writeBuffSize - (DWORD)this->writeBuff.size());
      this->writeBuff.insert(this->writeBuff.end(), data, data + *writeSize);
      data += *writeSize;
      size -= *writeSize;
      if (this->writeBuff.size() >= this->writeBuffSize) {
        //バッファが埋まったので出力
        DWORD write;
        if (WriteFile(this->file, &this->writeBuff.front(), (DWORD)this->writeBuff.size(), &write, NULL) == FALSE) {
          //-----  Write_PF  -----
          this->server->Stop();                   //サーバー送信停止


          _OutputDebugString(L"★WriteFile Err:0x%08X\r\n", GetLastError());
          SetEndOfFile(this->file);
          CloseHandle(this->file);
          this->file = INVALID_HANDLE_VALUE;
          return FALSE;
        }
        //-----  Write_PF  -----
        this->server->Append(&this->writeBuff.front(), write);     //書き込めた部分をパイプ転送



        this->writeBuff.erase(this->writeBuff.begin(), this->writeBuff.begin() + write);
        CBlockLock lock(&this->wroteLock);
        this->wrotePos += write;
      }
      if (this->writeBuff.empty() == false || size == 0) {
        return TRUE;
      }
    }
    if (size > this->writeBuffSize) {
      //バッファサイズより大きいのでそのまま出力
      DWORD write;
      if (WriteFile(this->file, data, size, &write, NULL) == FALSE) {
        //-----  Write_PF  -----
        this->server->Stop();                   //サーバー送信停止


        _OutputDebugString(L"★WriteFile Err:0x%08X\r\n", GetLastError());
        SetEndOfFile(this->file);
        CloseHandle(this->file);
        this->file = INVALID_HANDLE_VALUE;
        return FALSE;
      }
      //-----  Write_PF  -----
      this->server->Append(data, write);     //書き込めた部分をパイプ転送


      *writeSize += write;
      CBlockLock lock(&this->wroteLock);
      this->wrotePos += write;
    }
    else {
      //バッファにコピー
      *writeSize += size;
      this->writeBuff.insert(this->writeBuff.end(), data, data + size);
    }
    return TRUE;
  }
  return FALSE;
}

UINT WINAPI CWriteMain::TeeThread(LPVOID param)
{
  CWriteMain* sys = (CWriteMain*)param;
  wstring cmd = sys->teeCmd;
  Replace(cmd, L"$FilePath$", sys->savePath);
  vector<WCHAR> cmdBuff(cmd.c_str(), cmd.c_str() + cmd.size() + 1);

  WCHAR szCurrentDir[_MAX_PATH];
  DWORD ret = GetModuleFileName(NULL, szCurrentDir, _MAX_PATH);
  if (ret && ret < _MAX_PATH) {
    //カレントは実行ファイルのあるフォルダ
    WCHAR szDrive[_MAX_DRIVE];
    WCHAR szDir[_MAX_DIR];
    _wsplitpath_s(szCurrentDir, szDrive, _MAX_DRIVE, szDir, _MAX_DIR, NULL, 0, NULL, 0);
    _wmakepath_s(szCurrentDir, szDrive, szDir, NULL, NULL);

    //標準入力にパイプしたプロセスを起動する
    HANDLE tempPipe;
    HANDLE writePipe;
    if (CreatePipe(&tempPipe, &writePipe, NULL, 0)) {
      HANDLE readPipe;
      BOOL bRet = DuplicateHandle(GetCurrentProcess(), tempPipe, GetCurrentProcess(), &readPipe, 0, TRUE, DUPLICATE_SAME_ACCESS);
      CloseHandle(tempPipe);
      if (bRet) {
        SECURITY_ATTRIBUTES sa;
        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = TRUE;
        STARTUPINFO si = {};
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = readPipe;
        //標準(エラー)出力はnulデバイスに捨てる
        si.hStdOutput = CreateFile(L"nul", GENERIC_WRITE, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        si.hStdError = CreateFile(L"nul", GENERIC_WRITE, 0, &sa, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        PROCESS_INFORMATION pi;
        bRet = CreateProcess(NULL, &cmdBuff.front(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, szCurrentDir, &si, &pi);
        CloseHandle(readPipe);
        if (si.hStdOutput != INVALID_HANDLE_VALUE) {
          CloseHandle(si.hStdOutput);
        }
        if (si.hStdError != INVALID_HANDLE_VALUE) {
          CloseHandle(si.hStdError);
        }
        if (bRet) {
          CloseHandle(pi.hThread);
          CloseHandle(pi.hProcess);
          while (sys->teeThreadStopFlag == FALSE) {
            __int64 readablePos;
            {
              CBlockLock lock(&sys->wroteLock);
              readablePos = sys->wrotePos - sys->teeDelay;
            }
            LARGE_INTEGER liPos = {};
            DWORD read;
            if (SetFilePointerEx(sys->teeFile, liPos, &liPos, FILE_CURRENT) &&
              readablePos - liPos.QuadPart >= (__int64)sys->teeBuff.size() &&
              ReadFile(sys->teeFile, &sys->teeBuff.front(), (DWORD)sys->teeBuff.size(), &read, NULL) && read > 0) {
              DWORD write;
              if (WriteFile(writePipe, &sys->teeBuff.front(), read, &write, NULL) == FALSE) {
                break;
              }
            }
            else {
              Sleep(100);
            }
          }
          //プロセスは回収しない(標準入力が閉じられた後にどうするかはプロセスの判断に任せる)
        }
      }
      CloseHandle(writePipe);
    }
  }
  return 0;
}
