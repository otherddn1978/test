// ControlMan.cpp: implementation of the CAgentCtrl class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable : 4786)

#include <time.h>
#include <strstream>
#include <fstream>
#include "AgentCtrl.h"
#include "clctrl.h"
#include "eventcodes.h"
#include "colorconst.h"
#include <Lm.h>
#include "transtools.h"
#include "spymbcom.h"
#include "logfilemaxsize.h"
#include "agentautent1.h"
#include "md5.h"
#include "confcr.h"
#include "serveri.h"
#include "abdump.h"
#include "filedirtools.h"
#include "dire.h"
#include "hashfile.h"
#include "paramevtparser.h"
#include "spyfiles.h"
#include "evtlst.h"
#include "findmask.h"

// Версия для проверки совместимости с панелью управления, если данная версия не совпадает с версией панели управления то соединение будет разорвано
unsigned VersionConnectNTSM = 1;

const char * CAgentCtrl::ADMIN_GROUP = "AGENTADMIN";
const char * CAgentCtrl::USER_GROUP = "AGENTUSER";
const char * CAgentCtrl::AUDIT_GROUP = "AGENTAUDIT";

CAgentCtrl::CAgentCtrlExcept::CAgentCtrlExcept (int err, const char * fs, ...) throw ()
{
  va_list ap;

  va_start (ap, fs);
  Create(NULL, err, fs, ap);
  va_end (ap);
}

const char * CAgentCtrl::CAgentCtrlExcept::GetStrErr() const throw ()
{
  return "Ошибка аутентификации";
}

LONG CAgentCtrl::ExceptionHandler(DWORD code, PVOID adr, const char * strerr, const CONTEXT *ThreadContext)
{
  LogFileCtrl->printerr("CAgentCtrl : %s\n", strerr);

  return EXCEPTION_CONTINUE_SEARCH;
}

bool CAgentCtrl::StdExceptionHandler(const char * strerr)
{
  LogFileCtrl->printerr ("CAgentCtrl : Необработаное исключение: %s\n", strerr);
  return false;
}

CAgentCtrl::~CAgentCtrl ()
{
  if (BufferOut != NULL)
    delete [] BufferOut;
}

EXCEPTION_STR(
void CAgentCtrl::Init (const std::string & agCurrentDir,
                       const std::string & agDataDir,
                       const std::string & agMailDir,
                       CLogFileMaxSize * log,
                       unsigned long recvTimeOut,
                       const CBazaNt::RecBase & bazaRec,
                       const char * vers,
                       TWin32Thread * mailPotok,
                       const char * postext,
                       const CBazaNt::RecBaseAdv & bazaRecAdv)
)
  AgentCurrentDir = agCurrentDir;
  AgentDataDir = agDataDir;
  AgentMailDir = agMailDir;
  LogFileCtrl = log;
  RecvTimeOut = recvTimeOut;
  BazaRecord = bazaRec;
  Version = vers;
  MailPotok = mailPotok;
  PostExt = postext;

  if (FileDirTools::IsEndSlash(AgentCurrentDir.c_str ()) == true)
    AgentCurrentDir = AgentCurrentDir.substr(0, AgentCurrentDir.size() - 1);
  if (FileDirTools::IsEndSlash(AgentDataDir.c_str ()) == true)
    AgentDataDir = AgentDataDir.substr(0, AgentDataDir.size() - 1);
  if (FileDirTools::IsEndSlash(AgentMailDir.c_str ()) == true)
    AgentMailDir = AgentMailDir.substr(0, AgentMailDir.size() - 1);

  if (UpStrCmp(AgentCurrentDir.c_str(), AgentDataDir.c_str()) == 0)
    AgentDataDir = "";
  if (UpStrCmp(AgentCurrentDir.c_str(), AgentMailDir.c_str()) == 0)
    AgentMailDir = "";
  if (UpStrCmp(AgentDataDir.c_str(), AgentMailDir.c_str()) == 0)
    AgentMailDir = "";

  if (BufferOut == NULL)
    BufferOut = new (std::nothrow) unsigned char [Ctrl::CL_SERV_BUFFER_SIZE];
  if (BufferOut == NULL)
    throw CStrTypeExcept (Str_Function_Name, "Ошибка выделения памяти под буфер обмена");

  WsStateCr.done ();
  CWStateType * st = new (std::nothrow) CWStateType (EventCodes::ASTATE_NA, "Нет данных", SC_NOCOLOR);
  if (!WsStateCr.insert (st) && st != NULL)
    delete st;

  CModelType * mod;
  ModelTypeCr.done ();
  mod = new (std::nothrow) CModelType (bazaRec.Model, bazaRecAdv.ModelDesc, bazaRecAdv.ModelName, bazaRec.EventListName);
  if (!ModelTypeCr.insert (mod) && mod != NULL)
    delete mod;

  AgStateCr.done();
  CAgStateType * a = NULL;
  a = new (std::nothrow) CAgStateType(CBazaNt::ASTATE_NORMAL_MODE.state,CBazaNt::ASTATE_NORMAL_MODE.Name);
  if (!AgStateCr.insert(a) && a!=NULL)
    delete a;
  a = new (std::nothrow) CAgStateType(CBazaNt::ASTATE_TRANSFERED.state,CBazaNt::ASTATE_TRANSFERED.Name);
  if (!AgStateCr.insert(a) && a!=NULL)
    delete a;
  a = new (std::nothrow) CAgStateType(CBazaNt::ASTATE_PREPARE.state,CBazaNt::ASTATE_PREPARE.Name);
  if (!AgStateCr.insert(a) && a!=NULL)
    delete a;
  a = new (std::nothrow) CAgStateType(CBazaNt::ASTATE_UPDATE_NEED.state,CBazaNt::ASTATE_UPDATE_NEED.Name);
  if (!AgStateCr.insert(a) && a!=NULL)
    delete a;
  a = new (std::nothrow) CAgStateType(CBazaNt::ASTATE_REINSTALL_NEED.state,CBazaNt::ASTATE_REINSTALL_NEED.Name);
  if (!AgStateCr.insert(a) && a!=NULL)
    delete a;
  a = new (std::nothrow) CAgStateType(CBazaNt::ASTATE_MUST_BE_DELETED.state,CBazaNt::ASTATE_MUST_BE_DELETED.Name);
  if (!AgStateCr.insert(a) && a!=NULL)
    delete a;

  CAgType *ag = NULL;
  AgTypeCr.done();

  switch (bazaRec.OsType) {
  case CBazaNt::TypeOs::OS_WINNT:
    ag = new (std::nothrow) CAgType(CBazaNt::OS_WINNT.Os,   CBazaNt::OS_WINNT.Name,   1,1,1,1,1,0,1,CBazaNt::OS_WINNT.Dir,   1,0,0,0,0,0,0,1,0,0,0,0, "raw.nt", "ALL");
    break;
  case CBazaNt::TypeOs::OS_WIN_LITE:
    ag = new (std::nothrow) CAgType(CBazaNt::OS_WIN_LITE.Os,CBazaNt::OS_WIN_LITE.Name,1,0,1,0,1,0,1,CBazaNt::OS_WIN_LITE.Dir,1,0,0,0,0,0,0,1,0,0,0,1, "raw.lw", "ALL");
    break;
  default:
    ag = new (std::nothrow) CAgType(bazaRec.OsType,bazaRecAdv.AgentDesc,1,0,1,0,1,0,1,bazaRecAdv.AgentDir,1,0,0,0,0,0,0,1,0,0,0,1, "raw.lw", "ALL");
  }

  ag->eventListNames = new (std::nothrow) CStrList;
  if (!AgTypeCr.insert(ag) && ag != NULL)
    delete ag;
}

