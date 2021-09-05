#ifdef NTSM_DEF
#pragma warning(disable : 4786)

#include <fcntl.h>
#include "boost/algorithm/string/case_conv.hpp"
#include "filedirtools.h"
#endif

#include <errno.h>
#include "bazant.h"
#include "stdfile.h"
#include "getguid.h"

#define WRITE_MAS(var) memcpy(Mas + NumB, var, sizeof (var));NumB += sizeof(var)
#define WRITE_VAR(var) memcpy(Mas + NumB, & var, sizeof (var));NumB += sizeof(var)
#define READ_MAS(var) memcpy(var, Mas + NumB, sizeof (var));NumB += sizeof(var)
#define READ_VAR(var) memcpy(& var, Mas + NumB, sizeof (var));NumB += sizeof(var)
#define RESET_MAS(var) memset(var, 0, sizeof (var))
#define RESET_VAR(var) memset(& var, 0, sizeof (var))

const CBazaNt::TypeOs CBazaNt::OS_WINNT         (CBazaNt::TypeOs::OS_WINNT, "Windows NT/2000", "raw.nt");
const CBazaNt::TypeOs CBazaNt::OS_SM            (CBazaNt::TypeOs::OS_SM, "Подчиненная МБ", "raw.sm");
const CBazaNt::TypeOs CBazaNt::OS_NOVELL        (CBazaNt::TypeOs::OS_NOVELL, "Novell NetWare 4.xx/5.x", "raw.nw");
const CBazaNt::TypeOs CBazaNt::OS_EMBEDED       (CBazaNt::TypeOs::OS_EMBEDED, "Встраиваемый", "raw.ea");
const CBazaNt::TypeOs CBazaNt::OS_DOS           (CBazaNt::TypeOs::OS_DOS, "MS-DOS", "raw.dos");
const CBazaNt::TypeOs CBazaNt::OS_HPUX          (CBazaNt::TypeOs::OS_HPUX, "HP-UX 11.x", "raw.hp");
const CBazaNt::TypeOs CBazaNt::OS_ORACLE        (CBazaNt::TypeOs::OS_ORACLE, "ORACLE для HP-UX 11.x", "raw.or");
const CBazaNt::TypeOs CBazaNt::OS_WORACLE       (CBazaNt::TypeOs::OS_WORACLE, "ORACLE для Windows NT", "raw.wo");
const CBazaNt::TypeOs CBazaNt::OS_SYSLOG        (CBazaNt::TypeOs::OS_SYSLOG, "SysLog для Windows NT", "raw.sl");
const CBazaNt::TypeOs CBazaNt::OS_SECRETNET     (CBazaNt::TypeOs::OS_SECRETNET, "SecretNet", "raw.sn");
const CBazaNt::TypeOs CBazaNt::OS_SPECTRZ       (CBazaNt::TypeOs::OS_SPECTRZ, "SpectrZ", "raw.sz");
const CBazaNt::TypeOs CBazaNt::OS_WIN_LITE      (CBazaNt::TypeOs::OS_WIN_LITE, "Lite для Windows", "raw.lw");
const CBazaNt::TypeOs CBazaNt::OS_SOL_LITE      (CBazaNt::TypeOs::OS_SOL_LITE, "Solaris для Windows", "raw.sol");
const CBazaNt::TypeOs CBazaNt::OS_INFORMIX_LITE (CBazaNt::TypeOs::OS_INFORMIX_LITE, "Informix для Windows", "raw.infx");
const CBazaNt::TypeOs CBazaNt::OS_AIX_LITE      (CBazaNt::TypeOs::OS_AIX_LITE, "Aix для Windows", "raw.aix");
const CBazaNt::TypeOs CBazaNt::OS_HPUX1131      (CBazaNt::TypeOs::OS_HPUX1131, "HP-UX 11.31", "raw.hp1131");
const CBazaNt::TypeOs CBazaNt::OS_HPUX1131_RISC (CBazaNt::TypeOs::OS_HPUX1131_RISC, "HP-UX 11.31 RISC", "raw.hp1131risc");
const CBazaNt::TypeOs CBazaNt::OS_UNKNOWN       (CBazaNt::TypeOs::OS_UNKNOWN, "Неизвестный", "raw.unknown");

const CBazaNt::States CBazaNt::ASTATE_NORMAL_MODE      (CBazaNt::States::ASTATE_NORMAL_MODE,       "Работает",              "");
const CBazaNt::States CBazaNt::ASTATE_TRANSFERED       (CBazaNt::States::ASTATE_TRANSFERED,        "Получил обновление",    "");
const CBazaNt::States CBazaNt::ASTATE_PREPARE          (CBazaNt::States::ASTATE_PREPARE,           "Готов к инсталляции",   "Подготовить к инсталляции");
const CBazaNt::States CBazaNt::ASTATE_UPDATE_NEED      (CBazaNt::States::ASTATE_UPDATE_NEED,       "Обновляется",           "Обновить агента");
const CBazaNt::States CBazaNt::ASTATE_REINSTALL_NEED   (CBazaNt::States::ASTATE_REINSTALL_NEED,    "Заблокирован",          "");
const CBazaNt::States CBazaNt::ASTATE_MUST_BE_DELETED  (CBazaNt::States::ASTATE_MUST_BE_DELETED,   "Готов к деинсталляции", "Разрешить деинсталляцию");
const CBazaNt::States CBazaNt::ASTATE_EXPORT           (CBazaNt::States::ASTATE_EXPORT,            "Экспортирован",         "Экспортировать учетную запись");
const CBazaNt::States CBazaNt::ASTATE_UPDATE_PERMIT    (CBazaNt::States::ASTATE_UPDATE_PERMIT,     "Готов к обновлению",    "Разрешить обновление");
const CBazaNt::States CBazaNt::ASTATE_DELETE_AGENT     (CBazaNt::States::ASTATE_DELETE_AGENT,      "Удален",                "Подтвердить деинсталляцию");
const CBazaNt::States CBazaNt::ASTATE_INSTALL_AGENT    (CBazaNt::States::ASTATE_INSTALL_AGENT,     "Установлен",            "Инсталлировать");
const CBazaNt::States CBazaNt::ASTATE_CREATE_AGENT     (CBazaNt::States::ASTATE_CREATE_AGENT,      "Создается",             "");
const CBazaNt::States CBazaNt::CHANGE_SEANS            (CBazaNt::States::CHANGE_SEANS,             "",                      "Смена сеанса");
const CBazaNt::States CBazaNt::CHANGE_SEANS_DEL_CONTEXT(CBazaNt::States::CHANGE_SEANS_DEL_CONTEXT, "",                      "Смена сеанса с удалением контекста");
const CBazaNt::States CBazaNt::DEINSTALL_AGENT         (CBazaNt::States::DEINSTALL_AGENT,          "",                      "Деинсталлировать");
const CBazaNt::States CBazaNt::DELETE_AGENT_UZ         (CBazaNt::States::DELETE_AGENT_UZ,          "",                      "Удалить учетную запись");
const CBazaNt::States CBazaNt::BACKUP_AGENT_UZ         (CBazaNt::States::BACKUP_AGENT_UZ,          "",                      "Откатить к предыдущему состоянию");
const CBazaNt::States CBazaNt::IMPORT_AGENT_UZ         (CBazaNt::States::IMPORT_AGENT_UZ,          "",                      "Импортировать учетную запись");
const CBazaNt::States CBazaNt::SEND_AGENT_COMMAND      (CBazaNt::States::SEND_AGENT_COMMAND,       "",                      "");
const CBazaNt::States CBazaNt::SET_STATE_INSTALL_AGENT (CBazaNt::States::SET_STATE_INSTALL_AGENT,  "",                      "Подтвердить инсталляцию");
const CBazaNt::States CBazaNt::PREPARE_NEW_AGENT       (CBazaNt::States::PREPARE_NEW_AGENT,        "",                      "Создать учетную запись");
const CBazaNt::States CBazaNt::RESET_WRN_ERR           (CBazaNt::States::RESET_WRN_ERR,            "",                      "Сбросить предупреждения и ошибки");
const CBazaNt::States CBazaNt::ADD_RANGE_NUM           (CBazaNt::States::ADD_RANGE_NUM,            "",                      "Добавить диапозон номеров");
const CBazaNt::States CBazaNt::CHECK_AGENT_PLATFORM    (CBazaNt::States::CHECK_AGENT_PLATFORM,     "",                      "Проверить агентскую платформу");
const CBazaNt::States CBazaNt::GET_LOGS                (CBazaNt::States::GET_LOGS,                 "",                      "Получить журналы работы");


