/*******************************************************************************
Файл:      adsi.cpp
Автор:     Кашавцев Ю.А.
Описание:  Обертки для работы с ADSI
*******************************************************************************/

#define _WIN32_WINNT	0x0500

#include "adsi.h"


typedef struct tagADSERRMSG
{
	HRESULT    hr;
	LPCTSTR    pszError;
}ADSERRMSG;

#define ADDADSERROR(x)   x, _T(#x)

ADSERRMSG adsErr[] =
{
	ADDADSERROR(E_ADS_BAD_PATHNAME),
		ADDADSERROR(E_ADS_INVALID_DOMAIN_OBJECT),
		ADDADSERROR(E_ADS_INVALID_USER_OBJECT),
		ADDADSERROR(E_ADS_INVALID_COMPUTER_OBJECT),
		ADDADSERROR(E_ADS_UNKNOWN_OBJECT),
		ADDADSERROR(E_ADS_PROPERTY_NOT_SET),
		ADDADSERROR(E_ADS_PROPERTY_NOT_SUPPORTED),
		ADDADSERROR(E_ADS_PROPERTY_INVALID),
		ADDADSERROR(E_ADS_BAD_PARAMETER),
		ADDADSERROR(E_ADS_OBJECT_UNBOUND),
		ADDADSERROR(E_ADS_PROPERTY_NOT_MODIFIED),
		ADDADSERROR(E_ADS_PROPERTY_MODIFIED),
		ADDADSERROR(E_ADS_CANT_CONVERT_DATATYPE),
		ADDADSERROR(E_ADS_PROPERTY_NOT_FOUND),
		ADDADSERROR(E_ADS_OBJECT_EXISTS),
		ADDADSERROR(E_ADS_SCHEMA_VIOLATION),
		ADDADSERROR(E_ADS_COLUMN_NOT_SET),
		ADDADSERROR(E_ADS_INVALID_FILTER),
		ADDADSERROR(0),
};

std::string GetStandardADSIError(HRESULT hr)
{
	std::string s;

	if (hr & 0x00005000)
	{
		int idx=0;
		while (adsErr[idx].hr != 0)
		{
			if (adsErr[idx].hr == hr)
			{
				return adsErr[idx].pszError;
			}
			++idx;
		}
	}

	return "";
}


TADSIError::TADSIError(const std::string &msg, DWORD err) throw()
{
	error = err;
	message = msg;

	if (SUCCEEDED(err))
	{
		return;
	}

	if (error & 0x00005000) // Стандартные ошибки ADSI
	{
		message += " ";
		char	e[34];
#if _MSC_VER >= 1800
		message += _itoa_s(error, e, 10);
#else
		message += itoa(error, e, 10);
#endif
		message += ": ";
		message	+= GetStandardADSIError(error);
	}
	else if (HRESULT_FACILITY(err) == FACILITY_WIN32) // Ошибки Win32
	{
		TWin32Error	win32error("", error);
		message += ": ";
		message	+= win32error.what();
	}

	// Расширенное сообщение об ошибке ADSI
	WCHAR szBuffer[MAX_PATH];
	WCHAR szName[MAX_PATH];
	DWORD dwError;

	HRESULT hr = ADsGetLastError(&dwError, szBuffer, MAX_PATH, szName, MAX_PATH);

	if ( SUCCEEDED(hr) && dwError != ERROR_INVALID_DATA  && wcslen(szBuffer))
	{
		message += " (";
		message += WStrToCStr(szName);
		message += ": ";
		message += WStrToCStr(szBuffer);
		message += ")";
	}
}

const char* TADSIError::what() const throw()
{
	return message.c_str();
}

TADSI::~TADSI()
{
	if (rootDSE != NULL)
	{
		rootDSE->Release();
	}
	if (schemaNC != NULL)
	{
		schemaNC->Release();
	}
	if (confNC != NULL)
	{
		confNC->Release();
	}
	if (defNC != NULL)
	{
		defNC->Release();
	}
	if (confNCDel != NULL)
	{
		confNCDel->Release();
	}
	if (defNCDel != NULL)
	{
		defNCDel->Release();
	}
	if (domainDnsNC != NULL)
	{
		domainDnsNC->Release();
	}
	if (forestDnsNC != NULL)
	{
		forestDnsNC->Release();
	}
};

