#ifndef _BAZANT_H_2002_04_24
#define _BAZANT_H_2002_04_24


#include <time.h>
#include "semaphoret.h"
#endif

#include <map>
#include <vector>

#include "icexception.h"
#include "bytecount1.h"

#ifndef MAX_PATH
#define MAX_PATH 512
#endif

#include <vector>
#include <set>

class CBazaNt
{
public:
  class CBazaNtExcept : public CicException {
  public:
    CBazaNtExcept (int err, const CicException::StringType & type, const char * fs = NULL, ...) throw ();
    enum CBazaNtExceptConst {
      ERR_OPEN = -1, // Ошибка открытия файла базы
      ERR_SET_SECURITY = -2, // Ошибка установки атрибутов
      NAME_EXIST = -3, //IP адрес или имя уже существует в базе
      ERR_WRITE = -4,// Ошибка записи в файл базы
      ERR_READ = -5, // Ошибка чтения из файла базы
      ERR_REMOVE = -6, // Ошибка удаления файла базы
      ERR_RENAME = -7, // Ошибка переименования файла базы
      NOT_NUMBER = -8, // Кончились номера для агентов
      NUMBER_NOT_FOUND = -9, // Номер агента не найден в базе
      ERR_MB_NUMBER = -10,
      BAD_ADD_NUMBER = -11,
      IP_EXIST = -12,
      NUMBER_EXIST = -13,
      NOT_MEMORY_DUMP_STATE = -14,
      DUMP_STATE_INVALID = -15,
      ERR_MB_NUMBER_RANGE = -16,
      ERR_OPEN_RANGE = -17,
      ERR_EXIST_AGENTPLATFORM_PORT = -18,
      NOT_RANGE = -19,
      CRASH_RANGE_FILE = -20,
      GUID_NOT_EXIST = -21
    };
    
    virtual const char * GetStrErr() const throw ();
  };

  union Numbers {
    unsigned short AgentNum;
    struct {
      unsigned char NumAgent;
      unsigned char NumMb;
    }NumC;
  };

  struct DataBaseMem {
    DataBaseMem ();
    void Read (const unsigned char * Mas);
    unsigned long Write (unsigned char * Mas) const;

    unsigned char AgState;// Состояние агента на связи или нет
    unsigned long IPAg; // IP агента заполняется при выходе агента на связь
    unsigned char LastEvent; // Последнее событие посланное experty в менеджере состояния
    time_t TimeShutDown; // Время с которого начиаем отсчет для перезагрузки
    time_t TimeUpDown; // Время с которого начиаем отсчет для загрузки
    unsigned short AgentPort; // Порт на котором агент ждет команды
  };
  struct RecBase {
    RecBase ();
    void Read (const unsigned char * Mas);
    unsigned long Write (unsigned char * Mas) const;

    Numbers AgentNum; // Номер агента
    unsigned long AgentIp; // Ip Агента
    unsigned short AgentPort;// Порт агента
    unsigned long MbIp;// IP Машины безопасности
    unsigned short MbPort;// Порт машины безопасности
    unsigned char State;// Состояние агента
    unsigned char OsType;// Тип ОС агента
    unsigned long ConnectCount;// Кол-во сеансов агента с МБ
    unsigned char Model;// Модель агента
    unsigned char IsNotPing;// Признак удаленный агент или нет (в лок. или рег. сети)
    unsigned char IsLock;// Признак блокировки агента
    unsigned char StateBackup;
    unsigned char Reserv2[5]; // ***************************************************************************свободная область
    time_t LastSeans;
    unsigned char GUIDMB[16]; 
    unsigned char Reserv1[340]; // *************************************************************************свободная область
    unsigned char HashFiles[16];// Свертка на файлы агента
    unsigned char SerNum[16];// Серийный номер агента (при каждой подготовке должен меняться)
    char Name[100];// Имя агента
    char EventListName[256];// Имя списка событий
    unsigned char AllHashFiles[16];
    unsigned char GUIDAgent[16]; 
    unsigned short AgentPlatformPort;
    char Otvetstvenuy[100];
    char SerNumBackup[16];
    unsigned short SessionID;
    unsigned long SessionIP;
    char SessionUser[100];
    char OSUser[100];
    char ComputerName[100];
    char MBName[100];
    unsigned char Reserv4[22]; // ***************************************************************************свободная область
    unsigned char RangeMBNumber;
/*************** Reserv 1421 ***********************/
    unsigned char Reserv[1421];
// ************* В файл не записывается хранится только в памяти **********************
    DataBaseMem datamem;
  };