CBazaNt::CBazaNtExcept::CBazaNtExcept (int err, const StringType & type, const char * fs, ...) throw ()
{
  va_list ap;

  va_start (ap, fs);
  Create (type.FunctionName, err, fs, ap);
  va_end (ap);
}

const char * CBazaNt::CBazaNtExcept::GetStrErr() const throw ()
{
  switch (GetErr ()) {
    case ERR_OPEN:
      return "Ошибка открытия файла базы";
    case ERR_SET_SECURITY:
      return "Ошибка установки атрибутов security";
    case NAME_EXIST:
      return "Агент с таким именем уже существует базе";
    case ERR_WRITE:
      return "Ошибка записи в базу";
    case ERR_READ:
      return "Ошибка чтения из базы";
    case ERR_REMOVE:
      return "Ошибка удаления файла базы агентов";
    case ERR_RENAME:
      return "Ошибка переименования файла базы агентов";
    case NOT_NUMBER:
      return "Нет больше номеров для агентов. Для добавления агента необходимо добавить новый диапозон номеров";
    case NUMBER_NOT_FOUND:
      return "Агент с указанным номером в базе не найден";
    case ERR_MB_NUMBER:
      return "Номер МБ в базе не совпадает со значением инициирующем базу";
    case BAD_ADD_NUMBER:
      return "Номер для добавления в базу неправильный";
    case IP_EXIST:
      return "Агент с таким IP адресом и портом уже существует в базе";
    case NUMBER_EXIST:
      return "Искажение базы, агент с таким номером уже существует в базе";
    case NOT_MEMORY_DUMP_STATE:
      return "Не хватает памяти для размешения дампа состояния агентов";
    case DUMP_STATE_INVALID:
      return "Не верный формат дампа состояния";
    case ERR_MB_NUMBER_RANGE:
      return "Номер МБ в базе диапозонов не совпадает со значением инициирующем базу";
    case ERR_OPEN_RANGE:
      return "Ошибка открытия файла базы диапозонов";
    case ERR_EXIST_AGENTPLATFORM_PORT:
      return "Порт агентской платформы совпадает с портом агента";
    case NOT_RANGE:
      return "Невозможно выделить диапазон номеров для агентов. Отсутствуют свободные диапазоны.";
    case CRASH_RANGE_FILE:
      return "Диапозоны созданные в agentbaza не доступны для создания (не заданы в agentbazarange). Возможно файл agentbazarange поврежден.";
    case GUID_NOT_EXIST:
      return "Отсутствует GUID МБ или файл с GUID МБ поврежден";
  default:
    return "?";
  }
}

CBazaNt::DataBaseMem::DataBaseMem ()
{
  RESET_VAR (AgState);
  RESET_VAR (IPAg);
  RESET_VAR (LastEvent);
  RESET_VAR (TimeShutDown);
  RESET_VAR (TimeUpDown);
  RESET_VAR (AgentPort);
}

void CBazaNt::DataBaseMem::Read (const unsigned char * Mas)
{
  unsigned long NumB = 0;

  READ_VAR (AgState);
  READ_VAR (IPAg);
  READ_VAR (LastEvent);
  READ_VAR (TimeShutDown);
  READ_VAR (TimeUpDown);
  READ_VAR (AgentPort);
}

unsigned long CBazaNt::DataBaseMem::Write (unsigned char * Mas) const
{
  unsigned long NumB = 0;

  WRITE_VAR (AgState);
  WRITE_VAR (IPAg);
  WRITE_VAR (LastEvent);
  WRITE_VAR (TimeShutDown);
  WRITE_VAR (TimeUpDown);
  WRITE_VAR (AgentPort);

  return NumB;
}

CBazaNt::RecBase::RecBase ()
{
  RESET_VAR (AgentNum);
  RESET_VAR (AgentIp);
  RESET_VAR (AgentPort);
  RESET_VAR (MbIp);
  RESET_VAR (MbPort);
  RESET_VAR (State);
  RESET_VAR (OsType);
  RESET_VAR (ConnectCount);
  RESET_VAR (Model);
  RESET_VAR (IsNotPing);
  RESET_VAR (IsLock);
  RESET_VAR (StateBackup);
  RESET_MAS (Reserv2);
  RESET_VAR (LastSeans);
  RESET_MAS (GUIDMB);
  RESET_MAS (Reserv1);
  RESET_MAS (HashFiles);
  RESET_MAS (SerNum);
  RESET_MAS (Name);
  RESET_MAS (EventListName);
  RESET_MAS (AllHashFiles);
  RESET_MAS (GUIDAgent);
  RESET_VAR (AgentPlatformPort);
  RESET_MAS (Otvetstvenuy);
  RESET_MAS (SerNumBackup);
  RESET_VAR (SessionID);
  RESET_VAR (SessionIP);
  RESET_MAS (SessionUser);
  RESET_MAS (OSUser);
  RESET_MAS (ComputerName);
  RESET_MAS (MBName);
  RESET_MAS (Reserv4);
  RESET_VAR (RangeMBNumber);
  RESET_MAS (Reserv);
}

void CBazaNt::RecBase::Read (const unsigned char * Mas)
{
  unsigned long NumB = 0;
  
  READ_VAR (AgentNum);
  READ_VAR (AgentIp);
  READ_VAR (AgentPort);
  READ_VAR (MbIp);
  READ_VAR (MbPort);
  READ_VAR (State);
  READ_VAR (OsType);
  READ_VAR (ConnectCount);
  READ_VAR (Model);
  READ_VAR (IsNotPing);
  READ_VAR (IsLock);
  READ_VAR (StateBackup);
  READ_MAS (Reserv2);
  READ_VAR (LastSeans);
  READ_MAS (GUIDMB);
  READ_MAS (Reserv1);
  READ_MAS (HashFiles);
  READ_MAS (SerNum);
  READ_MAS (Name);
  READ_MAS (EventListName);
  READ_MAS (AllHashFiles);
  READ_MAS (GUIDAgent);
  READ_VAR (AgentPlatformPort);
  READ_MAS (Otvetstvenuy);
  READ_MAS (SerNumBackup);
  READ_VAR (SessionID);
  READ_VAR (SessionIP);
  READ_MAS (SessionUser);
  READ_MAS (OSUser);
  READ_MAS (ComputerName);
  READ_MAS (MBName);
  READ_MAS (Reserv4);
  READ_VAR (RangeMBNumber);
  READ_MAS (Reserv);
}

unsigned long CBazaNt::RecBase::Write (unsigned char * Mas) const
{
  unsigned long NumB = 0;
  
  WRITE_VAR (AgentNum);
  WRITE_VAR (AgentIp);
  WRITE_VAR (AgentPort);
  WRITE_VAR (MbIp);
  WRITE_VAR (MbPort);
  WRITE_VAR (State);
  WRITE_VAR (OsType);
  WRITE_VAR (ConnectCount);
  WRITE_VAR (Model);
  WRITE_VAR (IsNotPing);
  WRITE_VAR (IsLock);
  WRITE_VAR (StateBackup);
  WRITE_MAS (Reserv2);
  WRITE_VAR (LastSeans);
  WRITE_MAS (GUIDMB);
  WRITE_MAS (Reserv1);
  WRITE_MAS (HashFiles);
  WRITE_MAS (SerNum);
  WRITE_MAS (Name);
  WRITE_MAS (EventListName);
  WRITE_MAS (AllHashFiles);
  WRITE_MAS (GUIDAgent);
  WRITE_VAR (AgentPlatformPort);
  WRITE_MAS (Otvetstvenuy);
  WRITE_MAS (SerNumBackup);
  WRITE_VAR (SessionID);
  WRITE_VAR (SessionIP);
  WRITE_MAS (SessionUser);
  WRITE_MAS (OSUser);
  WRITE_MAS (ComputerName);
  WRITE_MAS (MBName);
  WRITE_MAS (Reserv4);
  WRITE_VAR (RangeMBNumber);
  WRITE_MAS (Reserv);
  
  return NumB;
}

CBazaNt::RecBaseAdv::RecBaseAdv()
{
  RESET_MAS (ModelDesc);
  RESET_MAS (ModelName);
  RESET_MAS (AgentDesc);
  RESET_MAS (AgentDir);
  RESET_MAS (Reserv);
}

void CBazaNt::RecBaseAdv::Read(const unsigned char * Mas)
{
  unsigned long NumB = 0;

  READ_MAS (ModelDesc);
  READ_MAS (ModelName);
  READ_MAS (AgentDesc);
  READ_MAS (AgentDir);
  READ_MAS (Reserv);
}

