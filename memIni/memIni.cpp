#pragma once

#include <memory>
#include <thread>
#include <future>
#include <map>
#include <sstream>
#include <filesystem>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/ini_parser.hpp>
#include <boost/asio.hpp>

#include <tchar.h>
#include "memIni.h"

#if defined(_MSC_VER)
#	define memIni_TLS __declspec(thread)
#elif defined(__GNUC__)
#	define memIni_TLS __thread
#else
#	define memIni_TLS thread_local
#endif

namespace memIni 
{

	class ini_info 
	{
	public:
		enum {
			loaded,
			modified,
			failed
		} flag;
		time_t timestamp;
		boost::property_tree::iptree pt;

		ini_info(const ini_info* p) :flag(modified), timestamp(time(nullptr))
		{
			if (p) {
				timestamp = p->timestamp;
				pt = p->pt;
			}
		}
	};

	typedef std::map<std::string, std::shared_ptr<ini_info>> ini_map;

	static std::atomic<BOOL> g_bInited = FALSE;
	static std::shared_ptr<const ini_map> g_pIniMap;
	static std::thread g_tWrite;
	static std::shared_ptr<boost::asio::io_service> g_pIniService;
	static std::shared_ptr<boost::asio::io_service::work> g_pIniWork;
	//static std::string g_sDingDing;
	static LPERRCALLBACK g_lpErrCallBack = nullptr;

	#define ini_name(s) boost::property_tree::path_of<std::string>::type(s, '\0')

	DWORD CopyString(const std::string& s, LPSTR lpReturnedString, DWORD nSize)
	{
		if (s.length() > nSize) {

			std::copy(s.begin(), s.begin() + nSize, lpReturnedString);
			
			for (auto itor = s.rbegin(); nSize > 0 && *itor == '\0'; itor++) {
				lpReturnedString[--nSize] = '\0';
			}
			return nSize;
		}
		else {
			std::copy(s.begin(), s.end(), lpReturnedString);
			return s.length();
		}
	}