EXCEPTION_STR(
unsigned char CAgentCtrl::AutentRec (CTrans * trans, unsigned char ** data, unsigned long * len)
)
  unsigned char AutentRec[260];
  unsigned char Sign[16];
  unsigned long LenOut;
  unsigned char Commm;
  time_t StartSeansTime;
  char userName[250];
  char passwdName[250];

  trans->SetParam((long *)& RecvTimeOut, NULL, NULL);
  // Принимаем номер агента 2б и время начала сеанса 4б и идентифицируем агента
  Commm = trans->RecvCtrlT (RecvTimeOut, Sign, 6, & LenOut);
  switch (Commm) {
  case AgCom::IDENT:
    if (LenOut != 6)
      throw CAgentCtrlExcept (0, "При идентификации получено меньше 6 байт");
    break;
  case AgCom::AG_TYPE:
    ClientType = Sign[0];
    if ((Commm = trans->RecvCtrlT (RecvTimeOut, Sign, 6, & LenOut)) != AgCom::IDENT)
      throw CAgentCtrlExcept (0, "При идентификации принята неправильная команда %d", Commm);
    if (LenOut != 6)
      throw CAgentCtrlExcept (0, "При идентификации получено меньше 6 байт");
    break;
  default:
    throw CAgentCtrlExcept (0, "При идентификации принята неправильная команда %d", Commm);
  }

  memcpy (& StartSeansTime, Sign + 2, 4);
  StartSeansTime = ntohl (StartSeansTime);

  trans->SendCtrl (AgCom::IDENT);

  // Аутентификация агента
  memcpy (AutentRec, AutentAgentBazaKey, 256);
  memcpy (AutentRec + 256, & StartSeansTime, 4);
  makeMD5Sign (Sign, AutentRec, 260);
  trans->ActivateProt (Sign);

  try {
    if ((Commm = trans->RecvCtrlT (RecvTimeOut, AutentRec, 256)) != AgCom::AUTH)
      throw CAgentCtrlExcept (0, "При аутентификации агента принята неправильная команда %d", Commm);
  }
  catch (CTransData::CTransDataExcept &) {
    trans->SendCtrl (AgCom::BAD);
    throw;
  }

  srand (time (NULL));
  unsigned Len = rand () % 256;
  for (unsigned i = 0; i < Len; i ++)
    AutentRec[i] = rand () % 256;
  trans->SendCtrl (AgCom::AUTH, AutentRec, Len);

  makeMD5Sign (Sign, (unsigned char *)& StartSeansTime, 4);
  trans->ActivateProt (Sign);

  switch ((Commm = trans->RecvCtrlT (RecvTimeOut, BufferOut, Ctrl::CL_SERV_BUFFER_SIZE, & LenOut))) {
  case AgCom::UPDATE:
  case AgCom::END_SEANS_DEL_CONTEXT:
  case AgCom::END_SEANS:
  case AgCom::SEND_AGENT_COMMAND:
    * data = BufferOut;
    * len = LenOut;
    return Commm;
  case AgCom::KERBEROS:
    strcpy(userName, (char*)BufferOut);
    trans->SendCtrl (AgCom::IDENT);
    trans->RecvCtrlT (RecvTimeOut, BufferOut, Ctrl::CL_SERV_BUFFER_SIZE);
    strcpy(passwdName, (char *)BufferOut);
    break;
  case AgCom::IDENT:
    break;
  default:
    throw CAgentCtrlExcept (0, "После аутентификации принята неправильная команда %d", Commm);
  }

// Если попали сюда значит соедиениие с панелью управления
// Проверка запущен ли поток
  if ((GetHandle () != NULL) && (IsFinished () == false)) {
    if (IsStopingThread == false) {
      trans->SendCtrl (AgCom::BAD, (unsigned char *)"Агент не может принять второе соединение от панели управления, чтобы продолжить работу закройте первое соединение и соединитесь снова", strlen ("Агент не может принять второе соединение от панели управления, чтобы продолжить работу закройте первое соединение и соединитесь снова"));
      LogFileCtrl->print (std::string ("CAgentCtrl : Агент не может принять второе соединение от панели управления, чтобы продолжить работу закройте первое соединение и соединитесь снова\n"));
      return 0;
    }
    else {
      LogFileCtrl->print (std::string ("CAgentCtrl : Поток приема команд от Панели управления уже запущен, завершаем поток\n"));
      Stop ();
      if (WaitFor (20000) == false) {
        LogFileCtrl->print (std::string ("CAgentCtrl : Поток приема команд не реагирует на запрос завершения, завершаем принудительно\n"));
        Trans.Close ();
        Terminate ();
      }
      LogFileCtrl->print (std::string ("CAgentCtrl : Поток приема команд завершён\n"));
    }
  }

  try {
    IsAdmin = 0;
    switch (ClientType) {
    case AgCom::CLCTRL_TYPE:
		IsAdmin = CheckUser(userName, passwdName);
      break;
    case AgCom::CLCTRL_NOPASSW:
      IsAdmin = ADMIN_USER;
      break;
    }
  }
  catch (CicException & ex) {
    unsigned char Buffer[1000];
    sprintf ((char *)Buffer, "%s %s", ex.GetStrErr (), ex.GetStrErrExt ());
    trans->SendCtrl (AgCom::BAD, Buffer, strlen ((char *)Buffer) + 1);
    throw;
  }

  trans->SendCtrl(AgCom::SUC, & IsAdmin, 1);
  
  Trans.Close ();
  Trans = * trans;
  trans->DetachSock ();
  trans->DisActivateProt ();

  Close ();
  Create(false, NULL, 0);

  return 0;
}