unsigned long CBazaNt::RecBaseAdv::Write (unsigned char * Mas) const
{
  unsigned long NumB = 0;

  WRITE_MAS (ModelDesc);
  WRITE_MAS (ModelName);
  WRITE_MAS (AgentDesc);
  WRITE_MAS (AgentDir);
  WRITE_MAS (Reserv);

  return NumB;
}

EXCEPTION_STR(
void CBazaNt::WriteRecAdvFile (RecBaseAdv * rec, const char * fileName)
)
  CStdFILE File;
  unsigned char TempMas[sizeof (RecBase)];
  unsigned long Pos = RecBase().Write (TempMas);
  unsigned char Mas[sizeof (RecBaseAdv)];

  if (File.fopen (fileName, "r+b") == false)
    throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN, Str_Function_Name, "%s (%s)", fileName, strerror (errno));
  File.fseek (Pos, SEEK_SET);

  unsigned long LenRec = rec->Write (Mas);
  if (File.fwrite (Mas, LenRec) != LenRec)
    throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, fileName);
}

EXCEPTION_STR(
void CBazaNt::ReadRecAdvFile (RecBaseAdv * rec, const char * fileName)
)
  CStdFILE File;
  unsigned char TempMas[sizeof (RecBase)];
  unsigned long Pos = RecBase().Write (TempMas);
  unsigned char Mas[sizeof (RecBaseAdv)];

  if (File.fopen (fileName, "rb") == false)
    throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN, Str_Function_Name, "%s (%s)", fileName, strerror (errno));
  File.fseek (Pos, SEEK_SET);

  unsigned long LenRec = rec->Write (Mas);
  if (File.fread (Mas, LenRec) != LenRec)
    throw CBazaNtExcept (CBazaNtExcept::ERR_READ, Str_Function_Name, fileName);
  rec->Read (Mas);
}

EXCEPTION_STR(
void CBazaNt::WriteRecFile (RecBase * rec, const char * fileName)
)
  CStdFILE File;
  unsigned char Mas[sizeof (RecBase)];
  if (File.fopen (fileName, "wb") == false)
    throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN, Str_Function_Name, "%s (%s)", fileName, strerror (errno));
  unsigned long LenRec = rec->Write (Mas);
  if (File.fwrite (Mas, LenRec) != LenRec)
    throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, fileName);
}

EXCEPTION_STR(
void CBazaNt::ReadRecFile (RecBase * rec, const char * fileName)
)
  CStdFILE File;
  unsigned char Mas[sizeof (RecBase)];
  unsigned long LenRec = rec->Write (Mas);

  if (File.fopen (fileName, "rb") == false)
    throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN, Str_Function_Name, "%s (%s)", fileName, strerror (errno));
  if (File.fread (Mas, LenRec) != LenRec)
    throw CBazaNtExcept (CBazaNtExcept::ERR_READ, Str_Function_Name, fileName);
  rec->Read (Mas);
}

EXCEPTION_STR(
void CBazaNt::ReadMemInBuf (const unsigned char * buf, unsigned long lenbuf, std::map<unsigned short, CBazaNt::DataBaseMem> & mapState)
)
  unsigned short agNum;
  CBazaNt::DataBaseMem dateMem;
  unsigned char tmpBuf[sizeof(CBazaNt::DataBaseMem)];
  unsigned long lenRec = dateMem.Write(tmpBuf) + 2;

  if ((lenbuf % lenRec) != 0)
    throw CBazaNtExcept (CBazaNtExcept::DUMP_STATE_INVALID, Str_Function_Name);

  mapState.clear();

  for (unsigned i = 0; i < lenbuf / lenRec; i ++) {
    agNum = *((unsigned short *)(buf + (i * lenRec)));
    dateMem.Read(buf + (i * lenRec) + 2);

    mapState[agNum] = dateMem;
  }
}

bool CBazaNt::IsNotWorkAgentState (unsigned char State)
{
  switch (State) {
    case CBazaNt::States::ASTATE_PREPARE:
    case CBazaNt::States::ASTATE_REINSTALL_NEED:
    case CBazaNt::States::ASTATE_DELETE_AGENT:
    case CBazaNt::States::ASTATE_CREATE_AGENT:
      return true;
    default:
      return false;
  }
}

unsigned char CBazaNt::IsWorkAgentState (unsigned char State, unsigned char SerNumBaza[16], unsigned char SerNumAgent[16])
{
  if (State == CBazaNt::States::ASTATE_UPDATE_NEED)
    return 0;

  switch (State) {
  case CBazaNt::States::ASTATE_NORMAL_MODE:
  case CBazaNt::States::ASTATE_TRANSFERED:
  case CBazaNt::States::ASTATE_PREPARE:
  case CBazaNt::States::ASTATE_MUST_BE_DELETED:
  case CBazaNt::States::ASTATE_UPDATE_PERMIT:
  case CBazaNt::States::ASTATE_INSTALL_AGENT:
    if (memcmp (SerNumBaza, SerNumAgent, 16) == 0)
      return 0;
    else
      return 2;
  default:
    return 1;
  }
}

bool CBazaNt::IsNextAgentState (unsigned char curState, CBazaNt::States::Const nextState, bool isMB)
{
  std::vector <CBazaNt::States::Const> spisState = GetAvaibleAgentStates (curState, isMB);

  for (unsigned i = 0; i < spisState.size(); i ++) {
    if (spisState[i] == nextState)
      return true;
  }

  return false;
}

std::vector <CBazaNt::States::Const> CBazaNt::GetAvaibleAgentStates (unsigned char curState, bool isMB)
{
  std::vector <CBazaNt::States::Const> spisState;

  if (isMB == true) {
    spisState.push_back (CBazaNt::States::ADD_RANGE_NUM);
    spisState.push_back (CBazaNt::States::RESET_WRN_ERR);

    if (curState == CBazaNt::States::ASTATE_PREPARE) {
      spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
      spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
      spisState.push_back (CBazaNt::States::ASTATE_NORMAL_MODE);
      spisState.push_back (CBazaNt::States::ASTATE_PREPARE);

      return spisState;
    }
    if (curState == CBazaNt::States::ASTATE_NORMAL_MODE) {
      spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
      spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
      spisState.push_back (CBazaNt::States::ASTATE_PREPARE);

      return spisState;
    }
    if (curState == CBazaNt::States::ASTATE_MUST_BE_DELETED) {
      spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
      spisState.push_back (CBazaNt::States::ASTATE_DELETE_AGENT);

      return spisState;
    }
    if (curState == CBazaNt::States::ASTATE_DELETE_AGENT) {
      spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
      spisState.push_back (CBazaNt::States::ASTATE_PREPARE);
      spisState.push_back (CBazaNt::States::DELETE_AGENT_UZ);

      return spisState;
    }
  }

  spisState.push_back (CBazaNt::States::IMPORT_AGENT_UZ);
  spisState.push_back (CBazaNt::States::RESET_WRN_ERR);
  spisState.push_back (CBazaNt::States::CHECK_AGENT_PLATFORM);

  if (curState == CBazaNt::States::ASTATE_NORMAL_MODE) {
    spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
    spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
    spisState.push_back (CBazaNt::States::ASTATE_UPDATE_PERMIT);
    spisState.push_back (CBazaNt::States::CHANGE_SEANS);
    spisState.push_back (CBazaNt::States::CHANGE_SEANS_DEL_CONTEXT);
    spisState.push_back (CBazaNt::States::SEND_AGENT_COMMAND);
    spisState.push_back (CBazaNt::States::GET_LOGS);

    return spisState;
  }
  if (curState == CBazaNt::States::ASTATE_TRANSFERED) {
    spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
    spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
    spisState.push_back (CBazaNt::States::ASTATE_EXPORT);
    spisState.push_back (CBazaNt::States::ASTATE_NORMAL_MODE);
    spisState.push_back (CBazaNt::States::GET_LOGS);

    return spisState;
  }
  if (curState == CBazaNt::States::ASTATE_PREPARE) {
    spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
    spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
    spisState.push_back (CBazaNt::States::ASTATE_NORMAL_MODE);
    spisState.push_back (CBazaNt::States::ASTATE_PREPARE);
    spisState.push_back (CBazaNt::States::ASTATE_INSTALL_AGENT);
    spisState.push_back (CBazaNt::States::SET_STATE_INSTALL_AGENT);

    return spisState;
  }
  if (curState == CBazaNt::States::ASTATE_UPDATE_NEED) {
    spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
    spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
    spisState.push_back (CBazaNt::States::ASTATE_TRANSFERED);
    spisState.push_back (CBazaNt::States::ASTATE_UPDATE_NEED);
    spisState.push_back (CBazaNt::States::GET_LOGS);

    return spisState;
  }
  if (curState == CBazaNt::States::ASTATE_REINSTALL_NEED) {
    spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
    spisState.push_back (CBazaNt::States::GET_LOGS);

    return spisState;
  }
  if (curState == CBazaNt::States::ASTATE_MUST_BE_DELETED) {
    spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
    spisState.push_back (CBazaNt::States::ASTATE_DELETE_AGENT);
    spisState.push_back (CBazaNt::States::BACKUP_AGENT_UZ);
    spisState.push_back (CBazaNt::States::CHANGE_SEANS);
    spisState.push_back (CBazaNt::States::CHANGE_SEANS_DEL_CONTEXT);
    spisState.push_back (CBazaNt::States::SEND_AGENT_COMMAND);
    spisState.push_back (CBazaNt::States::DEINSTALL_AGENT);
    spisState.push_back (CBazaNt::States::GET_LOGS);

    return spisState;
  }
  if (curState == CBazaNt::States::ASTATE_UPDATE_PERMIT) {
    spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
    spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
    spisState.push_back (CBazaNt::States::ASTATE_UPDATE_NEED);
    spisState.push_back (CBazaNt::States::BACKUP_AGENT_UZ);
    spisState.push_back (CBazaNt::States::CHANGE_SEANS);
    spisState.push_back (CBazaNt::States::CHANGE_SEANS_DEL_CONTEXT);
    spisState.push_back (CBazaNt::States::SEND_AGENT_COMMAND);
    spisState.push_back (CBazaNt::States::GET_LOGS);

    return spisState;
  }
  if (curState == CBazaNt::States::ASTATE_DELETE_AGENT) {
    spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
    spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
    spisState.push_back (CBazaNt::States::ASTATE_PREPARE);
    spisState.push_back (CBazaNt::States::DELETE_AGENT_UZ);

    return spisState;
  }
  if (curState == CBazaNt::States::ASTATE_INSTALL_AGENT) {
    spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
    spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
    spisState.push_back (CBazaNt::States::ASTATE_NORMAL_MODE);
    spisState.push_back (CBazaNt::States::GET_LOGS);

    return spisState;
  }
  if (curState == CBazaNt::States::ASTATE_CREATE_AGENT) {
  	spisState.push_back (CBazaNt::States::PREPARE_NEW_AGENT);
  	spisState.push_back (CBazaNt::States::DELETE_AGENT_UZ);
    return spisState;
  }

  throw CStrExcept ("Для определения перехода из состояния агента в другие состояния передано неизвестное состояние %d", curState);
}