	BOOL ReadIniFile(std::string& str, const std::string& file)
	{
		auto h = std::unique_ptr<std::remove_pointer_t<HANDLE>, void(*)(HANDLE)>(INVALID_HANDLE_VALUE,
			[](HANDLE h) {
				if (h != INVALID_HANDLE_VALUE) {
					CloseHandle(h);
				}
			}
		);

		do {
			h.reset(CreateFile(file.c_str(), GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
		} while (h.get() == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION && (Sleep(1), TRUE));

		if (h.get() == INVALID_HANDLE_VALUE) {
			return FALSE;
		}

		auto l = GetFileSize(h.get(), nullptr);
		auto p = std::make_unique<char[]>(l);
		DWORD n = 0;
		if (!ReadFile(h.get(), p.get(), l, &n, nullptr)) {
			return FALSE;
		}
		if (l != n) {
			return FALSE;
		}

		str.assign(p.get(), p.get() + l);
		return TRUE;
	}

	BOOL BackupIniFile(const std::string& file, time_t timestamp)
	{
		auto path = std::filesystem::path(file);
		//auto bak_path = std::tr2::sys::path(strprintf("%s.bak.%I64d", file, (int64_t)timestamp));
		auto bak_path = std::filesystem::path(file + ".bak");
		std::filesystem::copy_file(path, bak_path, std::filesystem::copy_options::overwrite_existing);
		return TRUE;
	}

	BOOL WriteIniFile(const std::string& str, const std::string& file) 
	{
		auto h = std::unique_ptr<std::remove_pointer_t<HANDLE>, void(*)(HANDLE)>(INVALID_HANDLE_VALUE,
			[](HANDLE h) {
				if (h != INVALID_HANDLE_VALUE) {
					CloseHandle(h);
				}
			}
		);
	
		do {
			h.reset(CreateFile(file.c_str(), GENERIC_WRITE, FILE_SHARE_WRITE, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0));
		} while (h.get() == INVALID_HANDLE_VALUE && GetLastError() == ERROR_SHARING_VIOLATION && (Sleep(1), TRUE));

		if (h.get() == INVALID_HANDLE_VALUE) {
			return FALSE;
		}

		DWORD n = 0;
		if (!WriteFile(h.get(), str.c_str(), str.length(), &n, nullptr)) {
			return FALSE;
		}
		if (str.length() != n) {
			return FALSE;
		}

		return TRUE;
	}

	BOOL ReadIni(const char* func, const char* filename, const std::string& str, boost::property_tree::iptree& pt)
	{
		try {
			std::istringstream sin;
			sin.str(str);
			boost::property_tree::ini_parser::read_ini(sin, pt);
		}
		catch (boost::property_tree::ini_parser_error & e) {
			if (g_lpErrCallBack) {
				g_lpErrCallBack(func, e.message().c_str(), filename, e.line());
			}
			return FALSE;
		}
		return TRUE;
	}

	BOOL WriteIni(const char* func, const char* filename, std::string& str, const boost::property_tree::iptree& pt)
	{
		try {
			std::stringstream sout;
			boost::property_tree::ini_parser::write_ini(sout, pt);
#if defined(_MSC_VER)
			{
				std::stringstream sout2;
				std::string l;
				while (std::getline(sout, l)) {
					sout2 << l << '\r' << std::endl;
				}
				str = sout2.str();
			}
#else
			str = sout.str();
#endif
		}
		catch (boost::property_tree::ini_parser_error & e) {
			//auto msg = strprintf("%s catch error! msg:%s, file:%s, line:%d", func, e.message(), filename, e.line());
			//LOG_ERROR("%s", msg);
			//SendDingDing(msg);
			if (g_lpErrCallBack) {
				g_lpErrCallBack(func, e.message().c_str(), filename, e.line());
			}
			return FALSE;
		}
		return TRUE;
	}


	BOOL TaskWrapper(LPCTSTR file, std::function<BOOL(ini_info&)> func)
	{
		auto m = std::make_unique<ini_map>(*g_pIniMap);
		auto& p = (*m)[file];
		p = std::make_shared<ini_info>(p.get());

		auto ret = func(*p);

		if (p->flag != ini_info::failed) {
			std::atomic_store(&g_pIniMap, std::shared_ptr<const ini_map>(m.release()));
		}
		return ret;
	}

	BOOL SyncInvoke(std::function<BOOL()> func)
	{
		if (std::this_thread::get_id() == g_tWrite.get_id()) {
			return func();
		}
		else {
			std::packaged_task<BOOL()> task(func);
			std::future<BOOL> ret = task.get_future();

			g_pIniService->post([&]() { task(); });
			return ret.get();
		}
	}

	BOOL LoadPrivateProfile(LPCSTR lpFileName)
	{
		auto func = [&](ini_info& info)
		{
			auto path = std::filesystem::path(lpFileName);
			info.timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::last_write_time(path).time_since_epoch()).count();

			info.flag = ini_info::failed;

			std::string s;
			if (!ReadIniFile(s, lpFileName)) {
				return FALSE;
			}

			if (!ReadIni(__FUNCTION__, lpFileName, s, info.pt)) {
				return FALSE;
			}

			info.flag = ini_info::loaded;

			(void)BackupIniFile(lpFileName, info.timestamp);
			return TRUE;
		};

		return SyncInvoke(std::bind(TaskWrapper, lpFileName, func));
	}

	BOOL ReviseFileName(LPCSTR& lpFileName)
	{
		static memIni_TLS TCHAR szWinIni[MAX_PATH] = {};
	
		if (lpFileName) {
			if (lpFileName[0] == '\0') {
				return FALSE;
			}

			if (std::filesystem::path(lpFileName).is_relative()) {
				GetWindowsDirectory(szWinIni, MAX_PATH);
				_tcscat(szWinIni, "\\");
				lpFileName = _tcscat(szWinIni, lpFileName);
			}
		}
		else {
			GetWindowsDirectory(szWinIni, MAX_PATH);
			_tcscat(szWinIni, "\\");
			lpFileName = _tcscat(szWinIni, "win.ini");
		}

		auto m = std::atomic_load(&g_pIniMap);
		if (m->find(lpFileName) == m->end()) {
			(void)LoadPrivateProfile(lpFileName);
		}
		return TRUE;
	}

	std::shared_ptr<const ini_info> GetPrivateProfilePtr(LPCSTR& lpFileName)
	{
		if (!ReviseFileName(lpFileName)) {
			return nullptr;
		}

		auto m = std::atomic_load(&g_pIniMap);
		auto itor = m->find(lpFileName);
		if (itor == m->end()) {
			return nullptr;
		}

		return itor->second;
	}

	void StartTimer(int sec, std::function<void()> func) 
	{
		struct handler_timer {
			static void func(std::shared_ptr<boost::asio::deadline_timer> timer, int sec, std::function<void()> func, const boost::system::error_code& e) {
				if (e) {
					return;
				}
				func();
				timer->expires_from_now(boost::posix_time::seconds(sec));
				timer->async_wait(boost::bind(&handler_timer::func, timer, sec, func, boost::asio::placeholders::error));
			}
		};
		auto timer = std::make_shared<boost::asio::deadline_timer>(*g_pIniService, boost::posix_time::seconds(sec));
		timer->async_wait(boost::bind(&handler_timer::func, timer, sec, func, boost::asio::placeholders::error));
	}

	void OnTimer()
	{
		auto m = std::make_unique<ini_map>(*g_pIniMap);

		for (auto& p : *m)
		{
			auto path = std::filesystem::path(p.first);
			auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(std::filesystem::last_write_time(path).time_since_epoch()).count();
			if (timestamp && p.second->timestamp != timestamp) {
				LoadPrivateProfile(p.first.c_str());
			}
			else if (p.second->flag == ini_info::modified)
			{
				std::string s;
				if (!WriteIni(__FUNCTION__, p.first.c_str(), s, p.second->pt)) {
					continue;
				}

				if (!WriteIniFile(s, p.first)) {
					continue;
				}

				p.second->flag = ini_info::loaded;
			}
		}
	}


	BOOL Init(INT nWriteInterval, /*LPCSTR lpDingDing,*/ LPERRCALLBACK lpErrCallBack)
	{
		if (!g_bInited.exchange(TRUE))
		{
			/*
			if (lpDingDing) {
				g_sDingDing = lpDingDing;
			}
			*/
			
			g_lpErrCallBack = lpErrCallBack;

			g_pIniMap = std::make_shared<ini_map>();

			g_pIniService = std::make_shared<boost::asio::io_service>();
			g_pIniWork = std::make_shared<boost::asio::io_service::work>(*g_pIniService);

			g_tWrite = std::thread([]() { g_pIniService->run(); });

			if (nWriteInterval <= 0) {
				nWriteInterval = 1;
			}
			StartTimer(nWriteInterval, OnTimer);
		}
		return TRUE;
	}

	VOID UnInit()
	{
		if (g_bInited.exchange(FALSE))
		{
			SyncInvoke([]() { OnTimer(); return TRUE; });

			g_pIniService->stop();

			if (g_tWrite.joinable()) {
				g_tWrite.join();
			}

			g_pIniWork.reset();
			g_pIniService.reset();
		}
	}


	BOOL IsInited() {
		return g_bInited;
	}

	UINT GetPrivateProfileInt(LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			return ::GetPrivateProfileInt(lpAppName, lpKeyName, nDefault, lpFileName);
		}

		if (!lpAppName) {
			return nDefault;
		}

		auto pf = GetPrivateProfilePtr(lpFileName);
		if (!pf) {
			return nDefault;
		}

		auto& pt = pf->pt;
		if (pt.find(lpAppName) != pt.not_found()) {
			nDefault = pt.get_child(ini_name(lpAppName)).get<INT>(ini_name(lpKeyName), nDefault);
		}
		return nDefault;
	}
	DWORD GetPrivateProfileString(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			return ::GetPrivateProfileString(lpAppName, lpKeyName, lpDefault, lpReturnedString, nSize, lpFileName);
		}