EXCEPTION_STR(
unsigned char CAgentCtrl::CheckUser (const char * UserName, const char * Password)
)
  unsigned char Res = 0;
  char PasswordMy[200] = {0};
  char Group[200] = {0};
  WCHAR WPassword[200] = {0};
  WCHAR WUserName[200] = {0};
  LOCALGROUP_USERS_INFO_0 * pGroupUserInfo = NULL;
  LOCALGROUP_USERS_INFO_0 * pGroupUserInfoDel = NULL;
  DWORD EntriesRead = 0;
  DWORD TotalEntries = 0;
  
  
  try {
    CStrToWStr(Password, WPassword, sizeof (WPassword));
    CStrToWStr (UserName, WUserName, sizeof (WUserName));
    
    switch (NetUserGetLocalGroups (NULL, WUserName, 0, 0, (LPBYTE*) (& pGroupUserInfo), MAX_PREFERRED_LENGTH, & EntriesRead, & TotalEntries)) {
    case ERROR_ACCESS_DENIED:
      throw CAgentCtrlExcept (0, "Нет доступа к учетной записи пользователя");
    case NERR_UserNotFound:
      throw CAgentCtrlExcept (0, "Пользователь %s не найден", UserName);
    case ERROR_MORE_DATA:
      throw CAgentCtrlExcept (0, "Не хватает памяти для получения груп");
    case NERR_InvalidComputer:
      throw CAgentCtrlExcept (0, "Не найден компьютер");
    case NERR_Success:
      break;
    }
    pGroupUserInfoDel = pGroupUserInfo;
    
    for (DWORD i = 0; i < EntriesRead; i ++, pGroupUserInfo ++) {
      WStrToCStr(pGroupUserInfo->lgrui0_name, Group, sizeof (Group));
      _strupr (Group);
      if (strcmp (Group, ADMIN_GROUP) == 0)
        Res |= ADMIN_USER;
	  if (strcmp (Group, AUDIT_GROUP) == 0)
		Res |= AUDIT_USER;
	  if (strcmp (Group, USER_GROUP) == 0)
		Res |= USER_USER;
    }
    if (Res == 0)
      throw CAgentCtrlExcept (0, "Пользователь %s не является членом локальной группы %s или %s или %s", UserName, ADMIN_GROUP, USER_GROUP, AUDIT_GROUP);
	if ((Res & ADMIN_USER) == ADMIN_USER)
		Res = ADMIN_USER;
	if ((Res & AUDIT_USER) == AUDIT_USER)
		Res = AUDIT_USER;
	if ((Res & USER_USER) == USER_USER)
		Res = USER_USER;

    char User11[300] = {0};
    char Domen11[300] = {0};

    const char * pUserS = strchr (UserName, '\\');
    if (pUserS == NULL) {
      strcpy (User11, UserName);
      strcpy (Domen11, ".");
    }
    else {
      strncpy (Domen11, UserName, pUserS - UserName);
      strcpy (User11, pUserS + 1);
    }

#ifdef _SERVICE
    HANDLE HandlPass = INVALID_HANDLE_VALUE;
    if (LogonUser (User11, Domen11, (char *)Password, LOGON32_LOGON_INTERACTIVE, LOGON32_PROVIDER_DEFAULT, & HandlPass) != 0)
      CloseHandle (HandlPass);
    else
      throw CAgentCtrlExcept (0, "Пользователь %s ввел неправильный пароль GetLastError () = %d", (char *)UserName, GetLastError ());
#endif
  }
  catch (...) {
    if (pGroupUserInfoDel != NULL)
      NetApiBufferFree (pGroupUserInfoDel);
    throw;
  }
  
  if (pGroupUserInfoDel != NULL)
    NetApiBufferFree (pGroupUserInfoDel);
  
  return Res;
}

EXCEPTION_STR(
unsigned CAgentCtrl::Execute()
)
  char IpMas[50] = {0};
  unsigned char Com;
  unsigned char * Buffer1;
  unsigned long LenOut;
  unsigned long Len;

  try {
    Buffer1 = new unsigned char [66000];
    CAutoMemoryMasDelete autoMem (Buffer1);

    TransTools::GetRemoteStrIP (& Trans, IpMas);
    if (IsAdmin == 1)
      LogFileCtrl->print (std::string ("CAgentCtrl : Аутентификация учетной записи пользователя успешна, пользователь АДМИНИСТРАТОР\n"));
    else
      LogFileCtrl->print (std::string ("CAgentCtrl : Аутентификация учетной записи пользователя успешна, пользователь НАБЛЮДАТЕЛЬ\n"));
    LogFileCtrl->print ("CAgentCtrl : Запущен %s\n", IpMas);

    while (stopFlag == FALSE) {
      for (unsigned iTime = 0; (iTime < 120) && (stopFlag == FALSE) && (Trans.CheckRecv(1000) == 0); iTime ++);
      if (Trans.CheckRecv(1) == 0)
        break;

      Com = Trans.RecvCtrlT (RecvTimeOut, Buffer1, 66000, & LenOut);
      if (Com == Ctrl::CLOSE_CONNECTION)
        break;
      if (Com == Ctrl::WATCH_DOG)
        continue;

      try {
        Len = Ctrl::CL_SERV_BUFFER_SIZE;
        ExecComand (IsAdmin, & Com, Buffer1, LenOut, BufferOut, & Len, IpMas);
        Trans.SendCtrl (Com, BufferOut, Len);
        LogFileCtrl->print ("CAgentCtrl : %s : завершена, длина выходного буфера %d %s\n", GetStrComand (Com), Len, IpMas);
      }
      catch (CicException & ex) {
        sprintf ((char *)BufferOut, "%s %s", ex.GetStrErr (), ex.GetStrErrExt ());
        LogFileCtrl->print ("%sCAgentCtrl : %s %s %s\n%s\n", CLogFileDel::INFO_ERR_ADD, ex.GetStrErr (), ex.GetStrErrExt (), IpMas, ex.GetFunc());
        Trans.SendCtrl (AgCom::BAD, BufferOut, strlen ((char *)BufferOut) + 1);
        continue;
      }
    }
  }
  catch (CSockExcept & ex2) {
    LogFileCtrl->print("%sCAgentCtrl : %s %s\n", CLogFileDel::INFO_ERR_ADD, ex2.GetStrErr(), ex2.GetStrErrExt ());
  }
  catch (CicException & ex) {
    LogFileCtrl->printwrn ("CAgentCtrl : %s %s %s\n%s\n", ex.GetStrErr (), ex.GetStrErrExt (), IpMas, ex.GetFunc());
  }
  catch (std::exception & ex) {
    LogFileCtrl->printwrn ("CAgentCtrl : %s %s\n", ex.what (), IpMas);
  }

  Trans.Close (5000);
  LogFileCtrl->print ("CAgentCtrl : Завершен %s\n", IpMas);

  return 0;
}


