

#include "icexception.h"
#include <windows.h>
#include <winbase.h>
#include <aclapi.h>
#include <tchar.h>
#include <Lmaccess.h>
#include <Lmerr.h>
#include <Lmapibuf.h>
#include <fstream>

#include "agenttools.h"
#include "aloader.h"
#include "Cfs_comm.h"
#include "findmask.h"
#include "fileprot.h"
#include "inifile.h"
#include "sysedt.h"
#include "spyaids.h"
#include "spyfiles.h"
//#include "service.h"
#include "WIN32C.H"

/*****************************************************************/
/*                                                               */
/*         Удаление каталога с подкаталогами                     */
/*   Ret: 1 - есть файлы для отложенного удаления                */
/*                                                               */
/*****************************************************************/
int AgentTools::DeleteDirectory (char *_Dir, BOOL DetAfterRestart, BOOL SNet)
{
    int		res=0;
    int     bHaveMoveAfterrestart = 0;
    char		Files[MAX_PATH], SubDir[MAX_PATH];
    char		DelFile[MAX_PATH],
                Dir[MAX_PATH];
    WIN32_FIND_DATA FileData;
    HANDLE	hFile;
    
    strcpy (Dir, _Dir);
    if (Dir[strlen(Dir)-1] == '\\')
        Dir[strlen(Dir)-1] = 0;

    strcpy (SubDir, Dir);
    strcpy (DelFile, Dir);
    strcpy (Files, Dir);
    strcat (Files, "\\*");
    
    hFile = FindFirstFile (Files, &FileData);
    if (hFile != INVALID_HANDLE_VALUE) res = 1;
    while (res != 0) {
        //  если директория
        if (FileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
            strcat (SubDir, "\\");
            strcat (SubDir, FileData.cFileName);
            if (_stricmp (FileData.cFileName, ".") != 0 &&
                _stricmp (FileData.cFileName, "..") != 0) {
                DeleteDirectory (SubDir, DetAfterRestart, SNet);
            }
        }
        else {
            //  если файл
            strcat (DelFile, "\\");
            strcat (DelFile, FileData.cFileName);
            SetFileAttributes(DelFile, FILE_ATTRIBUTE_NORMAL);
			bHaveMoveAfterrestart |= AgentTools::DeleteFile_AfterReboot (DelFile, DetAfterRestart, SNet);
            
        }
        //  ищем следующий файл
        strcpy (SubDir, Dir);
        strcpy (DelFile, Dir);
        res = FindNextFile (hFile, &FileData);
    }
    
    if (RemoveDirectory (Dir) == 0) {
        if (DetAfterRestart == TRUE && SNet == FALSE) {
            bHaveMoveAfterrestart |= DeleteFile_AfterReboot (Dir, DetAfterRestart, SNet);
        }
    }
    
    return bHaveMoveAfterrestart;
} 
int AgentTools::DeleteDirectory2 (char *_Dir, std::vector<std::string> & ExcludeList, BOOL DetAfterRestart, BOOL SNet)
{
    int		res=0;
    int     bHaveMoveAfterrestart = 0;
    char		Files[MAX_PATH], SubDir[MAX_PATH];
    char		DelFile[MAX_PATH],
        Dir[MAX_PATH];
    WIN32_FIND_DATA FileData;
    HANDLE	hFile;

    strcpy (Dir, _Dir);
    if (Dir[strlen(Dir)-1] == '\\')
        Dir[strlen(Dir)-1] = 0;

    strcpy (SubDir, Dir);
    strcpy (DelFile, Dir);
    strcpy (Files, Dir);
    strcat (Files, "\\*");

    hFile = FindFirstFile (Files, &FileData);
    if (hFile != INVALID_HANDLE_VALUE) res = 1;
    while (res != 0) {
        //  если директория
        if (FileData.dwFileAttributes == FILE_ATTRIBUTE_DIRECTORY) {
            strcat (SubDir, "\\");
            strcat (SubDir, FileData.cFileName);
            if (_stricmp (FileData.cFileName, ".") != 0 &&
                _stricmp (FileData.cFileName, "..") != 0) {
                    DeleteDirectory2 (SubDir, ExcludeList, DetAfterRestart, SNet);
                }
        }
        else {
            //  если файл
            strcat (DelFile, "\\");
            strcat (DelFile, FileData.cFileName);
            if (IsExcludeFile(DelFile, ExcludeList) == FALSE) {
                SetFileAttributes(DelFile, FILE_ATTRIBUTE_NORMAL);
                bHaveMoveAfterrestart |= AgentTools::DeleteFile_AfterReboot (DelFile, DetAfterRestart, SNet);
            }

        }
        //  ищем следующий файл
        strcpy (SubDir, Dir);
        strcpy (DelFile, Dir);
        res = FindNextFile (hFile, &FileData);
    }

    if (RemoveDirectory (Dir) == 0) {
        if (DetAfterRestart == TRUE && SNet == FALSE) {
            bHaveMoveAfterrestart |= DeleteFile_AfterReboot (Dir, DetAfterRestart, SNet);
        }
    }

    return bHaveMoveAfterrestart;
} 