		if (!lpAppName) {
			return GetPrivateProfileSectionNames(lpReturnedString, nSize, lpFileName);
		}

		if (!lpKeyName) {
			return GetPrivateProfileSectionKeyNames(lpAppName, lpReturnedString, nSize, lpFileName);
		}

		auto pf = GetPrivateProfilePtr(lpFileName);
		if (!pf) {
			return 0;
		}

		auto& pt = pf->pt;
		std::string s = (lpDefault ? lpDefault : "");
		if (pt.find(lpAppName) != pt.not_found()) {
			s = pt.get_child(ini_name(lpAppName)).get<std::string>(ini_name(lpKeyName), s);
		}
		s += '\0';

		return CopyString(s, lpReturnedString, nSize);
	}
	DWORD GetPrivateProfileSection(LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			return ::GetPrivateProfileSection(lpAppName, lpReturnedString, nSize, lpFileName);
		}

		if (!lpAppName) {
			return 0;
		}

		auto pf = GetPrivateProfilePtr(lpFileName);
		if (!pf) {
			return 0;
		}

		auto& pt = pf->pt;
		std::stringstream sout;
		if (pt.find(lpAppName) != pt.not_found()) {
			for (auto& i : pt.get_child(ini_name(lpAppName))) {
				sout << i.first << '=' << i.second.get_value<std::string>("") << '\0';
			}
		}
		sout << '\0';

		return CopyString(sout.str(), lpReturnedString, nSize);
	}
	DWORD GetPrivateProfileSectionKeyNames(LPCSTR lpAppName, LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			return ::GetPrivateProfileString(lpAppName, NULL, "", lpReturnedString, nSize, lpFileName);
		}

		if (!lpAppName) {
			return 0;
		}

		auto pf = GetPrivateProfilePtr(lpFileName);
		if (!pf) {
			return 0;
		}


		auto& pt = pf->pt;
		std::stringstream sout;
		if (pt.find(lpAppName) != pt.not_found()) {
			for (auto& i : pt.get_child(ini_name(lpAppName))) {
				sout << i.first << '\0';
			}
		}
		sout << '\0';

		return CopyString(sout.str(), lpReturnedString, nSize);
	}
	DWORD GetPrivateProfileSectionNames(LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			return ::GetPrivateProfileSectionNames(lpReturnedString, nSize, lpFileName);
		}

		auto pf = GetPrivateProfilePtr(lpFileName);
		if (!pf) {
			return 0;
		}

		auto& pt = pf->pt;
		std::stringstream sout;
		for (auto& i : pt) {
			sout << i.first << '\0';
		}
		sout << '\0';

		return CopyString(sout.str(), lpReturnedString, nSize);
	}
	BOOL GetPrivateProfileStruct(LPCSTR lpAppName, LPCSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			return ::GetPrivateProfileStruct(lpAppName, lpKeyName, lpStruct, uSizeStruct, lpFileName);
		}

		if (!lpAppName || !lpKeyName) {
			return FALSE;
		}

		auto l = uSizeStruct * 2 + 2 + 1;
		auto s = std::make_unique<TCHAR[]>(l);

		if (GetPrivateProfileString(lpAppName, lpKeyName, "", s.get(), l, lpFileName) != l - 1) {
			return FALSE;
		}

		BYTE sum = 0;
		for (UINT i = 0; i < uSizeStruct; i++)
		{
			DWORD b = 0;
			if (_stscanf(s.get() + i * 2, "%02X", &b) != 1) {
				return FALSE;
			}
			sum += (BYTE)b;
			((BYTE*)lpStruct)[i] = (BYTE)b;
		}
		{
			DWORD b = 0;
			if (_stscanf(s.get() + uSizeStruct * 2, "%02X", &b) != 1) {
				return FALSE;
			}
			if (sum != b) {
				return FALSE;
			}
		}

		return TRUE;
	}
	BOOL WritePrivateProfileString(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpString, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			return ::WritePrivateProfileString(lpAppName, lpKeyName, lpString, lpFileName);
		}

		if (!lpAppName) {
			return FALSE;
		}

		if (!ReviseFileName(lpFileName)) {
			return FALSE;
		}

		auto func = [&](ini_info& info)
		{
			auto& pt = info.pt;

			if (!lpKeyName) {
				pt.erase(lpAppName);
			}
			else if (!lpString) {
				if (pt.find(lpAppName) != pt.not_found()) {
					pt.get_child(ini_name(lpAppName)).erase(lpKeyName);
				}
			}
			else {

				if (pt.find(lpAppName) == pt.not_found()) {
					pt.put_child(ini_name(lpAppName), boost::property_tree::iptree());
				}
				pt.get_child(ini_name(lpAppName)).put_child(ini_name(lpKeyName), boost::property_tree::iptree(lpString));
			}
			return TRUE;
		};

		return SyncInvoke(std::bind(TaskWrapper, lpFileName, func));
	}
	BOOL WritePrivateProfileSection(LPCSTR lpAppName, LPCSTR lpString, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			return ::WritePrivateProfileSection(lpAppName, lpString, lpFileName);
		}

		if (!lpAppName) {
			return FALSE;
		}

		if (!ReviseFileName(lpFileName)) {
			return FALSE;
		}

		std::ostringstream sout;
		sout << '[' << lpAppName << ']' << std::endl;
		for (auto p = lpString; *p; p += lstrlen(p) + 1) {
			sout << p << std::endl;
		}

		boost::property_tree::iptree pt;
		if (!ReadIni(__FUNCTION__, lpFileName, sout.str(), pt)) {
			return FALSE;
		}

		auto func = [&](ini_info& info)
		{
			info.pt.put_child(ini_name(lpAppName), pt.get_child(ini_name(lpAppName)));
			return TRUE;
		};

		return SyncInvoke(std::bind(TaskWrapper, lpFileName, func));
	}
	BOOL WritePrivateProfileStruct(LPCSTR lpAppName, LPCSTR lpKeyName, LPVOID lpStruct, UINT uSizeStruct, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			return ::WritePrivateProfileStruct(lpAppName, lpKeyName, lpStruct, uSizeStruct, lpFileName);
		}

		auto l = uSizeStruct * 2 + 2 + 1;
		auto s = std::make_unique<TCHAR[]>(l);
		BYTE sum = 0;
		for (UINT i = 0; i < uSizeStruct; i++)
		{
			BYTE b = ((BYTE*)lpStruct)[i];
			sum += b;
			_stprintf(s.get() + i * 2, "%02X", b);
		}
		_stprintf(s.get() + uSizeStruct * 2, "%02X", sum);

		return WritePrivateProfileString(lpAppName, lpKeyName, s.get(), lpFileName);
	}

	DWORD GetPrivateProfile(LPSTR lpReturnedString, DWORD nSize, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			std::string s;
			if (!ReadIniFile(s, lpFileName)) {
				return 0;
			}
			s += '\0';
			return CopyString(s, lpReturnedString, nSize);
		}

		auto pf = GetPrivateProfilePtr(lpFileName);
		if (!pf) {
			return 0;
		}

		auto& pt = pf->pt;
		std::string s;
		if (!WriteIni(__FUNCTION__, lpFileName, s, pt)) {
			return 0;
		}
		s += '\0';

		return CopyString(s, lpReturnedString, nSize);

	}
	BOOL WritePrivateProfile(LPCSTR lpString, LPCSTR lpFileName)
	{
		if (!IsInited()) {
			return WriteIniFile(lpString, lpFileName);
		}

		if (!ReviseFileName(lpFileName)) {
			return FALSE;
		}

		boost::property_tree::iptree pt;
		if (!ReadIni(__FUNCTION__, lpFileName, lpString, pt)) {
			return FALSE;
		}

		auto func = [&](ini_info& info)
		{
			info.pt.swap(pt);
			return TRUE;
		};

		return SyncInvoke(std::bind(TaskWrapper, lpFileName, func));
	}


};