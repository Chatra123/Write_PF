#pragma once
#include "../../stdafx.h"
#include "PipeWriter.h"


///=============================
///�@Named Pipe         non-blocking I/O
///=============================
class NamedPipe : public PipeWriter
{
private:
  wstring PipeName;
  HANDLE hQuitOrder;
  HANDLE hWritePipe;

public:
  NamedPipe(wstring pipename, HANDLE hquit)
  {
    PipeName = pipename;
    hQuitOrder = hquit;
    hWritePipe = INVALID_HANDLE_VALUE;
  }
  ~NamedPipe(){}



  ///=============================
  ///�@�N���C�A���g�N��
  ///=============================
public:
  void RunClient(wstring command, bool hideClient)
  {
    DWORD dwCreationFlags = hideClient
      ? NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW
      : NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE;

    STARTUPINFO si = {};
    PROCESS_INFORMATION pi = {};
    si.cb = sizeof(si);

    BOOL ret = CreateProcess(
      NULL,                          //���s�\���W���[���̖��O
      (LPWSTR)command.c_str(),       //�R�}���h���C���̕�����
      NULL,                          //�Z�L�����e�B�L�q�q
      NULL,                          //�Z�L�����e�B�L�q�q
      FALSE,                         //�n���h���̌p���I�v�V����
      dwCreationFlags,               //�쐬�̃t���O
      NULL,                          //�V�������u���b�N
      NULL,                          //�J�����g�f�B���N�g���̖��O
      &si,                           //�X�^�[�g�A�b�v���
      &pi);                          //�v���Z�X���

    if (ret) {
      CloseHandle(pi.hThread);
      CloseHandle(pi.hProcess);
    }
  }



  ///=============================
  ///�@�p�C�v�ڑ�
  ///=============================
public:
  bool Connect()
  {
    //�쐬
    wstring fullname = L"\\\\.\\pipe\\" + this->PipeName;
    hWritePipe = CreateNamedPipe(
      fullname.c_str(),                             //�p�C�v��
      PIPE_ACCESS_OUTBOUND | FILE_FLAG_OVERLAPPED,  //�p�C�v���J�����[�h
      PIPE_TYPE_BYTE,                               //�p�C�v�ŗL�̃��[�h
      1,                                            //�C���X�^���X�̍ő吔
      1024 * 128,                                   //�o�̓o�b�t�@�̃T�C�Y
      0,                                            //���̓o�b�t�@�̃T�C�Y
      1000,                                         //�K��̐ڑ��^�C���A�E�g
      NULL);                                        //�Z�L�����e�B�L�q�q
    if (hWritePipe == INVALID_HANDLE_VALUE) return false;


    //�ڑ�
    HANDLE hPipeEvent = INVALID_HANDLE_VALUE;
    hPipeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hPipeEvent == INVALID_HANDLE_VALUE) return false;

    OVERLAPPED ovl = {};
    ovl.hEvent = hPipeEvent;


    bool connect;
    if (ConnectNamedPipe(hWritePipe, &ovl))
    {
      // Overlapped ConnectNamedPipe should return zero. 
      connect = false;
    }
    else
    {
      DWORD lastErr = GetLastError();
      if (lastErr == ERROR_PIPE_CONNECTED)
      {
        // success
        //   client connects before the function is called.
        //   there is a good connection between client and server.
        connect = true;
      }
      else if (lastErr == ERROR_IO_PENDING)
      {
        //�ҋ@
        if (WaitEvent(hPipeEvent, hQuitOrder))
        {
          DWORD lpTransferred;
          if (GetOverlappedResult(hWritePipe, &ovl, &lpTransferred, FALSE))
          {
            // success
            connect = true;
          }
          else
          {
            connect = false;
          }
        }
        else//hQuitOrder event is fired
          connect = false;
      }
      else// If an error occurs during the connect operation... 
        connect = false;
    }

    if (hPipeEvent != INVALID_HANDLE_VALUE)
    {
      CloseHandle(hPipeEvent);
      hPipeEvent = INVALID_HANDLE_VALUE;
    }
    return connect;
  }



  ///=============================
  ///�@�p�C�v��������
  ///=============================
public:
  BOOL Write(shared_ptr<BYTE> &data, shared_ptr<DWORD> &size, DWORD &written)
  {
    HANDLE hPipeEvent;
    hPipeEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (hPipeEvent == INVALID_HANDLE_VALUE)
      return FALSE;

    OVERLAPPED ovl = {};
    ovl.hEvent = hPipeEvent;

    //��
    BOOL success = WriteFile(hWritePipe, data.get(), *size, &written, &ovl);
    if (success)
    {
      //success
    }
    else
    {
      //�ҋ@
      if (WaitEvent(hPipeEvent, hQuitOrder))
      {
        success = GetOverlappedResult(hWritePipe, &ovl, &written, FALSE);
      }
      else
      {
        //hQuitOrder event is fired
        success = FALSE;
      }
    }

    if (hPipeEvent != INVALID_HANDLE_VALUE)
    {
      CloseHandle(hPipeEvent);
      hPipeEvent = INVALID_HANDLE_VALUE;
    }
    return success;
  }



  ///=============================
  ///�@���M�I��
  ///=============================
public:
  void Close()
  {
    if (hWritePipe != INVALID_HANDLE_VALUE)
    {
      FlushFileBuffers(hWritePipe);
      Sleep(300);

      DisconnectNamedPipe(hWritePipe);
      CloseHandle(hWritePipe);
      hWritePipe = INVALID_HANDLE_VALUE;
    }
  }



  ///=============================
  ///�@�C�x���g�ҋ@
  ///=============================
  // some event are fired
  //   hEvent               -->  return true;
  //   hQuitEvent           -->  return false;
  //   hQuitEvent & hEvent  -->  return false;
private:
  bool WaitEvent(HANDLE hEvent, HANDLE hQuitEvent)
  {
    HANDLE hWaits[2] = { hQuitEvent, hEvent };  //hQuitEvent���D��

    //WaitForMultipleObjects
    //  �����̃I�u�W�F�N�g���V�O�i����ԂɂȂ����ꍇ�́A�ŏ��̃C���f�b�N�X�ԍ����Ԃ�܂��B
    //  hEvent               -->  ret = 1;
    //  hQuitEvent           -->  ret = 0;
    //  hQuitEvent & hEvent  -->  ret = 0;
    DWORD ret = WaitForMultipleObjects(2, hWaits, FALSE, INFINITE);
    return ret == WAIT_OBJECT_0 + 1;
  }


  
};