std::vector <CBazaNt::States::Const> CBazaNt::GetAllAgentStates ()
{
  std::vector <CBazaNt::States::Const> spisState;

  spisState.push_back (CBazaNt::States::ASTATE_NORMAL_MODE);
  spisState.push_back (CBazaNt::States::ASTATE_TRANSFERED);
  spisState.push_back (CBazaNt::States::ASTATE_PREPARE);
  spisState.push_back (CBazaNt::States::ASTATE_UPDATE_NEED);
  spisState.push_back (CBazaNt::States::ASTATE_REINSTALL_NEED);
  spisState.push_back (CBazaNt::States::ASTATE_MUST_BE_DELETED);
  spisState.push_back (CBazaNt::States::ASTATE_EXPORT);
  spisState.push_back (CBazaNt::States::ASTATE_UPDATE_PERMIT);
  spisState.push_back (CBazaNt::States::ASTATE_DELETE_AGENT);
  spisState.push_back (CBazaNt::States::ASTATE_INSTALL_AGENT);
  spisState.push_back (CBazaNt::States::ASTATE_CREATE_AGENT);
  spisState.push_back (CBazaNt::States::DELETE_AGENT_UZ);
  spisState.push_back (CBazaNt::States::BACKUP_AGENT_UZ);
  spisState.push_back (CBazaNt::States::IMPORT_AGENT_UZ);
  spisState.push_back (CBazaNt::States::CHANGE_SEANS);
  spisState.push_back (CBazaNt::States::CHANGE_SEANS_DEL_CONTEXT);
  spisState.push_back (CBazaNt::States::SEND_AGENT_COMMAND);
  spisState.push_back (CBazaNt::States::DEINSTALL_AGENT);
  spisState.push_back (CBazaNt::States::SET_STATE_INSTALL_AGENT);
  spisState.push_back (CBazaNt::States::PREPARE_NEW_AGENT);
  spisState.push_back (CBazaNt::States::RESET_WRN_ERR);
  spisState.push_back (CBazaNt::States::ADD_RANGE_NUM);
  spisState.push_back (CBazaNt::States::CHECK_AGENT_PLATFORM);
  spisState.push_back (CBazaNt::States::GET_LOGS);

  return spisState;
}

const char * CBazaNt::GetStrAgentState (unsigned char state)
{
  switch (state) {
    case CBazaNt::States::ASTATE_NORMAL_MODE:
      return CBazaNt::ASTATE_NORMAL_MODE.Name;
    case CBazaNt::States::ASTATE_TRANSFERED:
      return ASTATE_TRANSFERED.Name;
    case CBazaNt::States::ASTATE_PREPARE:
      return ASTATE_PREPARE.Name;
    case CBazaNt::States::ASTATE_UPDATE_NEED:
      return ASTATE_UPDATE_NEED.Name;
    case CBazaNt::States::ASTATE_REINSTALL_NEED:
      return ASTATE_REINSTALL_NEED.Name;
    case CBazaNt::States::ASTATE_MUST_BE_DELETED:
      return ASTATE_MUST_BE_DELETED.Name;
    case CBazaNt::States::ASTATE_EXPORT:
      return ASTATE_EXPORT.Name;
    case CBazaNt::States::ASTATE_UPDATE_PERMIT:
      return ASTATE_UPDATE_PERMIT.Name;
    case CBazaNt::States::ASTATE_DELETE_AGENT:
      return ASTATE_DELETE_AGENT.Name;
    case CBazaNt::States::ASTATE_INSTALL_AGENT:
      return ASTATE_INSTALL_AGENT.Name;
    case CBazaNt::States::ASTATE_CREATE_AGENT:
      return ASTATE_CREATE_AGENT.Name;
    default:
      return "Неизвестное состояние";
  }
}

CBazaNt::States CBazaNt::GetStateDesc (CBazaNt::States::Const state)
{
  States st (state, "Неизвестное состояние", "Неизвестная команда");

  switch (state) {
    case CBazaNt::States::ASTATE_NORMAL_MODE: 
      return CBazaNt::ASTATE_NORMAL_MODE;
    case CBazaNt::States::ASTATE_TRANSFERED:
      return ASTATE_TRANSFERED;
    case CBazaNt::States::ASTATE_PREPARE:
      return ASTATE_PREPARE;
    case CBazaNt::States::ASTATE_UPDATE_NEED:
      return ASTATE_UPDATE_NEED;
    case CBazaNt::States::ASTATE_REINSTALL_NEED:
      return ASTATE_REINSTALL_NEED;
    case CBazaNt::States::ASTATE_MUST_BE_DELETED:
      return ASTATE_MUST_BE_DELETED;
    case CBazaNt::States::ASTATE_EXPORT:
      return ASTATE_EXPORT;
    case CBazaNt::States::ASTATE_UPDATE_PERMIT:
      return ASTATE_UPDATE_PERMIT;
    case CBazaNt::States::ASTATE_DELETE_AGENT:
      return ASTATE_DELETE_AGENT;
    case CBazaNt::States::ASTATE_INSTALL_AGENT:
      return ASTATE_INSTALL_AGENT;
    case CBazaNt::States::ASTATE_CREATE_AGENT:
      return ASTATE_CREATE_AGENT;
    case CBazaNt::States::CHANGE_SEANS:
      return CHANGE_SEANS;
    case CBazaNt::States::CHANGE_SEANS_DEL_CONTEXT:
      return CHANGE_SEANS_DEL_CONTEXT;
    default:
      return st;
  }
}

#ifdef NTSM_DEF
CBazaNt::CBazaNt()
{
  BazaName[0] = 0;
}