  struct RecBaseAdv {
    RecBaseAdv ();
    void Read (const unsigned char * Mas);
    unsigned long Write (unsigned char * Mas) const;

    char ModelDesc[500];
    char ModelName[500];
    char AgentDesc[256];
    char AgentDir[100];
/*************** Reserv 3512 ***********************/
    unsigned char Reserv[3512];
  };

  struct TypeOs {
    enum Const {
      OS_RANGEMB = 1,
      OS_SM        = 2,
      OS_HPUX      = 4,
      OS_WINNT     = 5,
      OS_NOVELL    = 8,
      OS_EMBEDED   = 9,
      OS_DOS       = 10,
      OS_ORACLE    = 11,
      OS_WORACLE   = 12,
      OS_WORACLE2   = 44,
      OS_SYSLOG    = 13,
      OS_SECRETNET = 14,
      OS_SECRETNET5 = 25,
      OS_SECRETNET5L = 30,
      OS_SPECTRZ   = 15,
      OS_WIN_LITE = 16,
      OS_SOL_LITE = 17,
      OS_INFORMIX_LITE = 18,
      OS_AIX_LITE = 19,
      OS_ACCORD = 26,
      OS_EVENTLOG = 35,
      OS_HPUX1131 = 54,
      OS_HPUX1131_RISC = 57,
      OS_UNKNOWN = 255
    };
    TypeOs (const unsigned char os, const char * const name, const char * const dir) : Os (os), Name (name), Dir (dir) {};
    const unsigned char Os;
    const char * const Name;
    const char * const Dir;
  };
  struct States {
    enum Const {
      ASTATE_NORMAL_MODE        = 0,  //Режим нормальной работы агента
      ASTATE_TRANSFERED         = 2,  //Агент передан на станцию при обновлении
      ASTATE_PREPARE            = 3,  //Агент подготовлен к установке или переустановке
      ASTATE_UPDATE_NEED        = 4,  //Необходимо обновление агента
      ASTATE_REINSTALL_NEED     = 8,  //Требуется переустановка агента (аудит не собирается)
      ASTATE_MUST_BE_DELETED    = 32, //Агент должен быть удален со станции
      ASTATE_EXPORT             = 33, //Агент экспортирован
      ASTATE_UPDATE_PERMIT      = 34, //Разрешено обновление агента 
      ASTATE_DELETE_AGENT       = 35, //Агент удален со станции
      ASTATE_INSTALL_AGENT      = 39, //Агент инсталирован
      ASTATE_CREATE_AGENT       = 53, //Агент создается (в базе агентов его пока нет, он находится в стадии создания)
      DELETE_AGENT_UZ           = 36, //Удаление учетной записи, не состояние УЗ
      BACKUP_AGENT_UZ           = 37, //Откат состояния УЗ агента, не состояние
      IMPORT_AGENT_UZ           = 44, //Импорт учтной записи агента, не состояние
      CHANGE_SEANS              = 41, //Смена сеанса, не состояние
      CHANGE_SEANS_DEL_CONTEXT  = 42, //Смена сеанса с удалением контекста
      SEND_AGENT_COMMAND        = 45, //Передача агенту дополнительной команды, не состояние
      DEINSTALL_AGENT           = 46, //Удаленая деинсталяция агента, не состояние
      SET_STATE_INSTALL_AGENT   = 47, //Установка состояния агент инсталирован
      PREPARE_NEW_AGENT         = 48, //Создание учетной записи агента
      RESET_WRN_ERR             = 49, //Сбросить предупреждения и ошибки
      ADD_RANGE_NUM             = 50, //Добавить диапозон номеров
      CHECK_AGENT_PLATFORM      = 51, //Проверка агентской платформы
      GET_LOGS                  = 52, //Получить журналы работы
    };
    States (const unsigned char st, const char * const name, const char * const panelComand) : state (st), Name (name), PanelComand (panelComand) {};
    const unsigned char state;
    const char * const Name;
    const char * const PanelComand;
  };

  static const char * GetStrAgentState (unsigned char state);