// Получение строкового описания класса объекта Active Directory по GUID
std::string TADSI::GetClassNameByGUID(const std::string &id)
{
	HRESULT hr;

	hr = ConnectLDAPServer();
	if (!SUCCEEDED(hr))
	{
		throw TADSIError("Ошибка получения корневого объекта AD", hr);
	}
	// Получение контекста схемы службы каталогов
	NCInit(L"schemaNamingContext", &schemaNC);

	// Формирование строки поиска
	std::wstring	filter = L"(schemaIDGuid=";
	filter += GUIDStrToOctetString(CStrToWStr(id));
	filter += L")";

	// Подготовка списка атрибутов для возвращения
	LPWSTR pszAttr[] = {L"lDAPDisplayName"};
	ADS_SEARCH_HANDLE hSearch;
	DWORD dwCount = sizeof(pszAttr)/sizeof(LPWSTR);

	// Выполнение поиска
	hr=schemaNC->ExecuteSearch((LPWSTR)filter.c_str(), pszAttr, dwCount, &hSearch);
	if (!SUCCEEDED(hr))
	{
		throw TADSIError("Ошибка поиска класса AD", hr);
	}

	// Получение результата
	std::vector<std::string>	className;
	className = GetSearchResult(schemaNC, hSearch, pszAttr, dwCount);
	return className.empty()?"":className[0];
}

// Получение строкового описания объекта Active Directory по GUID
std::string TADSI::GetObjectNameByGUID(const std::string &id, const std::string &namingContext)
{
	HRESULT hr;

	hr = ConnectLDAPServer();
	if (!SUCCEEDED(hr))
	{
		throw TADSIError("Ошибка получения корневого объекта AD", hr);
	}
	// Получение контекста имен по умолчанию службы каталогов
	IDirectorySearch	*NC;
	if (namingContext == "defaultNamingContext")
	{
		NCInit(CStrToWStr(namingContext), &defNC);
		NC = defNC;
	}
	else if (namingContext == "schemaNamingContext")
	{
		NCInit(CStrToWStr(namingContext), &schemaNC);
		NC = schemaNC;
	}
	else if (namingContext == "configurationNamingContext")
	{
		NCInit(CStrToWStr(namingContext), &confNC);
		NC = confNC;
	}
	else
	{
		throw TADSIError("Ошибка определения контекста поиска в AD");
	}

	// Формирование строки поиска
	std::wstring	filter = L"(objectGUID=";
	filter += GUIDStrToOctetString(CStrToWStr(id));
	filter += L")";

	// Подготовка списка атрибутов для возвращения
	LPWSTR pszAttr[] = {L"distinguishedName", L"displayName"};
	ADS_SEARCH_HANDLE hSearch;
	DWORD dwCount = sizeof(pszAttr)/sizeof(LPWSTR);

	// Выполнение поиска
	hr=NC->ExecuteSearch((LPWSTR)filter.c_str(), pszAttr, dwCount, &hSearch);
	if (!SUCCEEDED(hr))
	{
		throw TADSIError("Ошибка поиска объекта AD", hr);
	}

	// Получение результата
	std::vector<std::string>	objectAttr;
	objectAttr = GetSearchResult(schemaNC, hSearch, pszAttr, dwCount);
	std::string	objectName;
	if (objectAttr.size() == 1)
	{
		objectName = objectAttr[0];
	}
	else if (objectAttr.size() == 2)
	{
		objectName = (std::string)"distinguishedName: " + objectAttr[0] + " displayName: " + objectAttr[1];
	}
	return objectName;
}