EXCEPTION_STR(
void CBazaNt::Init (const char * bName, const char * semName, unsigned char numMb)
)
  SECURITY_ATTRIBUTES sa;
  SECURITY_DESCRIPTOR sd;

 if (!InitializeSecurityDescriptor (& sd, SECURITY_DESCRIPTOR_REVISION))
   throw CBazaNtExcept (CBazaNtExcept::ERR_SET_SECURITY, Str_Function_Name, "Error InitializeSecurityDescriptor");
 if(!SetSecurityDescriptorDacl (& sd, TRUE, (PACL) NULL, FALSE))
   throw CBazaNtExcept (CBazaNtExcept::ERR_SET_SECURITY, Str_Function_Name, "Error SetSecurityDescriptorDacl");
  sa.nLength = sizeof(sa);
  sa.lpSecurityDescriptor = &sd;
  sa.bInheritHandle = FALSE;

  Semaphore.OpenSem (semName, & sa);
  CTakeSem Sem (& Semaphore);

  GetReadRanges (bName, numMb);

  CStdFILE File;
  if (File.fopen (bName, "rb") == false) {
    if (File.fopen (bName, "wb") == false)
      throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN, Str_Function_Name, "%s (%s)", (char *)bName, strerror (errno));
    File.disablebuf ();
    Numbers Num;
    Num.NumC.NumMb = numMb;
    Num.NumC.NumAgent = 0;
    if (File.fwrite (&(Num.AgentNum), 2) != 2)
      throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, "ошибка записи номера агента");
    File.fclose ();
    if (File.fopen (bName, "rb") == false)
      throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN, Str_Function_Name, "%s (%s)", (char *)bName, strerror (errno));
  }

  if (File.fread (& (MaxAgentNum.AgentNum), 2) != 2)
    throw CBazaNtExcept (CBazaNtExcept::ERR_READ, Str_Function_Name, "ошибка чтения номера агента");
  if (numMb != MaxAgentNum.NumC.NumMb)
    throw CBazaNtExcept (CBazaNtExcept::ERR_MB_NUMBER, Str_Function_Name, "номер в базе %04X, номер инициирущий базу %04X", MaxAgentNum.NumC.NumMb, numMb);

  RecBase rec;
  unsigned char RecMas[sizeof (RecBase)];
  const unsigned long LenRec = rec.Write (RecMas);
  unsigned long FileSize = File.getfilesize ();

  for (unsigned short i = 0; i < ((FileSize - 2) / LenRec); i ++) {
    if (File.fread (RecMas, LenRec) != LenRec)
      throw CBazaNtExcept (CBazaNtExcept::ERR_READ, Str_Function_Name);
    rec.Read (RecMas);
    if (BazaSpis.find (rec.AgentNum.AgentNum) != BazaSpis.end ())
      throw CBazaNtExcept (CBazaNtExcept::NUMBER_EXIST, Str_Function_Name, "number %04X", rec.AgentNum.AgentNum);
    if (FindName (rec.Name) != 0) 
      throw CBazaNtExcept (CBazaNtExcept::NAME_EXIST, Str_Function_Name, "name %s", rec.Name);
//    if (FindIP (rec.AgentIp) != 0) 
//      throw CBazaNtExcept (CBazaNtExcept::IP_EXIST, Str_Function_Name, "%X", ntohl (rec.AgentIp));

    BazaSpis.insert (TBazaSpis::value_type (rec.AgentNum.AgentNum, rec));
  }
  strcpy (BazaName, bName);

  CheckRange ();
  ReadMBGUID (bName);
}

EXCEPTION_STR(
void CBazaNt::ReadInFile ()
)
  CTakeSem Sem (& Semaphore);
  unsigned short num1;
  CStdFILE File;

  BazaSpis.clear();

  if (File.fopen (BazaName, "rb") == false)
    throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN, Str_Function_Name, "%s (%s)", (char *)BazaName, strerror (errno));

  if (File.fread (& num1, 2) != 2)
    throw CBazaNtExcept (CBazaNtExcept::ERR_READ, Str_Function_Name, "номера МБ из agentbaza (файл меньше 2 байт или ошибка доступа)");

  RecBase rec;
  unsigned char RecMas[sizeof (RecBase)];
  const unsigned long LenRec = rec.Write (RecMas);
  unsigned long FileSize = File.getfilesize ();

  for (unsigned short i = 0; i < ((FileSize - 2) / LenRec); i ++) {
    if (File.fread (RecMas, LenRec) != LenRec)
      throw CBazaNtExcept (CBazaNtExcept::ERR_READ, Str_Function_Name, "записи агента из agentbaza (не прочитан размер записи или ошибка доступа)");
    rec.Read (RecMas);
    BazaSpis.insert (TBazaSpis::value_type (rec.AgentNum.AgentNum, rec));
  }
}

EXCEPTION_STR(
unsigned short CBazaNt::Add (const RecBase * rec, bool isImport)
)
  unsigned char range;
  CTakeSem Sem (& Semaphore);

  Numbers Num;

  if (isImport == false) {
    if (rec->OsType == CBazaNt::TypeOs::OS_SM) {
      range = GetFreeRange (false);
      Num.AgentNum = range;

      if (Num.AgentNum == 0)
        throw CBazaNtExcept (CBazaNtExcept::NOT_RANGE, Str_Function_Name);
    }
    else {
      if ((Num.AgentNum = GetFreeNumOnMB ()) == 0) {
        if (AddRange (MaxAgentNum.NumC.NumMb) == 0)
          throw CBazaNtExcept (CBazaNtExcept::NOT_RANGE, Str_Function_Name);

        if ((Num.AgentNum = GetFreeNumOnMB ()) == 0)
          throw CBazaNtExcept (CBazaNtExcept::NOT_NUMBER, Str_Function_Name);
      }
    }
  }
  else {
    Num.AgentNum = rec->AgentNum.AgentNum;
    if (BazaSpis.find (Num.AgentNum) != BazaSpis.end ())
      throw CBazaNtExcept (CBazaNtExcept::NUMBER_EXIST, Str_Function_Name, "number %04X", Num.AgentNum);
  }

  if (FindName (rec->Name) != 0) 
    throw CBazaNtExcept (CBazaNtExcept::NAME_EXIST, Str_Function_Name, "name %s", rec->Name);
  if (FindIP (Num.AgentNum, rec->AgentIp, rec->AgentPort) != 0)
    throw CBazaNtExcept (CBazaNtExcept::IP_EXIST, Str_Function_Name, "(порт %d). Необходимо изменить порт", rec->AgentPort);
  if (rec->AgentPort == rec->AgentPlatformPort)
    throw CBazaNtExcept (CBazaNtExcept::ERR_EXIST_AGENTPLATFORM_PORT, Str_Function_Name, "(порт %d). Необходимо изменить порт агента", rec->AgentPort);

  RecBase recadd = (* rec);
  recadd.AgentNum.AgentNum = Num.AgentNum;
  if (rec->OsType == CBazaNt::TypeOs::OS_SM)
    recadd.RangeMBNumber = range;


  BazaSpis.insert (TBazaSpis::value_type (recadd.AgentNum.AgentNum, recadd));
  try {
    FlushBaza ();
  }
  catch (...) {
    TBazaSpis::iterator itSpBaza = BazaSpis.find (recadd.AgentNum.AgentNum);
    if (itSpBaza != BazaSpis.end ())
      BazaSpis.erase (itSpBaza);
    throw;
  }

  return Num.AgentNum;
}

EXCEPTION_STR(
void CBazaNt::Delete (unsigned short agNum) 
)
  CTakeSem Sem (& Semaphore);
  
  TBazaSpis::iterator itSpBaza = BazaSpis.find (agNum);
  if (itSpBaza == BazaSpis.end ())
    throw CBazaNtExcept (CBazaNtExcept::NUMBER_NOT_FOUND, Str_Function_Name, "номер агента = %04X", agNum);

  RecBase recsave = (itSpBaza->second);
  BazaSpis.erase (itSpBaza);

  try {
    FlushBaza ();
  }
  catch (...) {
    BazaSpis.insert (TBazaSpis::value_type (recsave.AgentNum.AgentNum, recsave));
    throw;
  }
}

EXCEPTION_STR(
void CBazaNt::Read (unsigned short agNum, RecBase * rec)
)
  CTakeSem Sem (& Semaphore);

  TBazaSpis::iterator itSpBaza = BazaSpis.find (agNum);
  if (itSpBaza == BazaSpis.end ())
    throw CBazaNtExcept (CBazaNtExcept::NUMBER_NOT_FOUND, Str_Function_Name, "номер агента = %04X", agNum);
  (* rec) = itSpBaza->second;
}

