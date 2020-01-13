#pragma once
#include <windows.h>


//
//		    
//			内存获取ini配置，模块内并定时同步ini文件到内存中	 ,优点:
//			1、减少io操作，提升性能； 	 比windows 函数提升20倍
//          2、避免其他进程在修改ini文件时和windows系统获取配置的GetPrivateProfileInt等函数文件冲突，导致ini 文件被清零
//			
//			程序启动时会备份一个.memIni文件

namespace memIni
{

	//************************************
	// Method:    Init		   （必须调用)
	// FullName:  memIni::Init
	// Access:    public 
	// Returns:   BOOL
	// Qualifier: 配置文件中存在异常（比如相同的sectionname，相同的key）
	// Parameter: INT nWriteInterval	 默认的同步时间
	// Parameter: LPCSTR lpDingDing
	//************************************
	bool Init(INT nWriteInterval = 5);

	//************************************
	// Method:    UnInit
	// FullName:  memIni::UnInit
	// Access:    public 
	// Returns:   VOID
	// Qualifier:  关闭时调用
	//************************************
	VOID UnInit();

	/* 使用方式同windows ，见msdn 
	 * 如下函数，默认自动会读取 lpFileName 的配置文件
	 */
	UINT GetPrivateProfileInt(LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName);
	DWORD GetPrivateProfileString(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
	DWORD GetPrivateProfileSection(LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
	DWORD GetPrivateProfileSectionNames(LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
	BOOL GetPrivateProfileStruct(LPCSTR lpAppName, LPCSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCSTR lpFileName);

	BOOL WritePrivateProfileString(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName);
	BOOL WritePrivateProfileSection(LPCSTR lpAppName, LPCSTR lpString, LPCSTR lpFileName);
	BOOL WritePrivateProfileStruct(LPCSTR lpAppName, LPCSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCSTR lpFileName);

	
	/*保留接口，非常特殊特殊情况使用*/
	DWORD GetPrivateProfile(LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
	BOOL  WritePrivateProfile(LPCSTR lpString, LPCSTR lpFileName);
	DWORD GetPrivateProfileSectionKeyNames(LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
};

/*  demo
int _tmain()
{
  	  memIni::Init();

	  std::string strIni = "c://a.ini";
	  int nPort = memIni::GetPrivateProfileInt("checksvr","port","30623",strIni.c_str());

	  memIni::Unit();
	  return 0;
}  
*/