EXCEPTION_STR(
void CAgentCtrl::ExecComand (unsigned char IsAdmin,
                                       unsigned char * com, 
                                       unsigned char * bufin,
                                       unsigned long lenin,
                                       unsigned char * bufout,
                                       unsigned long * lenout,
                                       const char * IpMas)
)
  switch ((* com)) {
  case Ctrl::GET_CONFIG_INFO:
    LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));
    GetConfigInfo (bufout, lenout);
    break;
  case Ctrl::GET_SERVER_INFO:
    LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));
    GetServerInfo (bufout, lenout);
    break;
  case Ctrl::GET_AGENT_BASE_INFO:
    {
      std::ostrstream stre((char *)bufout, * lenout);

      LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));

      stre << AgTypeCr;
      stre << ModelTypeCr;
      if (! stre.good())
        throw CStrTypeExcept (Str_Function_Name, "Ошибка заполнения  CAgTypeCreator, CModelTypeCreator, CAgStateTypeCreator");
      (* lenout) = (unsigned long)(stre.tellp());
    }
    break;
  case Ctrl::GET_AGENT_BASE:
    LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));
    GetAgentBase(bufout, lenout);
    break;
  case Ctrl::GET_FILE:
    {
      LogFileCtrl->print ("CAgentCtrl : %s : FileName = %s\n", GetStrComand (* com), (char *)bufin);
      unsigned long lenRecv = GetFile ((char *)bufin);
      if (lenRecv != 0xffffffff)
        LogFileCtrl->print ("CAgentCtrl : %s : размер переданных данных %d\n", GetStrComand (* com), lenRecv);
      else
        LogFileCtrl->print ("CAgentCtrl : %s : передача файла прервана по таймауту\n", GetStrComand (* com));
      * lenout = 0;
    }
    break;
  case Ctrl::GET_DIR:
    LogFileCtrl->print ("CAgentCtrl : %s : DirName = %s\n", GetStrComand (* com), (char *)bufin);
    GetDir (bufout, lenout);
    break;
  case Ctrl::PUT_FILE:
    LogFileCtrl->print ("CAgentCtrl : %s : FileName = %s\n", GetStrComand (* com), (char *)bufin);
    PutFileEmpty();
    * lenout = 0;
    break;
  case Ctrl::GET_HASH_FILE:
    {
      std::string NameTemp;
      LogFileCtrl->print ("CAgentCtrl : %s : FileName = %s\n", GetStrComand (* com), (char *)bufin);

      if ((NameTemp = FindFileInDirs (GetFileNameFromPath ((char *)bufin))) == "")
        throw CStrTypeExcept (Str_Function_Name, "Не найден файл %s", (char *)bufin);

      if (HashFile (NameTemp.c_str(), (char *)bufout) != 0)
        throw CStrTypeExcept (Str_Function_Name, "Ошибка подсчета контрольной суммы на файл %s", (char *)bufin);
      * lenout = 16;
    }
    break;
  case Ctrl::WRITE_LOG:
    LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));
    * lenout = 0;
    break;
  case Ctrl::GET_EXPERT_DATA:
    LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));
    bufout[0] = 0;
    bufout[1] = 0;
    * lenout = 2;
    break;
  case Ctrl::GET_GLOBAL_EVTLIST:
    LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));
    GetGlobalList(bufout, lenout);
    break;
  case Ctrl::SCAN_MAIL_DIR:
    LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));
    ScanMailDir ();
    * lenout = 0;
    break;
  case Ctrl::GET_AGENT_FILEPATH:
    {
      LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));

      std::string str = FindFileInDirs ((char *)(bufin + 2));
      memcpy(bufout, str.c_str(), str.size() + 1);
      * lenout = (str.size() + 1);

      LogFileCtrl->print ("CAgentCtrl : %s : %s\n", GetStrComand (* com), str.c_str());
    }
    break;
  case Ctrl::GET_AGENT_AGENT_NAME:
    {
      LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));

      std::string str = FindFileInDirs ((char *)(bufin + 5));
      memcpy(bufout, str.c_str(), str.size() + 1);
      * lenout = (str.size() + 1);

      LogFileCtrl->print ("CAgentCtrl : %s : %s\n", GetStrComand (* com), str.c_str());
    }
    break;
  case Ctrl::GET_MBRANGE_ALLOC:
    bufout[0] = 0;
    * lenout = 1;
    break;
  case Ctrl::GET_STATIC_NTSM_VER:
    {
      unsigned short numVer = 1;

      LogFileCtrl->print ("CAgentCtrl : %s Версия %d\n", GetStrComand (* com), numVer);

      memcpy (bufout, & numVer, 2);
      * lenout = 2;
    }
    break;
  case Ctrl::GET_LAST_LOGON_TIME: 
    {
      LogFileCtrl->print ("CAgentCtrl : %s\n", GetStrComand (* com));

	  // Проверка совместимости версий панели управления и ntsm
	  if (lenin == 0) {
		  LogFileCtrl->print("Версия совместимости панели управления 0 и агента %d не совпадает. Соединение будет закрыто", VersionConnectNTSM);
		  throw CStrExcept("Панель управления несовместима с агентом. Соединение будет закрыто");
	  }
	  unsigned versionControlPanel = *((unsigned *)bufin);
	  if (versionControlPanel != VersionConnectNTSM) {
		  LogFileCtrl->print("Версия совместимости панели управления %d и агента %d не совпадает. Соединение будет закрыто", versionControlPanel, VersionConnectNTSM);
		  throw CStrExcept("Панель управления несовместима с агентом. Соединение будет закрыто");
	  }

	  time_t timeLogon = 0;
	  memcpy (bufout, & timeLogon, 4);

	  *lenout = 8;
    }
    break;
  case Ctrl::SET_LAST_LOGON_TIME:
    LogFileCtrl->print ("%s\n", GetStrComand (* com));
    * lenout = 0;
    break;
  case Ctrl::GET_PLUGINS_EDITOR_DIR:
  case Ctrl::GET_PLUGINS_EXECUTABLE_DIR:
    LogFileCtrl->print ("%s\n", GetStrComand (* com));
    bufout[0] = 0;
    * lenout = 1;
    break;
  case Ctrl::FILL_PLUGIN_CFG:
    LogFileCtrl->print ("%s\n", GetStrComand (* com));
    * lenout = 0;
    break;
  case Ctrl::AGENT_COMMAND:
    LogFileCtrl->print ("%s\n", GetStrComand (* com));
    Trans.SendCtrl (Ctrl::AGENT_COMMAND_END);
	*lenout = 0;
	break;
  case Ctrl::IS_AGENT_PRIVATE_CONFIG:
    LogFileCtrl->print ("%s\n", GetStrComand (* com));
    bufout[0] = 0;
    *lenout = 1;
    break;
  case Ctrl::GET_PARAMTEREVT_FILE:
    LogFileCtrl->print ("%s\n", GetStrComand (* com));

    sprintf ((char *)bufout, "%s", AGENT_PARAMETER);
    * lenout = strlen ((char *)bufout) + 1;
    break;
  default:
    throw CStrTypeExcept (Str_Function_Name, "Принята неподдерживаемая команда %d", * com);
  }
}