  static const States ASTATE_NORMAL_MODE;
  static const States ASTATE_TRANSFERED;
  static const States ASTATE_PREPARE;
  static const States ASTATE_UPDATE_NEED;
  static const States ASTATE_REINSTALL_NEED;
  static const States ASTATE_MUST_BE_DELETED;
  static const States ASTATE_EXPORT;
  static const States ASTATE_UPDATE_PERMIT;
  static const States ASTATE_DELETE_AGENT;
  static const States ASTATE_INSTALL_AGENT;
  static const States ASTATE_CREATE_AGENT;
  static const States DELETE_AGENT_UZ;
  static const States BACKUP_AGENT_UZ;
  static const States IMPORT_AGENT_UZ;
  static const States CHANGE_SEANS;
  static const States CHANGE_SEANS_DEL_CONTEXT;
  static const States SEND_AGENT_COMMAND;
  static const States DEINSTALL_AGENT;
  static const States SET_STATE_INSTALL_AGENT;
  static const States PREPARE_NEW_AGENT;
  static const States RESET_WRN_ERR;
  static const States ADD_RANGE_NUM;
  static const States CHECK_AGENT_PLATFORM;
  static const States GET_LOGS;

  static const TypeOs OS_WIN98;
  static const TypeOs OS_WINNT;
  static const TypeOs OS_SM;
  static const TypeOs OS_NOVELL;
  static const TypeOs OS_EMBEDED;
  static const TypeOs OS_DOS;
  static const TypeOs OS_HPUX;
  static const TypeOs OS_ORACLE;
  static const TypeOs OS_WORACLE;
  static const TypeOs OS_SYSLOG;
  static const TypeOs OS_SECRETNET;
  static const TypeOs OS_SPECTRZ;
  static const TypeOs OS_WIN_LITE;
  static const TypeOs OS_SOL_LITE;
  static const TypeOs OS_INFORMIX_LITE;
  static const TypeOs OS_AIX_LITE;
  static const TypeOs OS_HPUX1131;
  static const TypeOs OS_HPUX1131_RISC;
  static const TypeOs OS_UNKNOWN;

#ifdef NTSM_DEF
public:
// **************************** Конструкторы **********************************
  CBazaNt();

// Инициализация базы
// throw (CSemaphoreT::CSemaphoreTExcept)
// throw (CStdFILE::CStdFileExcept)
// throw (CBazaNt::CBazaNtExcept) CBazaNtExcept::ERR_SET_SECURITY
//                                CBazaNtExcept::ERR_OPEN
//                                CBazaNtExcept::ERR_WRITE
//                                CBazaNtExcept::ERR_READ
//                                CBazaNtExcept::NAME_EXIST
//                                CBazaNtExcept::ERR_MB_NUMBER
// bName - [IN] имя файла базы
// semName - [IN] имя семафора
  void Init (const char * bName, const char * semName, unsigned char numMb);
  void ReplaceUnknownType (unsigned char unkonownAgentType, unsigned char unknownModelType, std::set <unsigned char> & listAgentType, std::set <unsigned char> & listModelType);
// Чтение из файла всех записей (необходимо только для Audit_Parser)
  void ReadInFile ();
// ***************************** Операции **************************************
// Добавление записи
// throw (CSemaphoreT::CSemaphoreTExcept)
// throw (CStdFILE::CStdFileExcept)
// throw (CBazaNt::CBazaNtExcept) CBazaNtExcept::ERR_OPEN
//                                CBazaNtExcept::ERR_WRITE
//                                CBazaNtExcept::ERR_REMOVE
//                                CBazaNtExcept::ERR_RENAME
//                                CBazaNtExcept::NAME_EXIST
// rec - [IN] запись агента (поле AgentNum устанавливается в методе Add)
// num - [IN] номер агента который нужно получать с помощью GetNextNumber ()
  unsigned short Add (const RecBase * rec, bool isImport);

// Чтение записи
// throw (CSemaphoreT::CSemaphoreTExcept)
// throw (CBazaNt::CBazaNtExcept) CBazaNtExcept::NUMBER_NOT_FOUND
// agNum - [IN] номер агента запись которого хотим прочитать
// rec - [OUT] буфер для записи
  void Read (unsigned short agNum, RecBase * rec);

// Запись записи
// Номер агента запись которого будет записываться берется из rec->AgentNum
// throw (CSemaphoreT::CSemaphoreTExcept)
// throw (CStdFILE::CStdFileExcept)
// throw (CBazaNt::CBazaNtExcept) CBazaNtExcept::ERR_OPEN
//                                CBazaNtExcept::ERR_WRITE
//                                CBazaNtExcept::ERR_REMOVE
//                                CBazaNtExcept::ERR_RENAME
//                                CBazaNtExcept::NAME_EXIST
//                                CBazaNtExcept::NUMBER_NOT_FOUND
// rec - [IN] запись для записи
// recold - [IN] если = NULL то записывается rec, иначе записываются только те поля 
//               из rec которые изменились по сравнению с recold
  void Write (const RecBase * rec, const RecBase * recold = NULL);

