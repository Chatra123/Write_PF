#pragma once
#include "../../stdafx.h"


class PipeWriter {
protected:
  PipeWriter() {}
public:
  virtual ~PipeWriter() {}
public:
  virtual void RunClient(wstring command, bool hide) = 0;
  virtual bool Connect() = 0;
  virtual BOOL Write(shared_ptr<BYTE> &data, shared_ptr<DWORD> &size, DWORD &written) = 0;
  virtual void Close() = 0;
};