unsigned short CBazaNt::ReadSpis (unsigned short * agNum, RecBase * rec)
{
  CTakeSem Sem (& Semaphore);

  if (* agNum == 0xffff)
    return 0;

  TBazaSpis::iterator itSpBaza;
  for (unsigned short i = (* agNum) + 1; ; i ++) {
    itSpBaza = BazaSpis.find (i);
    if (itSpBaza != BazaSpis.end ()) {
      if (itSpBaza->second.OsType == CBazaNt::TypeOs::OS_RANGEMB)
        continue;
      if (itSpBaza->second.State == CBazaNt::States::ASTATE_EXPORT)
        continue;

      (* rec) = itSpBaza->second;
      (* agNum) = i;
      return i;
    }
    if (i == 0xffff)
      break;
  }

  return 0;
}

EXCEPTION_STR(
void CBazaNt::Write (const RecBase * rec, const RecBase * recold)
)
  CTakeSem Sem (& Semaphore);
  RecBase recw;
  RecBase recsave;

  TBazaSpis::iterator itSpBaza = BazaSpis.find (rec->AgentNum.AgentNum);
  if (itSpBaza == BazaSpis.end ())
    throw CBazaNtExcept (CBazaNtExcept::NUMBER_NOT_FOUND, Str_Function_Name, "номер агента = %04X", rec->AgentNum.AgentNum);

  unsigned short agnum = FindName (rec->Name);
  if ((agnum > 0) && (agnum != (rec->AgentNum.AgentNum)))
    throw CBazaNtExcept (CBazaNtExcept::NAME_EXIST, Str_Function_Name, "name %s", rec->Name);
  if (FindIP (rec->AgentNum.AgentNum, rec->AgentIp, rec->AgentPort) != 0)
    throw CBazaNtExcept (CBazaNtExcept::IP_EXIST, Str_Function_Name, "(порт %d). Необходимо изменить порт", rec->AgentPort);
  if (rec->AgentPort == rec->AgentPlatformPort)
    throw CBazaNtExcept (CBazaNtExcept::ERR_EXIST_AGENTPLATFORM_PORT, Str_Function_Name, "(порт %d). Необходимо изменить порт агента", rec->AgentPort);

  recsave = (itSpBaza->second);

  if (recold != NULL) {
    unsigned char Mas[sizeof (RecBase)];
    unsigned char MasOld[sizeof (RecBase)];
    unsigned char MasWr[sizeof (RecBase)];
    unsigned i;

    recsave.Write (MasWr);
    rec->Write (Mas);
    recold->Write (MasOld);

    for (i = 0; i < sizeof (Mas); i ++)
      MasWr[i] = (Mas[i] != MasOld[i]) ? Mas[i] : MasWr[i];
    recw.Read (MasWr);

    (recsave.datamem).Write (MasWr);
    (rec->datamem).Write (Mas);
    (recold->datamem).Write (MasOld);
    for (i = 0; i < sizeof (Mas); i ++)
      MasWr[i] = (Mas[i] != MasOld[i]) ? Mas[i] : MasWr[i];
    (recw.datamem).Read (MasWr);
  }
  else
    recw = (* rec);

  (itSpBaza->second) = recw;
  try {
    FlushBaza ();
  }
  catch (...) {
    (itSpBaza->second) = recsave;
    throw;
  }
}

EXCEPTION_STR(
void CBazaNt::WriteMem (unsigned short agNum, const DataBaseMem * rec, const DataBaseMem * recold)
)
  CTakeSem Sem (& Semaphore);
  DataBaseMem recw;
  RecBase recbase;

  TBazaSpis::iterator itSpBaza = BazaSpis.find (agNum);
  if (itSpBaza == BazaSpis.end ())
    throw CBazaNtExcept (CBazaNtExcept::NUMBER_NOT_FOUND, Str_Function_Name, "номер агента = %04X", agNum);

  recbase = itSpBaza->second;
  recw = recbase.datamem;
  if (recold != NULL) {
    unsigned char Mas[sizeof (DataBaseMem)];
    unsigned char MasOld[sizeof (DataBaseMem)];
    unsigned char MasWr[sizeof (DataBaseMem)];

    recw.Write (MasWr);
    rec->Write (Mas);
    recold->Write (MasOld);

    for (unsigned i = 0; i < sizeof (Mas); i ++)
      MasWr[i] = (Mas[i] != MasOld[i]) ? Mas[i] : MasWr[i];
    recw.Read (MasWr);

    recbase.datamem = recw;
  }
  else
    recbase.datamem = (* rec);

  (itSpBaza->second) = recbase;
}

unsigned char CBazaNt::GetMbNum () const throw ()
{
  return (MaxAgentNum.NumC.NumMb);
}

unsigned short CBazaNt::FindName (const char * name) 
{
  TBazaSpis::iterator itSpBaza;
  RecBase rec;

  for (itSpBaza = BazaSpis.begin (); itSpBaza != BazaSpis.end (); itSpBaza ++) {
    if (itSpBaza->second.OsType == CBazaNt::TypeOs::OS_RANGEMB)
      continue;
    if (strcmp ((itSpBaza->second).Name, name) == 0)
      return (itSpBaza->first);
  }
  return 0;
}

unsigned short CBazaNt::FindIP (unsigned short agnum, unsigned long IP, unsigned short port) 
{
  TBazaSpis::iterator itSpBaza;
  RecBase rec;

  if (IP == 0)
    return 0;

  for (itSpBaza = BazaSpis.begin (); itSpBaza != BazaSpis.end (); itSpBaza ++) {
    if ((agnum != itSpBaza->first) && (itSpBaza->second.State != States::ASTATE_EXPORT) && (IP == (itSpBaza->second).AgentIp) && (port == (itSpBaza->second).AgentPort))
      return (itSpBaza->first);
  }

  return 0;
}

EXCEPTION_STR(
void CBazaNt::FlushBaza ()
)
  RecBase rec;
  CStdFILE File;
  char TempBazaName [MAX_PATH];
  unsigned char Mas[sizeof (RecBase)];
  TBazaSpis::iterator itSpBaza;
  unsigned long LenRec;

  sprintf (TempBazaName, "%s.tmp", BazaName);
  if (File.fopen (TempBazaName, "wb") == false)
    throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN, Str_Function_Name, "%s (%s), изменения в базе не сохранены", BazaName, strerror (errno));

  try {
    File.disablebuf ();
    
    if (File.fwrite (& MaxAgentNum.AgentNum, 2) != 2)
      throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, "изменения в базе не сохранены");
    
    for (itSpBaza = BazaSpis.begin (); itSpBaza != BazaSpis.end (); itSpBaza ++) {
      LenRec = (itSpBaza->second).Write (Mas);
      if (File.fwrite (Mas, LenRec) != LenRec)
        throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, "изменения в базе не сохранены");
    }
    File.fclose ();
    SetFileAttributes (BazaName, FILE_ATTRIBUTE_NORMAL);
    if ((remove (BazaName) != 0) && (errno != ENOENT))
      throw CBazaNtExcept (CBazaNtExcept::ERR_REMOVE, Str_Function_Name, "%s, изменения в базе не сохранены", strerror (errno));

    for (unsigned g = 0; g < 3; g ++) {
      try {
        SetFileAttributes (BazaName, FILE_ATTRIBUTE_NORMAL);
        SetFileAttributes (TempBazaName, FILE_ATTRIBUTE_NORMAL);
        if (rename (TempBazaName, BazaName) != 0)
          throw CBazaNtExcept (CBazaNtExcept::ERR_RENAME, Str_Function_Name, "%s, изменения в базе не сохранены", strerror (errno));
        break;
      }
      catch (CBazaNtExcept & ex) {
        if (g == 2)
          throw ex;
        Sleep (300);
      }
    }
  }
  catch (...) {
    File.fclose ();
    SetFileAttributes (TempBazaName, FILE_ATTRIBUTE_NORMAL);
    remove (TempBazaName);
    throw;
  }
}

std::string CBazaNt::GetGroupFromName (const CBazaNt::RecBase & rec)
{
  std::string::size_type pos;
  std::string tmpstr = rec.Name;

  pos = tmpstr.find(".");
  if (pos == std::string::npos)
    return "";

  tmpstr = tmpstr.substr (pos + 1);
  boost::to_upper(tmpstr, std::locale("Russian")); 

  return tmpstr;
}

std::string CBazaNt::GetGroup (unsigned short agNum)
{
  CTakeSem Sem (& Semaphore);
  TBazaSpis::iterator itSpBaza;

  for (itSpBaza = BazaSpis.begin (); itSpBaza != BazaSpis.end (); itSpBaza ++) {
    if (itSpBaza->first == agNum)
      return GetGroupFromName(itSpBaza->second);
  }

  return "";
}