EXCEPTION_STR(
void CAgentCtrl::GetConfigInfo (unsigned char * bufin, unsigned long * len)
)
  CConfigRecordCreator confcr;

  confcr.done ();
  CConfigRecord * conf = NULL;

  conf = new (std::nothrow) CConfigRecord (Ctrl::KeyNamePort00, "1", "1");
  if (!confcr.insert (conf) && conf != NULL)
    delete conf;

  conf = new (std::nothrow) CConfigRecord (Ctrl::KeyNameFilePort, "2", "2");
  if (!confcr.insert (conf) && conf != NULL)
    delete conf;

  conf = new (std::nothrow) CConfigRecord (Ctrl::KeyNameDogPort, "3", "3");
  if (!confcr.insert (conf) && conf != NULL)
    delete conf;

  conf = new (std::nothrow) CConfigRecord (Ctrl::KeyNamePath, "", "Путь до компонентов МБ");
  if (!confcr.insert (conf) && conf != NULL)
    delete conf;

  conf = new (std::nothrow) CConfigRecord (Ctrl::KeyNameMQManagerOutDef, "", "");
  if (!confcr.insert (conf) && conf != NULL)
    delete conf;

  conf = new (std::nothrow) CConfigRecord (Ctrl::KeyNameMQChanelOutDef, "", "");
  if (!confcr.insert (conf) && conf != NULL)
    delete conf;

  conf = new (std::nothrow) CConfigRecord (Ctrl::KeyNameMQIPOutDef, "", "");
  if (!confcr.insert (conf) && conf != NULL)
    delete conf;

  conf = new (std::nothrow) CConfigRecord (Ctrl::KeyNameMQPortOutDef, "1", "1");
  if (!confcr.insert (conf) && conf != NULL)
    delete conf;

  std::ostrstream stre((char *)bufin, * len);
  stre << confcr;
  if (! stre.good())
    throw CStrTypeExcept (Str_Function_Name, "Ошибка заполнения CConfigRecordCreator");
  (* len) = (unsigned long)(stre.tellp());
}

EXCEPTION_STR(
void CAgentCtrl::GetServerInfo (unsigned char * bufin, unsigned long * len)
)
  CServerInfo Serv;

  Serv.type = CServerInfo::AGENT;
  Serv.heap = BazaRecord.AgentNum.AgentNum;

  memset (Serv.description, 0, sizeof (Serv.description));
  strncpy (Serv.description, BazaRecord.Name, sizeof (Serv.description) - 1);

  unsigned long IP = ntohl (CTrans::GetIP ());
  CTrans::GetStrIP (IP, Serv.IPAddress);

  strncpy (Serv.version, Version.c_str(), sizeof (Serv.version) - 1);

  std::ostrstream stre((char *)bufin, * len);
  stre << Serv;
  if (! stre.good())
    throw CStrTypeExcept (Str_Function_Name, "Ошибка заполнения CServerInfo");
  (* len) = (unsigned long)(stre.tellp());
}

EXCEPTION_STR(
void CAgentCtrl::GetAgentBase (unsigned char * bufin, unsigned long * lenout)
)
  CAgentDumpRecord * agrec;
  CAgentDumpRecordCreator agcr;

  if ((agrec = new (std::nothrow) CAgentDumpRecord) == NULL)
    throw CStrTypeExcept (Str_Function_Name, "Ошибка размещения CAgentDumpRecord");

  agrec->number = BazaRecord.AgentNum.AgentNum;           //  2 байта - номер агента
  strcpy (agrec->description, BazaRecord.Name);           //  0-terminated string - имя агента
  sprintf (agrec->dirname, "%04X", BazaRecord.AgentNum.AgentNum);         //  0-terminated string - индивидуальный каталог агента в базе агентов
  agrec->ostype = BazaRecord.OsType;                      //  1 байт - тип агента
  agrec->model = BazaRecord.Model;                        //  1 байт - модель анализа аудита от агента
  agrec->state = CBazaNt::States::ASTATE_NORMAL_MODE;                        //  1 байт - состояние агента в базе агентов
  strcpy (agrec->eventlistname, BazaRecord.EventListName);//  0-terminated string - список событий, фиксируемый агентом
  agrec->ipaddress = BazaRecord.AgentIp;                  //  4 байта - IP-адрес, закрепленный за агентом
  agrec->portA = BazaRecord.MbPort;                       //  2 байта порт агента для связи с МБ
  agrec->portF = BazaRecord.AgentPort;                    //  2 байта порт агента для приема файлов
  agrec->ipaddressmb = BazaRecord.MbIp;                   //  4 байта - IP-адрес для связи с МБ
  agrec->ISNotPing = BazaRecord.IsNotPing;   // 0 - пинговать в statemanager (NT mb only)
  agrec->portD = BazaRecord.AgentPlatformPort;
  strcpy (agrec->Otvetstvenuy, BazaRecord.Otvetstvenuy);
  agrec->IsAgentCh = 0;
  strcpy (agrec->version, Version.c_str());

  // Это неправильно но необходимо чтобы записывать настройки MQSeries и IsNotPing
  if (((agrec->advEventsListNames) = new (std::nothrow) CStrList) == NULL)
    throw CStrTypeExcept (Str_Function_Name, "Ошибка размещения advEventsListNames");

  if (! agcr.insert (agrec))
      throw CStrTypeExcept (Str_Function_Name, "Ошибка вставки CAgentDumpRecord");

  std::ostrstream stre((char *)bufin, * lenout);
  stre << agcr;
  if (! stre.good())
    throw CStrTypeExcept (Str_Function_Name, "Ошибка заполнения CAgentDumpRecordCreator");
  (* lenout) = (unsigned long)(stre.tellp());
}

