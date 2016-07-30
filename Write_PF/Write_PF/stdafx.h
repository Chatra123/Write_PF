// stdafx.h : 標準のシステム インクルード ファイルのインクルード ファイル、または
// 参照回数が多く、かつあまり変更されない、プロジェクト専用のインクルード ファイル
// を記述します。
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Windows ヘッダーから使用されていない部分を除外します。
// Windows ヘッダー ファイル:
#include <windows.h>



// TODO: プログラムに必要な追加ヘッダーをここで参照してください。
#include "../../Common/Common.h"


//------  Write_PF  ------
#include <iostream>                //  wstring    cout << "hello" << endl;
#include <fstream>                 //  wofstream  Log
#include <thread>
#include <deque>
#include <memory>                   // shared_ptr
#include <mutex>                    // lock
#include <regex>
#include <rpc.h>                    // UUID
#pragma comment(lib ,"rpcrt4.lib")  // UUID

using namespace std;