/*****************************************************************/
/*                                                               */
/*      Присутствует ли файл в списке исключаемых                */
/*                                                               */
/*****************************************************************/
BOOL AgentTools::IsExcludeFile (char * FullFileName, std::vector<std::string> & ExcludeList)
{
    for (int i = 0; i < (int)ExcludeList.size(); i ++) {
        //if (UpStrCmp(FullFileName, ExcludeList[i].c_str()) == 0)
            //return TRUE;
        char file[MAX_PATH];
        char mask[MAX_PATH];
        strcpy(file, FullFileName);
        strcpy(mask, ExcludeList[i].c_str());
        strupr(file);
        strupr(mask);
        if (FindMask(file, mask) == true)
            return TRUE;
    }
    return FALSE;

}


/*****************************************************************/
/*                                                               */
/*      Удаление файла после перезагрузки в Windows 9x           */
/*                                                               */
/*****************************************************************/
#ifndef _NT

BOOL AgentTools::MoveFileAfterRestart (char *file) 
{
    char WinInit[MAX_PATH];
    char Short[MAX_PATH];
    DWORD dwSize;

    GetWindowsDirectory (WinInit, MAX_PATH);
    strcat (WinInit, "\\WININIT.INI");

    memset (Short, 0, MAX_PATH);
    dwSize = GetShortPathName (file, Short, MAX_PATH);
    if (dwSize != NULL) {
        if (AddPrivateProfileString ("rename", "NUL", Short, WinInit) == 0)
            return TRUE;
    }
    return FALSE;

}

#endif