EXCEPTION_STR(
unsigned long CAgentCtrl::GetFile (const char * fileName)
)
  unsigned char Com[2] = {AgCom::BEGIN_FILE, AgCom::BAD};
  std::string FileName = GetFileNameFromPath (fileName);
  std::string PathName;
  std::string Maska;

  CModelType * m = (CModelType *)ModelTypeCr.getByFirstField (0, BazaRecord.Model);
  if (m == NULL)
    throw CStrTypeExcept (Str_Function_Name, "Неизвестная модель анализа %d", BazaRecord.Model);
  Maska = "*.pcf.";
  Maska += m->mod_suffix;

  PathName = FindFileInDirs (FileName);

  if ((PathName == "") && (FindMaskNoCase(FileName.c_str(), Maska.c_str()) == true)) {
    FileName = FileName.substr(0, FileName.find_last_of ('.'));
    PathName = FindFileInDirs (FileName);
  }

  if (PathName == "")
    PathName = fileName;

  return TransTools::SendFile2 (Com, & Trans, PathName.c_str());
}

EXCEPTION_STR(
void CAgentCtrl::PutFileEmpty ()
)
  unsigned char Buffer[20000];
  unsigned long rlen;

  for (rlen = 1; rlen != 0;)
    Trans.RecvCtrlT(RecvTimeOut, Buffer, sizeof (Buffer), & rlen);

  throw CStrTypeExcept (Str_Function_Name, "Команда PUT_FILE не разрешена для выполнения");
}

EXCEPTION_STR(
void CAgentCtrl::GetDir (unsigned char * bufin, unsigned long * len)
)
  std::list <FileDirTools::CFileData> ListFile;
  std::list <FileDirTools::CFileData>::iterator Itr;
  CDirectoryEntryCreator DirCr;
  CDirectoryEntry * dir;
  std::string TempStr;

  CModelType * m = (CModelType *)ModelTypeCr.getByFirstField (0, BazaRecord.Model);
  if (m == NULL)
    throw CStrTypeExcept (Str_Function_Name, "Неизвестная модель анализа %d", BazaRecord.Model);

  if (AgentCurrentDir != "") {
    ListFile.clear();

    TempStr = AgentCurrentDir;
    if (FileDirTools::IsEndSlash(TempStr.c_str()) == true)
      TempStr = TempStr.substr(0, TempStr.size() - 1);

    ListFile = FileDirTools::FindAllWays (TempStr.c_str(), FALSE);

    for (Itr = ListFile.begin(); Itr != ListFile.end (); Itr ++) {
      if (Itr->IsDir() == true)
        continue;

      if (FindMaskNoCase ((Itr->GetName()).c_str(), "*.pcf") == true) {
        TempStr = Itr->GetName();
        TempStr += ".";
        TempStr += m->mod_suffix;
      }
      else
        TempStr = Itr->GetName();

      dir = new (std::nothrow) CDirectoryEntry (TempStr.c_str(), (unsigned long)(Itr->GetSize()), Itr->GetCreateTimeUnix(), CDirectoryEntry::S_FILE, "");
      if (dir == NULL)
        throw CStrTypeExcept (Str_Function_Name, "Ошибка выделения памяти под CDirectoryEntry");

      if (! DirCr.insert(dir))
        throw CStrTypeExcept (Str_Function_Name, "Ошибка вставки CDirectoryEntry в CDirectoryEntryCreator");
    }
  }

  if (AgentDataDir != "") {
    ListFile.clear();

    TempStr = AgentDataDir;
    if (FileDirTools::IsEndSlash(TempStr.c_str()) == true)
      TempStr = TempStr.substr(0, TempStr.size() - 1);

    ListFile = FileDirTools::FindAllWays (TempStr.c_str(), FALSE);

    for (Itr = ListFile.begin(); Itr != ListFile.end (); Itr ++) {
      if (Itr->IsDir() == true)
        continue;

      if (FindMaskNoCase ((Itr->GetName()).c_str(), "*.pcf") == true) {
        TempStr = Itr->GetName();
        TempStr += ".";
        TempStr += m->mod_suffix;
      }
      else
        TempStr = Itr->GetName();

      dir = new (std::nothrow) CDirectoryEntry (TempStr.c_str(), (unsigned long)(Itr->GetSize()), Itr->GetCreateTimeUnix(), CDirectoryEntry::S_FILE, "");
      if (dir == NULL)
        throw CStrTypeExcept (Str_Function_Name, "Ошибка выделения памяти под CDirectoryEntry");

      if (! DirCr.insert(dir))
        throw CStrTypeExcept (Str_Function_Name, "Ошибка вставки CDirectoryEntry в CDirectoryEntryCreator");
    }
  }

  if (AgentMailDir != "") {
    ListFile.clear();

    TempStr = AgentMailDir;
    if (FileDirTools::IsEndSlash(TempStr.c_str()) == true)
      TempStr = TempStr.substr(0, TempStr.size() - 1);

    ListFile = FileDirTools::FindAllWays (TempStr.c_str(), FALSE);

    for (Itr = ListFile.begin(); Itr != ListFile.end (); Itr ++) {
      if (Itr->IsDir() == true)
        continue;

      if (FindMaskNoCase ((Itr->GetName()).c_str(), "*.pcf") == true) {
        TempStr = Itr->GetName();
        TempStr += ".";
        TempStr += m->mod_suffix;
      }
      else
        TempStr = Itr->GetName();

      dir = new (std::nothrow) CDirectoryEntry (TempStr.c_str(), (unsigned long)(Itr->GetSize()), Itr->GetCreateTimeUnix(), CDirectoryEntry::S_FILE, "");
      if (dir == NULL)
        throw CStrTypeExcept (Str_Function_Name, "Ошибка выделения памяти под CDirectoryEntry");

      if (! DirCr.insert(dir))
        throw CStrTypeExcept (Str_Function_Name, "Ошибка вставки CDirectoryEntry в CDirectoryEntryCreator");
    }
  }

  std::ostrstream stre ((char *)bufin, * len);
  stre << DirCr;
  if (!stre.good())
    throw CStrTypeExcept (Str_Function_Name, "Ошибка заполнения информации о директории");
  * len = (unsigned long)(stre.tellp());
}

