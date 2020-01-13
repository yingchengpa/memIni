#pragma once
#include <windows.h>


//
//		    
//			�ڴ��ȡini���ã�ģ���ڲ���ʱͬ��ini�ļ����ڴ���	 ,�ŵ�:
//			1������io�������������ܣ� 	 ��windows ��������20��
//          2�����������������޸�ini�ļ�ʱ��windowsϵͳ��ȡ���õ�GetPrivateProfileInt�Ⱥ����ļ���ͻ������ini �ļ�������
//			
//			��������ʱ�ᱸ��һ��.memIni�ļ�

namespace memIni
{

	//************************************
	// Method:    Init		   ���������)
	// FullName:  memIni::Init
	// Access:    public 
	// Returns:   BOOL
	// Qualifier: �����ļ��д����쳣��������ͬ��sectionname����ͬ��key��
	// Parameter: INT nWriteInterval	 Ĭ�ϵ�ͬ��ʱ��
	// Parameter: LPCSTR lpDingDing
	//************************************
	bool Init(INT nWriteInterval = 5);

	//************************************
	// Method:    UnInit
	// FullName:  memIni::UnInit
	// Access:    public 
	// Returns:   VOID
	// Qualifier:  �ر�ʱ����
	//************************************
	VOID UnInit();

	/* ʹ�÷�ʽͬwindows ����msdn 
	 * ���º�����Ĭ���Զ����ȡ lpFileName �������ļ�
	 */
	UINT GetPrivateProfileInt(LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName);
	DWORD GetPrivateProfileString(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
	DWORD GetPrivateProfileSection(LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
	DWORD GetPrivateProfileSectionNames(LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName);
	BOOL GetPrivateProfileStruct(LPCSTR lpAppName, LPCSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCSTR lpFileName);

	BOOL WritePrivateProfileString(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName);
	BOOL WritePrivateProfileSection(LPCSTR lpAppName, LPCSTR lpString, LPCSTR lpFileName);
	BOOL WritePrivateProfileStruct(LPCSTR lpAppName, LPCSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCSTR lpFileName);

	
	/*�����ӿڣ��ǳ������������ʹ��*/
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