// Получение строкового описания удаленного(захороненного) объекта Active Directory по GUID
std::string TADSI::GetDeletedObjectNameByGUID(const std::string &id, const std::string &namingContext)
{
	HRESULT hr;

	hr = ConnectLDAPServer();
	if (!SUCCEEDED(hr))
	{
		throw TADSIError("Ошибка получения корневого объекта AD", hr);
	}
	// Получение контейнера удаленных объектов службы каталогов
	IDirectorySearch	*NC;
	if (namingContext == "defaultNamingContext")
	{
		NCInitDel(CStrToWStr(namingContext), &defNCDel);
		NC = defNCDel;
	}
	else if (namingContext == "configurationNamingContext")
	{
		NCInitDel(CStrToWStr(namingContext), &confNCDel);
		NC = confNCDel;
	}
	else if (namingContext == "DomainDnsZones")
	{
		NCInitDel(CStrToWStr(namingContext), &domainDnsNC);
		NC = domainDnsNC;
	}
	else if (namingContext == "ForestDnsZones")
	{
		NCInitDel(CStrToWStr(namingContext), &forestDnsNC);
		NC = forestDnsNC;
	}
	else
	{
		throw TADSIError("Ошибка определения контекста поиска в AD");
	}

	// Формирование строки поиска
	std::wstring	filter = L"(objectGUID=";
	filter += GUIDStrToOctetString(CStrToWStr(id));
	filter += L")";

	// Подготовка списка атрибутов для возвращения
	LPWSTR pszAttr[] = {L"distinguishedName"};
	ADS_SEARCH_HANDLE hSearch;
	DWORD dwCount = sizeof(pszAttr)/sizeof(LPWSTR);

	// Выполнение поиска
	hr=NC->ExecuteSearch((LPWSTR)filter.c_str(), pszAttr, dwCount, &hSearch);
	if (!SUCCEEDED(hr))
	{
		throw TADSIError("Ошибка поиска захороненного объекта AD", hr);
	}

	// Получение результата
	std::vector<std::string>	objectName;
	objectName = GetSearchResult(schemaNC, hSearch, pszAttr, dwCount);
	return objectName.empty()?"":objectName[0];
}

// Соединение с LDAP сервером (инициализация объекта RootDSE)
HRESULT TADSI::ConnectLDAPServer()
{
	HRESULT hr = S_OK;

	// Получение корневого объекта службы каталогов
	if (rootDSE == NULL)
	{
		hr = ADsGetObject(L"LDAP://RootDSE", IID_IADs, (void**)&rootDSE);
	}
	return hr;
}

// Инициализация контекста поиска по схеме AD
void TADSI::NCInit(const std::wstring &namingContext, IDirectorySearch **NC)
{
	HRESULT hr;
	// Получение контекста схемы службы каталогов
	if (*NC == NULL)
	{
		VARIANT	NCDesc;
		VariantInit(&NCDesc);
		hr = rootDSE->Get(SysAllocString(namingContext.c_str()), &NCDesc);
		std::wstring	NCDescPath = L"LDAP://";
		NCDescPath += NCDesc.bstrVal;
		hr = ADsGetObject(NCDescPath.c_str(), IID_IDirectorySearch, (void**)NC);
		if (!SUCCEEDED(hr))
		{
			throw TADSIError("Ошибка инициализации контекста поиска в AD", hr);
		}

		// Подготовка к поиску
		SetSearchSettings(*NC);
	}
}

// Инициализация контекста поиска захороненных объектов в AD
void TADSI::NCInitDel(const std::wstring &namingContext, IDirectorySearch **NC)
{
	HRESULT hr;
	// Получение контекста схемы службы каталогов
	if (*NC == NULL)
	{
		VARIANT	NCDesc;
		VariantInit(&NCDesc);
		std::wstring	NCDescPath = L"LDAP://CN=Deleted Objects,";
		if (namingContext == L"DomainDnsZones" || namingContext == L"ForestDnsZones")
		{
			hr = rootDSE->Get(SysAllocString(L"defaultNamingContext"), &NCDesc);
			NCDescPath += L"DC=";
			NCDescPath += namingContext;
			NCDescPath += L",";
		}
		else
		{
			hr = rootDSE->Get(SysAllocString(namingContext.c_str()), &NCDesc);
		}
		NCDescPath += NCDesc.bstrVal;
		hr = ADsOpenObject((LPWSTR)NCDescPath.c_str(), NULL, NULL, ADS_FAST_BIND | ADS_SECURE_AUTHENTICATION, IID_IDirectorySearch, (void**)NC);
		if (!SUCCEEDED(hr))
		{
			throw TADSIError("Ошибка инициализации контекста поиска захороненных объектов в AD", hr);
		}

		// Подготовка к поиску
		SetSearchSettingsDel(*NC);
	}
}