EXCEPTION_STR(
void CAgentCtrl::GetGlobalList (unsigned char * bufin, unsigned long * len)
)
  ParametrEvtParser Pars;
  std::string TempName;
  CEventList EvList;
  CStr * str;

  TempName = FindFileInDirs(AGENT_PARAMETER);
  if (TempName == "")
    throw CStrTypeExcept (Str_Function_Name, "Не найден файл со списком событий %s", AGENT_PARAMETER);

  Pars.Parse(TempName);

  for (unsigned i = 0; i < Pars.Events.size (); i ++) {
    if ((str = new (std::nothrow) CStr ((Pars.Events[i].Name).c_str ())) == NULL)
      throw CStrTypeExcept (Str_Function_Name, "Ошибка выделения памяти под CStr");
    if (! EvList.insert(str))
      throw CStrTypeExcept (Str_Function_Name, "Ошибка вставки в CEventList");
  }

  std::ostrstream strm((char*)bufin, * len);
  strm.seekp (0, std::ios::beg);
  strm << EvList;
  if (! strm.good())
    throw CStrTypeExcept (Str_Function_Name, "Ошибка заполнения глобального списка strm.good () = 0");
  (* len) = (unsigned long)(strm.tellp());
}

void CAgentCtrl::ScanMailDir ()
{
  std::list <FileDirTools::CFileData> ListFile;
  std::list <FileDirTools::CFileData>::iterator Itr;
  std::string TempStr;
  std::string PostExtMask;
  unsigned char Command[2] = {AgCom::BEGIN_FILE, AgCom::BAD};

  if ((MailPotok != NULL) && (MailPotok->GetHandle() != NULL)) {
    LogFileCtrl->print (std::string ("CAgentCtrl : Остановка почтового потока\n"));
    MailPotok->Stop ();
    if (MailPotok->WaitFor(30000) == false)
      LogFileCtrl->print (std::string ("CAgentCtrl : Почтовый поток не остановился в течении 30 секунд, продолжаем выполнение\n"));
    else
      LogFileCtrl->print (std::string ("CAgentCtrl : Почтовый поток остановлен\n"));
  }
  else
    LogFileCtrl->print (std::string ("CAgentCtrl : Почтовый поток не запущен\n"));

  PostExtMask = "*.";
  PostExtMask += PostExt;

  if (AgentCurrentDir != "") {
    TempStr = AgentCurrentDir;
    if (FileDirTools::IsEndSlash(TempStr.c_str()) == true)
      TempStr = TempStr.substr(0, TempStr.size() - 1);

    ListFile = FileDirTools::FindAllWays (TempStr.c_str(), FALSE);

    for (Itr = ListFile.begin(); Itr != ListFile.end (); Itr ++) {
      if (Itr->IsDir() == true)
        continue;

      if (FindMaskNoCase ((Itr->GetName()).c_str(), PostExtMask.c_str()) == true) {
        Trans.SendCtrl(AgCom::BEGIN_FILE, (unsigned char *)((Itr->GetName()).c_str()), strlen ((Itr->GetName()).c_str()) + 1);
        TransTools::SendFile1(Command, & Trans, (Itr->GetFullPath()).c_str());
        DeleteFile ((Itr->GetFullPath()).c_str());
      }
    }
  }

  if (AgentDataDir != "") {
    TempStr = AgentDataDir;
    if (FileDirTools::IsEndSlash(TempStr.c_str()) == true)
      TempStr = TempStr.substr(0, TempStr.size() - 1);

    ListFile = FileDirTools::FindAllWays (TempStr.c_str(), FALSE);

    for (Itr = ListFile.begin(); Itr != ListFile.end (); Itr ++) {
      if (Itr->IsDir() == true)
        continue;

      if (FindMaskNoCase ((Itr->GetName()).c_str(), PostExtMask.c_str()) == true) {
        Trans.SendCtrl(AgCom::BEGIN_FILE, (unsigned char *)((Itr->GetName()).c_str()), strlen ((Itr->GetName()).c_str()) + 1);
        TransTools::SendFile1(Command, & Trans, (Itr->GetFullPath()).c_str());
        DeleteFile ((Itr->GetFullPath()).c_str());
      }
    }
  }

  if (AgentMailDir != "") {
    TempStr = AgentMailDir;
    if (FileDirTools::IsEndSlash(TempStr.c_str()) == true)
      TempStr = TempStr.substr(0, TempStr.size() - 1);

    ListFile = FileDirTools::FindAllWays (TempStr.c_str(), FALSE);

    for (Itr = ListFile.begin(); Itr != ListFile.end (); Itr ++) {
      if (Itr->IsDir() == true)
        continue;

      if (FindMaskNoCase ((Itr->GetName()).c_str(), PostExtMask.c_str()) == true) {
        Trans.SendCtrl(AgCom::BEGIN_FILE, (unsigned char *)((Itr->GetName()).c_str()), strlen ((Itr->GetName()).c_str()) + 1);
        TransTools::SendFile1(Command, & Trans, (Itr->GetFullPath()).c_str());
        DeleteFile ((Itr->GetFullPath()).c_str());
      }
    }
  }
}