/********************************************************************************/
/*                                                                              */
/*    Установка ключа AppInitDLL_s                                              */
/*    Если ключа Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows нет,  */      
/*    то он создается с правами Read для Everyone и Full Control для            */
/*    Administrator                                                             */
/*                                                                              */
/********************************************************************************/
#ifdef _NT
BOOL AgentTools::Add_AppInitDLLs (char * file)
{
    HKEY   hKey = NULL;
    char   subkey[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
    char   Data[MAX_PATH*3];
    char * NewData;
    DWORD  dwSize = MAX_PATH*3,
           dwRes,
           dwType;
    PSID   pEveryoneSID = NULL, 
           pAdminSID = NULL;
    PACL   pACL = NULL;
    PSECURITY_DESCRIPTOR pSD = NULL;
    EXPLICIT_ACCESS ea[2];
    SID_IDENTIFIER_AUTHORITY SIDAuthWorld = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY SIDAuthNT = SECURITY_NT_AUTHORITY;
    SECURITY_ATTRIBUTES sa;
    
    
    
    try {
        strcpy (Data, file);
        strcat (Data, ",");
        NewData = Data + strlen(Data);
        
        // Create a well-known SID for the Everyone group.
        if(!AllocateAndInitializeSid( &SIDAuthWorld, 1, SECURITY_WORLD_RID, 0, 0, 0, 0, 0, 0, 0, &pEveryoneSID))
            throw (int)0;
        
        // Initialize an EXPLICIT_ACCESS structure for an ACE.
        // The ACE will allow Everyone read access to the key.
        ZeroMemory(&ea, 2 * sizeof(EXPLICIT_ACCESS));
        ea[0].grfAccessPermissions = KEY_READ;
        ea[0].grfAccessMode = SET_ACCESS;
        ea[0].grfInheritance= NO_INHERITANCE;
        ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[0].Trustee.TrusteeType = TRUSTEE_IS_WELL_KNOWN_GROUP;
        ea[0].Trustee.ptstrName  = (LPTSTR) pEveryoneSID;
        
        // Create a SID for the BUILTIN\Administrators group.
        if(!AllocateAndInitializeSid( &SIDAuthNT, 2, SECURITY_BUILTIN_DOMAIN_RID, DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0, &pAdminSID) )
            throw (int)0;
        
        // Initialize an EXPLICIT_ACCESS structure for an ACE.
        // The ACE will allow the Administrators group full access to the key.
        ea[1].grfAccessPermissions = KEY_ALL_ACCESS;
        ea[1].grfAccessMode = SET_ACCESS;
        ea[1].grfInheritance= NO_INHERITANCE;
        ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea[1].Trustee.ptstrName  = (LPTSTR) pAdminSID;
        
        // Create a new ACL that contains the new ACEs.
        dwRes = SetEntriesInAcl(2, ea, NULL, &pACL);
        if (ERROR_SUCCESS != dwRes)
            throw (int)0;
        
        // Initialize a security descriptor.  
        pSD = (PSECURITY_DESCRIPTOR) LocalAlloc(LPTR, SECURITY_DESCRIPTOR_MIN_LENGTH); 
        if (pSD == NULL)
            throw (int)0;
        
        if (!InitializeSecurityDescriptor(pSD, SECURITY_DESCRIPTOR_REVISION))
            throw (int)0;
        
        // Add the ACL to the security descriptor. 
        if (!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE))
            throw (int)0;
        
        // Initialize a security attributes structure.
        sa.nLength = sizeof (SECURITY_ATTRIBUTES);
        sa.lpSecurityDescriptor = pSD;
        sa.bInheritHandle = FALSE;

        dwRes = RegCreateKeyEx(HKEY_LOCAL_MACHINE, subkey, NULL, "", 0, KEY_READ|KEY_WRITE, &sa, &hKey, NULL);
        if (dwRes != ERROR_SUCCESS)
            throw (int)0;
        
        dwRes = RegQueryValueEx(hKey, "AppInit_DLLs", NULL, &dwType,
            (LPBYTE)NewData, &dwSize);
        if (dwRes != ERROR_SUCCESS || dwSize <= 1)
            strcpy (Data, file);

        dwRes = RegSetValueEx(hKey, "AppInit_DLLs", NULL, REG_SZ,
            (CONST BYTE *)Data, strlen(Data)+1);
        if (dwRes != ERROR_SUCCESS)
            throw (int)0;

        FreeSid(pEveryoneSID);
        FreeSid(pAdminSID);
        LocalFree(pACL);
        LocalFree(pSD);
        RegCloseKey (hKey);
    }
    catch (int) {
        if (pEveryoneSID)
            FreeSid(pEveryoneSID);
        if (pAdminSID) 
            FreeSid(pAdminSID);
        if (pACL) 
            LocalFree(pACL);
        if (pSD) 
            LocalFree(pSD);
        if (hKey)
            RegCloseKey (hKey);
        return FALSE;
    }

    return TRUE;
}