  void WriteMem (unsigned short agNum, const DataBaseMem * rec, const DataBaseMem * recold = NULL);

// Удаление записи
// throw (CSemaphoreT::CSemaphoreTExcept)
// throw (CStdFILE::CStdFileExcept)
// throw (CBazaNt::CBazaNtExcept) CBazaNtExcept::ERR_OPEN
//                                CBazaNtExcept::ERR_WRITE
//                                CBazaNtExcept::ERR_REMOVE
//                                CBazaNtExcept::ERR_RENAME
//                                CBazaNtExcept::NUMBER_NOT_FOUND
// agNum - [IN] номер агента запись которого хотим удалить
  void Delete (unsigned short agNum);

// Чтение базы по списку
// RETURN : если 0 значит агентов начиная с номера agNum и до конца нет, 
//          иначе номер агента чью запись прочитали
// throw (CSemaphoreT::CSemaphoreTExcept)
// agNum - [IN/OUT] входное значение номер агента с которого начнем читать,
//                  выходное значение номер агента  запись которого прочитали + 1
// rec - [OUT] буфер для записи
  unsigned short ReadSpis (unsigned short * agNum, RecBase * rec);

// ********************************** Атрибуты **********************************
  unsigned char GetMbNum () const throw ();

  std::string GetGroup (unsigned short agNum);
  std::vector<unsigned short> GetNumFromGroup (const std::string & group);

  bool IsAgentMB (unsigned short agNum);

  unsigned long WriteMemInBuf (unsigned char * buf, unsigned long lenbuf);

  unsigned short AddRange (unsigned char numMB);
  void WriteRangeFile (unsigned char numMB, const char * fileName);
  // кол-во диапозонов доступных для добавления
  unsigned char GetMbRangeAlloc ();
  // кол-во дипозонов добавленых
  unsigned char GetMbRangeAdd (unsigned char mbNum);

  bool IsAgentExistInRange (unsigned char range);
  unsigned char GetMbRangeAddAll ();
  unsigned short GetFirstRangePMB (unsigned char mbNum);
  bool IsRangeAgentExist (unsigned char range);
  void GetAllRange (unsigned char range[255]);

  std::string GetMBGUID () {return MB_GUID;}
  void GetMBGUID (unsigned char masGuid[16]);
  void WriteMBGUID (const char * bName, std::string & MB_GUID);
  unsigned char CheckRangeMas (unsigned short numMB, unsigned char rangeMas[255]);
  void SaveBackupBaza (const std::string & bazaDir);

private:
  void ReadMBGUID (const char * bName);
  unsigned short FindName (const char * name);
  unsigned short FindIP (unsigned short num, unsigned long IP, unsigned short port);
  void FlushBaza ();
  static std::string GetGroupFromName (const RecBase & rec);
  unsigned char GetFreeRange (bool isAgent);
  unsigned short GetFreeNumInRange (unsigned char range);
  unsigned short GetFreeNumOnMB ();
  void GetReadRanges (const char * bName, unsigned char numMB);
  bool IsFreeRangeMbNum ();
  void CheckRange ();

private:
  char BazaName[MAX_PATH];
  CSemaphoreT Semaphore;
  typedef std::map<unsigned short, RecBase> TBazaSpis;
  TBazaSpis BazaSpis;
  Numbers MaxAgentNum;
  unsigned char RangeAllocate[255];
  std::string MB_GUID;
#endif

public:
  static void WriteRecFile (RecBase * rec, const char * fileName);
  static void ReadRecFile (RecBase * rec, const char * fileName);
  static void ReadRecAdvFile (RecBaseAdv * rec, const char * fileName);
  static void WriteRecAdvFile (RecBaseAdv * rec, const char * fileName);
  static void ReadMemInBuf (const unsigned char * buf, unsigned long lenbuf, std::map<unsigned short, CBazaNt::DataBaseMem> & mapState);
  static std::vector <CBazaNt::States::Const> GetAvaibleAgentStates (unsigned char curState, bool isMB);
  static std::vector <CBazaNt::States::Const> GetAllAgentStates ();
  static bool IsNextAgentState (unsigned char curState, CBazaNt::States::Const nextState, bool isMB);
  static CBazaNt::States GetStateDesc (CBazaNt::States::Const state);
// return 0 - можно пускать, 1 состояние не позволяет принимать соединение, 2 не совпадают серйные номера
  static unsigned char IsWorkAgentState (unsigned char State, unsigned char SerNumBaza[16], unsigned char SerNumAgent[16]);
  static bool IsNotWorkAgentState (unsigned char State);

};

#endif