const char * CAgentCtrl::GetStrComand (unsigned char Com)
{
  switch (Com) {
  case AgCom::AUTH:
    return "AUTH";
  case Ctrl::GET_AGENT_BASE:
    return "GET_AGENT_BASE";
  case Ctrl::GET_AGENT_BASE_INFO:
    return "GET_AGENT_BASE_INFO";
  case Ctrl::PREPARE_AGENT: 
    return "PREPARE_AGENT";
  case Ctrl::DELETE_AGENT:
    return "DELETE_AGENT";
  case Ctrl::UNINSTALL_AGENT:
    return "UNINSTALL_AGENT";
  case Ctrl::REINSTALL_AGENT: 
    return "REINSTALL_AGENT";
  case Ctrl::READY_AGENT:
    return "READY_AGENT";
  case Ctrl::UPDATE_AGENT:
    return "UPDATE_AGENT";
  case Ctrl::COMPILE:
    return "COMPILE";
  case Ctrl::GET_GLOBAL_EVTLIST:
    return "GET_GLOBAL_EVTLIST";
  case Ctrl::GET_CONFIG_INFO:
    return "GET_CONFIG_INFO";
  case Ctrl::GET_SERVER_INFO:
    return "GET_SERVER_INFO";
  case Ctrl::REINSTALL_NEEDS:
    return "REINSTALL_NEEDS";
  case Ctrl::GET_FILE:
    return "GET_FILE";
  case Ctrl::GET_FILE_INFO:
    return "GET_FILE_INFO";
  case Ctrl::RENAME_FILE: 
    return "RENAME_FILE";
  case Ctrl::DELETE_FILE:
    return "DELETE_FILE";
  case Ctrl::PUT_FILE:
    return "PUT_FILE";
  case Ctrl::MAKE_DIR: 
    return "MAKE_DIR";
  case Ctrl::GET_DIR:
    return "GET_DIR";
  case Ctrl::GET_DIR_EXT:
    return "GET_DIR_EXT";
  case Ctrl::GET_DIR_EXT2:
    return "GET_DIR_EXT2";
  case Ctrl::GET_EXPERT_DATA:
    return "GET_EXPERT_DATA";
  case Ctrl::EXPORT_BAZA_REC:
    return "EXPORT_BAZA_REC";
  case Ctrl::IMPORT_BAZA_REC:
    return "IMPORT_BAZA_REC";
  case Ctrl::REREAD_CONFIG:
    return "REREAD_CONFIG";
  case Ctrl::SEND_AGENT_FILES_ERR:
    return "SEND_AGENT_FILES_ERR";
  case Ctrl::GET_WSSTATE:
    return "GET_WSSTATE";
  case Ctrl::GET_SHEDULER_DATA:
    return "GET_SHEDULER_DATA";
  case Ctrl::ADD_SHEDULER_DATA:
    return "ADD_SHEDULER_DATA";
  case Ctrl::DEL_SHEDULER_DATA:
    return "DEL_SHEDULER_DATA";
  case Ctrl::CHANGE_SHEDULER_DATA:
    return "CHANGE_SHEDULER_DATA";
  case Ctrl::SAVE_OUT_BUFFER:
    return "SAVE_OUT_BUFFER";
  case Ctrl::SCAN_MAIL_DIR:
    return "SCAN_MAIL_DIR"; 
  case AgCom::BEGIN_FILE:
    return "BEGIN_FILE";
  case Ctrl::AGENT_INSTAL:
    return "AGENT_INSTAL";
  case Ctrl::AGENT_DEINSTALL:
    return "AGENT_DEINSTALL";
  case Ctrl::GET_MEDIA_SOURCE:
    return "GET_MEDIA_SOURCE";
  case Ctrl::ADD_MEDIA_SOURCE:
    return "ADD_MEDIA_SOURCE";
  case Ctrl::DEL_MEDIA_SOURCE:
    return "DEL_MEDIA_SOURCE";
  case Ctrl::CH_PRIOR_MEDIA_SOURCE:
    return "CH_PRIOR_MEDIA_SOURCE";
  case Ctrl::CHANGE_SEANS:
    return "CHANGE_SEANS";
  case Ctrl::AUTENT_UPDATE:
    return "AUTENT_UPDATE";
  case Ctrl::COPY_FILE_DIR:
    return "COPY_FILE_DIR";
  case Ctrl::WRITE_LOG:
    return "WRITE_LOG";
  case Ctrl::GET_HASH_FILE:
    return "GET_HASH_FILE";
  case Ctrl::GET_AGENT_FILEPATH:
    return "GET_AGENT_FILEPATH";
  case Ctrl::GET_AGENT_AGENT_NAME:
    return "GET_AGENT_AGENT_NAME";
  case Ctrl::GET_MBRANGE_ALLOC:
    return "GET_MBRANGE_ALLOC";
  case Ctrl::GET_STATIC_NTSM_VER:
    return "GET_STATIC_NTSM_VER";
  case Ctrl::GET_LAST_LOGON_TIME:
    return "GET_LAST_LOGON_TIME";
  case Ctrl::SET_LAST_LOGON_TIME:
    return "SET_LAST_LOGON_TIME";
  case Ctrl::GET_PLUGINS_EDITOR_DIR:
    return "GET_PLUGINS_EDITOR_DIR";
  case Ctrl::GET_PLUGINS_EXECUTABLE_DIR:
    return "GET_PLUGINS_EXECUTABLE_DIR";
  case Ctrl::FILL_PLUGIN_CFG:
    return "FILL_PLUGIN_CFG";
  case Ctrl::AGENT_COMMAND:
    return "AGENT_COMMAND";
  case Ctrl::IS_AGENT_PRIVATE_CONFIG:
    return "IS_AGENT_PRIVATE_CONFIG";
  case Ctrl::GET_PARAMTEREVT_FILE:
    return "GET_PARAMTEREVT_FILE";
  default:
    return "НЕИЗВЕСТНАЯ КОМАНДА";
  }
}

std::string CAgentCtrl::GetFileNameFromPath (const std::string & path)
{
  int Find1;
  int Find2;

  Find1 = path.find_last_of('\\');
  Find2 = path.find_last_of('/');

  if ((Find1 == std::string::npos) && (Find2 == std::string::npos))
    return path;


  return path.substr (((Find1 > Find2) ? Find1 : Find2) + 1);
}

std::string CAgentCtrl::FindFileInDirs (const std::string & filename)
{
  std::string PathName;

  if (AgentCurrentDir != "") {
    PathName = AgentCurrentDir;
    if (FileDirTools::IsEndSlash(PathName.c_str()) == false)
      PathName += "\\";
    PathName += filename;
    if (FileDirTools::IsFileExsist (PathName.c_str()) == true)
      return PathName;
  }
  if (AgentDataDir != "") {
    PathName = AgentDataDir;
    if (FileDirTools::IsEndSlash(PathName.c_str()) == false)
      PathName += "\\";
    PathName += filename;
    if (FileDirTools::IsFileExsist (PathName.c_str()) == true)
      return PathName;
  }
  if (AgentMailDir != "") {
    PathName = AgentMailDir;
    if (FileDirTools::IsEndSlash(PathName.c_str()) == false)
      PathName += "\\";
    PathName += filename;
    if (FileDirTools::IsFileExsist (PathName.c_str()) == true)
      return PathName;
  }

  return "";
}