std::vector<unsigned short> CBazaNt::GetNumFromGroup (const std::string & group)
{
  CTakeSem Sem (& Semaphore);
  TBazaSpis::iterator itSpBaza;
  std::vector<unsigned short> vecNum;
  std::string tempGroup = group;

  boost::to_upper(tempGroup, std::locale("Russian")); 

  for (itSpBaza = BazaSpis.begin (); itSpBaza != BazaSpis.end (); itSpBaza ++) {
    if (GetGroup(itSpBaza->first) == tempGroup)
      vecNum.push_back(itSpBaza->first);
  }

  return vecNum;
}

EXCEPTION_STR(
bool CBazaNt::IsAgentMB (unsigned short agNum)
)
  CTakeSem Sem (& Semaphore);

  if (agNum == 0)
    return true;

  TBazaSpis::iterator itSpBaza = BazaSpis.find (agNum);
  if (itSpBaza == BazaSpis.end ())
    throw CBazaNtExcept (CBazaNtExcept::NUMBER_NOT_FOUND, Str_Function_Name, "номер агента = %04X", agNum);
  return ((itSpBaza->second.OsType == CBazaNt::TypeOs::OS_SM) ? true : false);
}

EXCEPTION_STR(
unsigned long CBazaNt::WriteMemInBuf (unsigned char * buf, unsigned long lenbuf)
)
  CTakeSem Sem (& Semaphore);
  TBazaSpis::iterator itSpBaza;
  unsigned long lenWrite = 0;
  unsigned long estimateSize = BazaSpis.size() * (sizeof (DataBaseMem) + 2);

  if (estimateSize > lenbuf)
    throw CBazaNtExcept (CBazaNtExcept::NOT_MEMORY_DUMP_STATE, Str_Function_Name, "размер дампа состояния %d", estimateSize);

  for (itSpBaza = BazaSpis.begin (); itSpBaza != BazaSpis.end (); itSpBaza ++) {
    memcpy (buf + lenWrite, &(itSpBaza->first), 2);
    lenWrite += 2;
    lenWrite += (itSpBaza->second.datamem.Write(buf + lenWrite));
  }

  return lenWrite;
}

EXCEPTION_STR(
void CBazaNt::GetReadRanges (const char * bName, unsigned char numMB)
)
  CStdFILE fileRange;
  std::string rangeName = bName;
  rangeName += "range";
  unsigned char numMBRead;

  if (fileRange.fopen (rangeName.c_str(), "rb") == false)
    throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN_RANGE, Str_Function_Name, "%s (%s)", rangeName.c_str(), strerror (errno));

  if (fileRange.fread (& numMBRead, 1) != 1)
    throw CBazaNtExcept (CBazaNtExcept::ERR_READ, Str_Function_Name, "ошибка чтения файла выделенных диапозонов номеров агентов");

  if (numMBRead != numMB)
    throw CBazaNtExcept (CBazaNtExcept::ERR_MB_NUMBER_RANGE, Str_Function_Name);

  if (fileRange.fread (RangeAllocate, 255) != 255)
    throw CBazaNtExcept (CBazaNtExcept::ERR_READ, Str_Function_Name, "ошибка чтения файла выделенных диапозонов номеров агентов");

  fileRange.fclose ();
}

unsigned char CBazaNt::GetFreeRange (bool isAgent)
{
  CTakeSem Sem (& Semaphore);
  unsigned k;

  if (MaxAgentNum.NumC.NumMb == 0) {
    k = (isAgent == true) ? 1 : 2;

    for (unsigned short i = k; i <= 0xFF; i ++) {
      if ((BazaSpis.find (i) == BazaSpis.end ()) && (RangeAllocate[i-1] > 0))
        return (unsigned char)i;
    }
  }
  else {
    if (isAgent == true) {
      if ((BazaSpis.find (MaxAgentNum.NumC.NumMb) == BazaSpis.end ()) && (RangeAllocate[MaxAgentNum.NumC.NumMb-1] > 0))
        return MaxAgentNum.NumC.NumMb;

      for (unsigned short i = 2; i <= 0xFF; i ++) {
        if ((BazaSpis.find (i) == BazaSpis.end ()) && (RangeAllocate[i-1] > 0))
          return (unsigned char)i;
      }
    }
    else {
      for (unsigned short i = 2; i <= 0xFF; i ++) {
        if ((BazaSpis.find (i) == BazaSpis.end ()) && (RangeAllocate[i-1] > 0) && (i != MaxAgentNum.NumC.NumMb))
          return (unsigned char)i;
      }
    }
  }

  return 0;
}

unsigned short CBazaNt::GetFreeNumInRange (unsigned char range)
{
  Numbers num;

  if (range == 0)
    return 0;

  num.NumC.NumMb = range;

  for (unsigned short i = 0; i <= 0xFF; i ++) {
    num.NumC.NumAgent = (unsigned char)i;
    if (BazaSpis.find (num.AgentNum) == BazaSpis.end ())
      return num.AgentNum;
  }

  return 0;
}

unsigned short CBazaNt::GetFreeNumOnMB ()
{
  TBazaSpis::iterator itSpBaza;
  unsigned short agNum;

  for (unsigned short i = 1; i <= 0xFF; i ++) {
    if ((itSpBaza = BazaSpis.find (i)) == BazaSpis.end ())
      continue;
    if (itSpBaza->second.OsType != TypeOs::OS_RANGEMB)
        continue;
    if (itSpBaza->second.RangeMBNumber != MaxAgentNum.NumC.NumMb)
      continue;

    if ((agNum = GetFreeNumInRange ((unsigned char)i)) != 0)
      return agNum;
  }

  return 0;
}

EXCEPTION_STR(
unsigned short CBazaNt::AddRange (unsigned char numMB)
)
  CBazaNt::RecBase recBaza;
  unsigned short range;

  CTakeSem Sem (& Semaphore);

  bool isRangeAgent = (numMB == MaxAgentNum.NumC.NumMb) ? true : false;

  if ((range = GetFreeRange (isRangeAgent)) == 0)
    return 0;

  recBaza.OsType = CBazaNt::TypeOs::OS_RANGEMB;
  recBaza.RangeMBNumber = numMB;

  RecBase recadd = recBaza;
  recadd.AgentNum.AgentNum = range;

  BazaSpis.insert (TBazaSpis::value_type (recadd.AgentNum.AgentNum, recadd));
  try {
    FlushBaza ();
  }
  catch (...) {
    TBazaSpis::iterator itSpBaza = BazaSpis.find (recadd.AgentNum.AgentNum);
    if (itSpBaza != BazaSpis.end ())
      BazaSpis.erase (itSpBaza);
    throw;
  }

  return recadd.AgentNum.AgentNum;
}

EXCEPTION_STR(
void CBazaNt::WriteRangeFile (unsigned char numMB, const char * fileName)
)
  unsigned char byte11;
  TBazaSpis::iterator itSpBaza;

  CTakeSem Sem (& Semaphore);
  CStdFILE File;

  if (File.fopen (fileName, "wb") == false)
    throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN, Str_Function_Name, "%s (%s)", fileName, strerror (errno));
  if (File.fwrite(& numMB, 1) != 1)
    throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, fileName);

  for (unsigned short i = 1; i <= 0xFF; i ++) {
    byte11 = 0;

    if ((itSpBaza = BazaSpis.find (i)) == BazaSpis.end ()) {
      if (File.fwrite(& byte11, 1) != 1)
        throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, fileName);
      continue;
    }

    if ((itSpBaza->second.OsType != TypeOs::OS_RANGEMB) && (itSpBaza->second.OsType != TypeOs::OS_SM)) {
      if (File.fwrite(& byte11, 1) != 1)
        throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, fileName);
      continue;
    }

    if ((itSpBaza->second.OsType == TypeOs::OS_RANGEMB) && (itSpBaza->second.RangeMBNumber != numMB)) {
      if (File.fwrite(& byte11, 1) != 1)
        throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, fileName);
      continue;
    }

    if ((itSpBaza->second.OsType == TypeOs::OS_SM) && (itSpBaza->first) != numMB) {
      if (File.fwrite(& byte11, 1) != 1)
        throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, fileName);
      continue;
    }

    byte11 = 1;
    if (File.fwrite(& byte11, 1) != 1)
      throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, fileName);
  }
}

unsigned char CBazaNt::GetMbRangeAlloc ()
{
  unsigned char res = 0;

  CTakeSem Sem (& Semaphore);
  
  for (unsigned short i = 1; i <= 0xFF; i ++) {
    if (RangeAllocate[i-1] > 0)
      res ++;
  }

  return res;
}