// Установка настроек поиска для существующих объектов
void TADSI::SetSearchSettings(IDirectorySearch *NC)
{
	HRESULT hr;

	// Установка настроек поиска
	ADS_SEARCHPREF_INFO prefInfo[2];
	prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
	prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
	prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

	// Установка максимального количества возвращаемых записей 1
	prefInfo[1].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
	prefInfo[1].vValue.dwType = ADSTYPE_INTEGER;
	prefInfo[1].vValue.Integer = 1;

	// Сопоставление настроек поиска с контекстом поиска
	hr = NC->SetSearchPreference(prefInfo, 2);
	if (!SUCCEEDED(hr))
	{
		throw TADSIError("Ошибка установки настроек поиска в AD", hr);
	}
}

// Установка настроек поиска для захороненных объектов
void TADSI::SetSearchSettingsDel(IDirectorySearch *NC)
{
	HRESULT hr;

	// Подготовка к поиску
	// Установка настроек поиска
	ADS_SEARCHPREF_INFO prefInfo[3];
	prefInfo[0].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
	prefInfo[0].vValue.dwType = ADSTYPE_INTEGER;
	prefInfo[0].vValue.Integer = ADS_SCOPE_SUBTREE;

	// Поиск удаленных объектов
	prefInfo[1].dwSearchPref = ADS_SEARCHPREF_TOMBSTONE;
	prefInfo[1].vValue.dwType =	ADSTYPE_BOOLEAN;
	prefInfo[1].vValue.Boolean = TRUE;

	// Установка максимального количества возвращаемых записей 1
	prefInfo[2].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
	prefInfo[2].vValue.dwType = ADSTYPE_INTEGER;
	prefInfo[2].vValue.Integer = 1;

	// Сопоставление настроек поиска с объектом поиска
	hr = NC->SetSearchPreference(prefInfo, 3);
	if (!SUCCEEDED(hr))
	{
		throw TADSIError("Ошибка установки настроек поиска захороненных объектов в AD", hr);
	}
}

// Получение результата поиска
std::vector<std::string> TADSI::GetSearchResult(IDirectorySearch *NC, ADS_SEARCH_HANDLE hSearch, LPWSTR attr[], DWORD numberAttributes)
{
	HRESULT hr;

	// Получение результата
	ADS_SEARCH_COLUMN			col;
	std::vector<std::string>	attrList;
	if (NC->GetFirstRow(hSearch) != S_ADS_NOMORE_ROWS)
	{
		for (DWORD i = 0; i < numberAttributes; ++i)
		{
			hr = NC->GetColumn(hSearch, attr[i], &col);
			if (SUCCEEDED(hr))
			{
				attrList.push_back(WStrToCStr(col.pADsValues->CaseIgnoreString));
				NC->FreeColumn(&col);
			}
			else if (i == 0 ||
				    (i > 0 && hr != E_ADS_COLUMN_NOT_SET)) // Временное решение на отсутствие атрибута "displayName", которое должно присутствовать у всех объетов, но иногда встречаются объекты без этого атрибута
			{
				throw TADSIError("Ошибка получения результата поиска в AD", hr);
			}
		}
	}
	NC->CloseSearchHandle(hSearch);
	return attrList;
}

// Преобразование отображаемой строки GUID вида {19195A5B-6DA0-11D0-AFD3-00C04FD930C9}
// к Octet-строке вида \\5B\\5A\\19\\19\\A0\\6D\\D0\\11\\AF\\D3\\00\\C0\\4F\\D9\\30\\C9
std::wstring GUIDStrToOctetString(const std::wstring &strGUID)
{
	if (strGUID.size() != 38)
		return L"";

	std::wstring	octetGUID = L"\\";
	octetGUID += strGUID.substr(7, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(5, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(3, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(1, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(12, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(10, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(17, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(15, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(20, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(22, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(25, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(27, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(29, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(31, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(33, 2);
	octetGUID += L"\\";
	octetGUID += strGUID.substr(35, 2);

	return octetGUID;
}