/*****************************************************************/
/*                                                               */
/*              Удаление ключа AppInitDLL_s                      */
/*                                                               */
/*****************************************************************/
BOOL AgentTools::Remove_AppInitDLLs (char * strfile)
{
  HKEY   hKey = NULL;
  char   subkey[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Windows";
  char   Data[MAX_PATH*2];
  char   file[MAX_PATH];
  char * p, * p1, * p2;
  DWORD  dwSize = MAX_PATH*2,
    dwRes,
    dwType;
  bool IsFindDsk = false;

  try {
    dwRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkey, NULL, KEY_READ|KEY_WRITE, &hKey);
    if (dwRes != ERROR_SUCCESS)
      throw (int)0;

    dwRes = RegQueryValueEx(hKey, "AppInit_DLLs", NULL, &dwType, (LPBYTE)Data, &dwSize);
    if (dwRes == ERROR_SUCCESS) {
      _strupr(Data);
      strcpy(file, strfile);
      _strupr(file);

      while ((p  = strstr(Data, file)) != NULL) {
        IsFindDsk = true;
        p1 = strchr (p, ' ');
        p2 = strchr (p, ',');
        p1 = (p1)? p1: p + strlen(p);
        p2 = (p2)? p2: p + strlen(p);
        p1 = (p1 < p2)? p1: p2;
        while ( !isalpha(*p1) && !isdigit(*p1) && *p1 != 0)
          p1++;
        memmove (p, p1, strlen(p1)+1);
      }

      if (IsFindDsk == true) {
        dwRes = RegSetValueEx(hKey, "AppInit_DLLs", NULL, REG_SZ, (CONST BYTE *)Data, strlen(Data)+1);
        if (dwRes != ERROR_SUCCESS)
          throw (int)0;
      }
    }

    RegCloseKey (hKey);
  }
  catch (int) {
    if (hKey)
      RegCloseKey (hKey);
    return FALSE;
  }

  return TRUE;
}
#endif