unsigned char CBazaNt::GetMbRangeAdd (unsigned char mbNum)
{
  TBazaSpis::iterator itSpBaza;
  unsigned char res = 0;

  CTakeSem Sem (& Semaphore);

  for (unsigned short i = 1; i <= 0xFF; i ++) {
    if ((itSpBaza = BazaSpis.find (i)) == BazaSpis.end ())
      continue;
    if ((itSpBaza->second.OsType != TypeOs::OS_RANGEMB) && (itSpBaza->second.OsType != TypeOs::OS_SM))
      continue;
    if ((itSpBaza->second.OsType == TypeOs::OS_RANGEMB) && (itSpBaza->second.RangeMBNumber != mbNum))
      continue;
    if ((itSpBaza->second.OsType == TypeOs::OS_SM) && (itSpBaza->first != mbNum))
      continue;

    res ++;
  }

  return res;
}

bool CBazaNt::IsAgentExistInRange (unsigned char range)
{
  Numbers num;

  CTakeSem Sem (& Semaphore);

  if (range == 0)
    return false;

  num.NumC.NumMb = range;

  for (unsigned short i = 0; i <= 0xFF; i ++) {
    num.NumC.NumAgent = (unsigned char)i;
    if (BazaSpis.find (num.AgentNum) != BazaSpis.end ())
      return true;
  }

  return false;
}

unsigned char CBazaNt::GetMbRangeAddAll ()
{
  TBazaSpis::iterator itSpBaza;
  unsigned char res = 0;

  CTakeSem Sem (& Semaphore);

  for (unsigned short i = 1; i <= 0xFF; i ++) {
    if ((itSpBaza = BazaSpis.find (i)) == BazaSpis.end ())
      continue;
    if ((itSpBaza->second.OsType != TypeOs::OS_RANGEMB) && (itSpBaza->second.OsType != TypeOs::OS_SM))
      continue;

    res ++;
  }

  return res;
}

unsigned short CBazaNt::GetFirstRangePMB (unsigned char mbNum)
{
  TBazaSpis::iterator itSpBaza;

  CTakeSem Sem (& Semaphore);

  for (unsigned short i = 1; i <= 0xFF; i ++) {
    if ((itSpBaza = BazaSpis.find (i)) == BazaSpis.end ())
      continue;
    if ((itSpBaza->second.OsType == TypeOs::OS_RANGEMB) && (itSpBaza->second.RangeMBNumber == mbNum))
      return i;
  }

  return 0;
}

bool CBazaNt::IsRangeAgentExist (unsigned char range)
{
  TBazaSpis::iterator itSpBaza;

  CTakeSem Sem (& Semaphore);

  if ((itSpBaza = BazaSpis.find (range)) == BazaSpis.end ())
    return false;
  else {
    if (itSpBaza->second.OsType == TypeOs::OS_RANGEMB)
      return true;
    else
      return false;
  }
}

EXCEPTION_STR(
void CBazaNt::CheckRange ()
)
  TBazaSpis::iterator itSpBaza;

  for (unsigned short i = 1; i <= 0xFF; i ++) {
    if ((itSpBaza = BazaSpis.find (i)) == BazaSpis.end ())
      continue;
    if (((itSpBaza->second.OsType == TypeOs::OS_RANGEMB) || (itSpBaza->second.OsType == TypeOs::OS_SM)) && (RangeAllocate[i-1] == 0))
      throw CBazaNtExcept (CBazaNtExcept::CRASH_RANGE_FILE, Str_Function_Name);
  }
}

void CBazaNt::GetAllRange (unsigned char range[255])
{
  if (range != NULL)
    memcpy (range, RangeAllocate, 255);
}

EXCEPTION_STR(
void CBazaNt::ReadMBGUID (const char * bName)
)
  std::string nameMBID = bName;
  nameMBID += "MBID";
  char tempMas[50]= {0};
  CStdFILE File;

  if ((File.fopen (nameMBID.c_str(), "rb") == true) && (File.getfilesize() == 36)) {
    if (File.fread(tempMas, 36) != 36)
      throw CBazaNtExcept (CBazaNtExcept::ERR_READ, Str_Function_Name, nameMBID.c_str());
    MB_GUID = tempMas;
    File.fclose();
    return;
  }

  File.fclose ();

  if (GetMbNum () == 0) {
    MB_GUID = GUIDNameSpace::GetUniqueGUID();
    WriteMBGUID(bName, MB_GUID);
  }
  else
    throw CBazaNtExcept (CBazaNtExcept::GUID_NOT_EXIST, Str_Function_Name, ",на подчиненной МБ № %02X", GetMbNum ());
}

EXCEPTION_STR(
void CBazaNt::WriteMBGUID (const char * bName, std::string & MB_GUID)
)
  std::string nameMBID = bName;
  nameMBID += "MBID";
  CStdFILE File;

  if (File.fopen (nameMBID.c_str(), "wb") == false)
    throw CBazaNtExcept (CBazaNtExcept::ERR_OPEN, Str_Function_Name, "%s (%s)", nameMBID.c_str(), strerror (errno));
  if (File.fwrite(MB_GUID.c_str(), MB_GUID.size()) != MB_GUID.size())
    throw CBazaNtExcept (CBazaNtExcept::ERR_WRITE, Str_Function_Name, nameMBID.c_str());
  File.fclose();
}

EXCEPTION_STR(
unsigned char CBazaNt::CheckRangeMas (unsigned short numMB, unsigned char rangeMas[255])
)
  TBazaSpis::iterator itSpBaza;

  CTakeSem Sem (& Semaphore);

  for (unsigned short i = 1; i <= 0xFF; i ++) {
    if (rangeMas[i-1] > 0) {
      if ((itSpBaza = BazaSpis.find (i)) == BazaSpis.end ())
        return (unsigned char)i;

      if ((itSpBaza->second.OsType == TypeOs::OS_SM) && (itSpBaza->first) == numMB)
        continue;
      if ((itSpBaza->second.OsType == TypeOs::OS_RANGEMB) && (itSpBaza->second.RangeMBNumber == numMB))
        continue;

      return (unsigned char)i;
    }
  }

  return 0;
}

void CBazaNt::GetMBGUID (unsigned char masGuid[16]) 
{
  GUIDNameSpace::ConvertGUIDFromString(GetMBGUID(), masGuid);
}

void CBazaNt::ReplaceUnknownType (unsigned char unkonownAgentType, unsigned char unknownModelType, std::set <unsigned char> & listAgentType, std::set <unsigned char> & listModelType)
{
  TBazaSpis::iterator itSpBaza;
  RecBase rec;
  bool isChangeBaza = false;

  CTakeSem Sem (& Semaphore);

  for (itSpBaza = BazaSpis.begin (); itSpBaza != BazaSpis.end (); itSpBaza ++) {
    if (itSpBaza->second.OsType == CBazaNt::TypeOs::OS_RANGEMB)
      continue;

    if (listAgentType.find (itSpBaza->second.OsType) == listAgentType.end()) {
      itSpBaza->second.OsType = unkonownAgentType;
      itSpBaza->second.State = CBazaNt::States::ASTATE_REINSTALL_NEED;
      isChangeBaza = true;
    }

    if (listModelType.find (itSpBaza->second.Model) == listModelType.end()) {
      itSpBaza->second.Model = unknownModelType;
      itSpBaza->second.State = CBazaNt::States::ASTATE_REINSTALL_NEED;
      isChangeBaza = true;
    }
  }

  if (isChangeBaza == true) {
// бэкап файла базы
    char TempBazaName[512];
    sprintf (TempBazaName, "%s_%08X.backup", BazaName, time (NULL));
    CopyFile (BazaName, TempBazaName, FALSE);

    FlushBaza ();
  }
}

void CBazaNt::SaveBackupBaza (const std::string & bazaDir)
{
  CTakeSem Sem (& Semaphore);
  std::list <FileDirTools::CFileData> listFile;
  std::string tempBazaName;
  std::string tempTime;

  listFile = FileDirTools::FindAllWaysMaskByName(bazaDir, "agentbaza_backup*_*_*-*");

  if (listFile.size() > 9) {
    listFile.sort ();
    DeleteFile((listFile.begin()->GetFullPath()).c_str());
  }

  tempBazaName = bazaDir;
  FileDirTools::AddEndSlash(tempBazaName);
  tempBazaName += "agentbaza_backup";
  StrFtime(tempTime, time (NULL), "%Y_%m_%d %H-%M");
  tempBazaName += tempTime;

  CopyFile (BazaName, tempBazaName.c_str(), FALSE);
}

#endif