/*****************************************************************/
/*                                                               */
/*                Удаление файла после перезагрузки              */
/*  Ret: 1 - было отложенное удаление                            */
/*                                                               */
/*****************************************************************/
int AgentTools::DeleteFile_AfterReboot (char * Buffer, BOOL bDelAfterRestart, BOOL SNet) 
{
    int res;
    int bHaveMoveAfterRestart = 0;

    if ((res = DeleteFile(Buffer)) == 0) {
        if (bDelAfterRestart == TRUE) {
#ifdef _NT
            SetFileAttributes(Buffer, FILE_ATTRIBUTE_NORMAL);
            MoveFileEx (Buffer, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
#else
            if (SNet == FALSE) {
                MoveFileAfterRestart (Buffer);
            }
#endif
        }
    }
    return res;
}


/*****************************************************************/
/*                                                               */
/*            Получение IP-адреса и порта из строки              */
/*                                                               */
/*****************************************************************/
void AgentTools::GetAltMBData (char * str_address, char * altaddress, unsigned short * altport)
{
	char * p,
		 * p1,
		   str[100];

	strcpy (str, str_address);

	// ищем порт
	p = p1 =_tcschr (str, ':');
	if (p == 0 || p == str)
		throw CStrExcept("Некорректная запись в поле \"Адрес МБ\"");
	while ( *p1 != 0 ) {
		if (_istdigit(*p1))
			break;
		p1 = _tcsinc(p1);
	}
	if (*p1 == 0)
		throw CStrExcept("Некорректная запись в поле \"Адрес МБ\"");
	*altport = _ttoi (p1);
	
	// ищем адрес
	*p = 0;
	while ( p > str) {
		if (_istdigit(*p) || _istalpha(*p))
			break;
		p = _tcsdec(str, p);
	}
	if (*p == 0)
		throw CStrExcept("Некорректная запись в поле \"Адрес МБ\"");
	else
		*(_tcsinc(p)) = 0;
	
	int digit = 1,
		point = 0;
	// проверка корректности адреса
	for (size_t i=0; i<_tcslen(str)-1; i++) {
		if (str[i] == '.')
			point ++;
		if ( !_istdigit(str[i]) &&
			str[i] != '.' )
            digit = 0;
    }
    if (digit && point != 3)
        throw CStrExcept("Некорректная запись в поле \"Адрес МБ\"");
    if (digit = 0)
        throw CStrExcept("Некорректная запись в поле \"Адрес МБ\"");
    
    strcpy (altaddress, str);
}


/*****************************************************************/
/*                                                               */
/*      Проверка текущего пользователя на принадлежность         */
/*      к группе администраторов                                 */
/*                                                               */
/*****************************************************************/
#ifdef _NT

BOOL AgentTools::IsCurUserAdminEx()
{
    SID_IDENTIFIER_AUTHORITY  sidAuthority = SECURITY_NT_AUTHORITY;
    PSID                      pSID;
    HANDLE                    hToken;
    BYTE                      buf[4096];
    PTOKEN_GROUPS             pTokenGroupsInfo = (PTOKEN_GROUPS)buf;
    DWORD                     dwLen;
    BOOL                      bRes;
    int                       i;

    if (OpenProcessToken (GetCurrentProcess (), 
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken) == 0)
        return FALSE;

    if (GetTokenInformation(hToken, TokenGroups, pTokenGroupsInfo, 4096, &dwLen) == 0)
        return FALSE;

    // Выделяем память и инициализируем SID для встроенной группы администраторов  
    if(!AllocateAndInitializeSid(&sidAuthority,	2, SECURITY_BUILTIN_DOMAIN_RID,
        DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0,	0, 0,	&pSID))
        return FALSE;
    
    // Проверка корректности SID
    if(!IsValidSid(pSID))
        return FALSE;
    
    // Проверяем присутствие SID в массиве pTokenGroupsInfo
    for (i = 0, bRes = FALSE; i < (int)pTokenGroupsInfo->GroupCount; i ++) {
        if ( EqualSid(pSID, (pTokenGroupsInfo->Groups[i].Sid)) ) {
            bRes = TRUE;
            break;
        }
    }

    LocalFree(pSID);

    return bRes;
}    

#endif


/*****************************************************************/
/*                                                               */
/*        Определение "Рубеж 1.4 для Windows NT"                 */
/*                                                               */
/*****************************************************************/
BOOL AgentTools::IsRubezh1_4()
{
    try {
        
#ifdef _NT
        // Проверка наличия устнойства RUBEG.SYS
        HANDLE hRubezhHandle;
        char   RubezhDriveName[] = "\\\\.\\RUBEG";

		hRubezhHandle = CreateFile(
			RubezhDriveName,	// Win32 имя устройства
			GENERIC_READ,		// Режим доступа (для чтения)
			FILE_SHARE_READ,	// Режим разделения
			NULL,	  		    // Указатель на атрибуты защиты
			OPEN_EXISTING,		// Как создавать
			0,			        // Атрибуты файла
			NULL);			    // Описатель файла с атрибутами
        
        if (hRubezhHandle == INVALID_HANDLE_VALUE)
            throw CStrExcept("Ошибка #%d открытия устройства %s.", GetLastError(), RubezhDriveName);
        
        CloseHandle(hRubezhHandle);
        
#else
        return FALSE;
#endif

    }
    catch (CStrExcept &) {
        return FALSE;
    }
    return TRUE;

}

BOOL AgentTools::FindBuffer (unsigned char *filebuf, 
                               DWORD filebufsize,
                               unsigned char *sign,
                               DWORD signsize, 
                               DWORD *dwOffset)
{
    unsigned char* pointer = filebuf;

    while (pointer < filebuf+filebufsize-1) {
        
        // ищем первый символ
        pointer = (unsigned char*)memchr (pointer, (int)sign[0], filebufsize-(pointer-filebuf-1));
        if (pointer == NULL)
            return FALSE;
        
        // проверяем на размер
        int len = signsize + (int)(pointer-filebuf);
        int len1 = filebufsize+(int)filebuf;
        if (len >= len1)
            return FALSE;

        // нашли первый символ
        if (memcmp (pointer, sign, signsize) == 0) {
            *dwOffset = (DWORD)(pointer - filebuf);
            return TRUE;
        }

        pointer++;
    }

    return FALSE;
}

#ifdef _NT
int AgentTools::EnableShutDownPriv()
{
    HANDLE hToken;
    LUID ShutDownValue;
    TOKEN_PRIVILEGES tkp;
	  int	res;

    if (!OpenProcessToken(GetCurrentProcess(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            &hToken)) {
        return GetLastError();
    }

    if (!LookupPrivilegeValue((LPSTR) NULL,
            SE_SHUTDOWN_NAME,
            &ShutDownValue)) {
        return GetLastError();
    }

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Luid = ShutDownValue;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    AdjustTokenPrivileges(hToken,
        FALSE,
        &tkp,
        sizeof(TOKEN_PRIVILEGES),
        (PTOKEN_PRIVILEGES) NULL,
        (PDWORD) NULL);
    if ((res = GetLastError()) != ERROR_SUCCESS) {
        return res;
    }

    return NO_ERROR;
}
#endif//_NT


/******************************************************************/
/*                                                                */
/*     Определение режима работы (старая или новая МБ).           */
/*     Ищет файл RECAGENT_NAME.                                   */
/*                                                                */
/******************************************************************/
BOOL AgentTools::CheckWorkMode()
{
    HANDLE          hFile;
    char            filename[MAX_PATH];
    WIN32_FIND_DATA FindData;

    GetSpyDir(filename);
    strcat (filename, RECAGENT_NAME);
    
    if ((hFile = FindFirstFile(filename, &FindData)) == INVALID_HANDLE_VALUE)
        return FALSE;

    FindClose(hFile);
    return TRUE;
}

/******************************************************************/
/*                                                                */
/*     Определение версии агента. Читается файл DFA.DLL           */
/*                                                                */
/******************************************************************/
char * AgentTools::GetAgentVersion(char * AgentVersion, int size, DWORD * dwAgentVersionId/* = NULL*/)
{
    char filename[MAX_PATH],
         str[300] = "0000";
    std::ifstream file;
    int  str_len;
    
    if (AgentVersion && size > 4) {
        memset (AgentVersion, 0, size);
        strcpy (AgentVersion, "0000");
    }

    GetSpyDir(filename);
    strcat (filename, AGENT_PROG_INJECT);
    file.open(filename, std::ios::in);
    if (file.is_open()) {
        file.read(str, 300);
        str_len = file.gcount();
        if (str_len > 0)
            str[str_len] = 0;
        file.close();

        // убираем 0x0D 0x0A
        int len = strlen(str);
        for (int i = 0; i < len; i++) {
            if (str[i] == 0x0D || str[i] == 0x0A)
                str[i] = 0;
        }
        // если параметры верные, переписываем данные
        if (AgentVersion && size > 4) {
            strcpy (AgentVersion, str);
        }
    }
    
    // Получаем вещественное значение
    if (dwAgentVersionId) {
        * dwAgentVersionId = atol(str);
    }

    return AgentVersion;
}



/******************************************************************/
/*                                                                */
/*        Запуск агента                                           */
/*        В случае ошибки вырабатывается CicException             */
/*                                                                */
/******************************************************************/
/*
void AgentTools::StartAgent()
{
    //========================================================
    // Запускаем CFS
    //========================================================
#ifdef _NT
    CService CFS;
    DWORD dwAccess = SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | STANDARD_RIGHTS_READ;
    CFS.ConnectSCM(dwAccess);
    dwAccess = dwAccess = READ_CONTROL | SERVICE_START;
    if (CFS.OpenService("CFS", dwAccess) == FALSE)
        throw CStrExcept("Запуск агента: Ошибка открытия сервиса агента 0-го уровня");
    if (CFS.StartService() == FALSE)
        throw CStrExcept("Запуск агента: Ошибка запуска агента 0-го уровня");
#endif
    
    //========================================================
    // Запускаем STHL
    //========================================================
    CService STHL;
#ifdef _NT
    dwAccess = SC_MANAGER_CONNECT | SC_MANAGER_ENUMERATE_SERVICE | STANDARD_RIGHTS_READ;
    STHL.ConnectSCM(dwAccess);
    dwAccess = dwAccess = READ_CONTROL | SERVICE_START;
    if (STHL.OpenService("STHL", dwAccess) == FALSE)
        throw CStrExcept("Запуск агента: Ошибка открытия сервиса головного агента");
    if (STHL.StartService() == FALSE)
        throw CStrExcept("Запуск агента: Ошибка запуска головного агента");
#else
    char Sthl_Path[MAX_PATH];
    
    GetSpyDir(Sthl_Path);
    strcat (Sthl_Path, "STHL.EXE");
    if (STHL.StartService(Sthl_Path) == FALSE)
        throw CStrExcept("Запуск агента: Ошибка запуска головного агента");
#endif
}
*/

/******************************************************************/
/*                                                                */
/*                       Копирование файла                        */
/*                                                                */
/******************************************************************/
int AgentTools::MyCopyFile(LPCSTR File, LPCSTR NewFile, BOOL FailIfExists, int TryNum, DWORD dwSleep/* = 500*/)
{
    int res = 0;

    for (int i = 0; i < TryNum; i ++) {
        res = CopyFile (File, NewFile, FailIfExists);
        if (res == 0 && TryNum > 1)
            Sleep (dwSleep);
        else
            break;
    }

    return res;
}

/******************************************************************/
/*                                                                */
/*           Перемещение файла через переименование               */
/*                                                                */
/******************************************************************/
#ifdef _NT
BOOL AgentTools::MoveFileNT_WithRename (const char * File, const char * FileTo)
{
    char    NewFileName[MAX_PATH],
            NewPath[MAX_PATH],
            PrefName[MAX_PATH],
          * Name;
    BOOL    AlreadyRenamed = FALSE;
    
    try {
        if (MoveFile(File, FileTo) == 0) {
            strcpy (NewPath, FileTo);
            Name = strrchr (NewPath, '\\');
            strcpy (PrefName, Name + 1);
            *Name = 0;
            Name = strrchr (PrefName, '.');
            *Name = 0;
            if (strlen(PrefName) > 3)
                PrefName[3] = 0;
            
            if (GetTempFileName(NewPath, PrefName, 0, NewFileName) != 0) {
                DeleteFile (NewFileName);
                
                if (MoveFile (FileTo, NewFileName) != 0) {
                    AlreadyRenamed = TRUE;
                    
                    if (MyCopyFile (File, FileTo, FALSE, 1))
                        if (DeleteFile(File) == 0) {
                            DWORD dwErr = GetLastError();
                            throw (BOOL)FALSE;
                        }
                }
                else {
                    DWORD dwErr = GetLastError();
                    throw (BOOL)FALSE;
                }
            }
            else {
                DWORD dwErr = GetLastError();
                throw (BOOL)FALSE;
            }
        }       
    }
    catch (BOOL) {
        // Исходный файл успели переименовать, а новый переписать не удалось.
        // Нужно восстановить исходный файл
        if (AlreadyRenamed == TRUE) {
            MyCopyFile (NewFileName, FileTo, TRUE, 1);
        }
        return FALSE;
    }
    return TRUE;
}

#endif


/******************************************************************/
/*                                                                */
/*           Копирование файла через переименование               */
/*                                                                */
/******************************************************************/
#ifdef _NT
BOOL AgentTools::CopyFileNT_WithRename (const char * File, const char * FileTo)
{
    char    NewFileName[MAX_PATH],
            NewPath[MAX_PATH],
            PrefName[MAX_PATH],
          * Name;
    BOOL    AlreadyRenamed = FALSE;
    
    try {
        if (CopyFile(File, FileTo, FALSE) == 0) {
            strcpy (NewPath, FileTo);
            Name = strrchr (NewPath, '\\');
            strcpy (PrefName, Name + 1);
            *Name = 0;
            Name = strrchr (PrefName, '.');
            *Name = 0;
            if (strlen(PrefName) > 3)
                PrefName[3] = 0;
            
            if (GetTempFileName(NewPath, PrefName, 0, NewFileName) != 0) {
                DeleteFile (NewFileName);
                
                if (MoveFile (FileTo, NewFileName) != 0) {
                    AlreadyRenamed = TRUE;
                    
                    if (MyCopyFile (File, FileTo, FALSE, 1) == 0) {
						DWORD dwErr = GetLastError();
						throw (BOOL)FALSE;
                    }
                }
                else {
                    DWORD dwErr = GetLastError();
                    throw (BOOL)FALSE;
                }
            }
            else {
                DWORD dwErr = GetLastError();
                throw (BOOL)FALSE;
            }
        }       
    }
    catch (BOOL) {
        // Исходный файл успели переименовать, а новый переписать не удалось.
        // Нужно восстановить исходный файл
        if (AlreadyRenamed == TRUE) {
            MyCopyFile (NewFileName, FileTo, TRUE, 1);
        }
        return FALSE;
    }
    return TRUE;
}
#endif

BOOL AgentTools::IsSecretNet()
{
#ifdef _NT
    return FALSE;
#else
    DWORD TmpDW;
    char  OutBuff;
    BOOL  TmpB;
    CFSAutent cfs;

    if (cfs.Autentification("\\\\.\\CFS.VXD", (char*)AGENT_REMOVE_SIGN, (char*)DYNAMIC_DRIVER_SIGN) == FALSE)
        if (cfs.Autentification("\\\\.\\CFS", (char*)AGENT_REMOVE_SIGN, (char*)DYNAMIC_DRIVER_SIGN) == FALSE)
            throw CStrExcept("Невозможно подключиться к динамическому драйверу");

    // Определяем наличие SecretNet
    TmpB = cfs.CfsNoProtectAsyncIoControl(CFS_GET_STATUS_SNET, &OutBuff, sizeof(OutBuff), &OutBuff, sizeof(OutBuff), &TmpDW);
    if (TmpB) {
        if (OutBuff == 1) // SNet установлен
            return TRUE;
    }
    else
        throw CStrExcept("Ошибка получения данных от динамического драйвера");

    return FALSE;
#endif
}

BOOL AgentTools::IsNTMB(char * CurDir/* = NULL*/)
{
    char Path[MAX_PATH];
    WIN32_FIND_DATA FindData;
    HANDLE hFile;

    if (CurDir == NULL || strlen(CurDir) == 0) {
        GetModuleFileName(GetModuleHandle(NULL), Path, MAX_PATH);
        strcpy( (strrchr(Path, '\\') + 1), RECAGENT_NAME);
    }
    else {
        strcpy(Path, CurDir);
        if (Path[strlen(Path)-1] == '\\')
            Path[strlen(Path)-1] = 0;
        strcat(Path, "\\" RECAGENT_NAME);
    }

    if ((hFile = FindFirstFile(Path, &FindData)) == INVALID_HANDLE_VALUE)
        return FALSE;
    else {
        FindClose(hFile);
        return TRUE;
    }
}

void AgentTools::MakeLOADER(const char * AloaderPath)
{
    char AloaderIni[MAX_PATH];
    char AloaderExe[MAX_PATH];

    strcpy(AloaderIni, AloaderPath);
    if (AloaderIni[strlen(AloaderIni)-1] != '\\')
        strcat(AloaderIni, "\\");
    strcpy(AloaderExe, AloaderIni);
    strcat(AloaderIni, ALOADER_INI);
    strcat(AloaderExe, AGENT_ALOADER);

    TIniFile Ini(AloaderIni);
    TIniFile::TStringList keys;
    try {
        Ini.EnumKeys(ALoader::g_ALoader_SectionName, keys);
        for (unsigned int i = 0; i < keys.size(); i ++) {
            char File[MAX_PATH];
            strcpy(File, AloaderPath);
            if (File[strlen(File)-1] != '\\')
                strcat(File, "\\");
            strcat(File, keys[i].c_str());
            strcat(File, ".EXE");

            if (stricmp(AloaderExe, File) != 0) {
                if (CopyFile(AloaderExe, File, FALSE) == 0)
                    throw CStrExcept("Ошибка %d копирования файла %s -> %s", GetLastError(), AloaderExe, File);
            }
        }
    }
    catch (TIniFile::TException & tex) {
        throw CStrExcept(tex.what());
    }

}