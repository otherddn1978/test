<<<<<<< HEAD
=======
#include <set>
#include "inifile.h"
#include "kdlstring.h"
>>>>>>> 6e679ab5115ce1484f4ee714ff930ed72ef106a5

std::string TIniFile::ReadString(const char *section, const char *key, const char *def) const
{
	int		size(MAX_READ_BUF);
	if (def != NULL && size < (int)strlen(def))
	{
		size = strlen(def) + 1;
	}
	TAutoBuffer	buf(1);

	int	r(0);
	do
	{
		size *= 2;
		buf.resize(size);
		r = GetPrivateProfileString(section, key, def, buf.buffer, size, file.c_str());
	}
	while (r == (size - 1));
	std::string res(buf.buffer);
	return res;
}

std::string TIniFile::ReadString(const std::string &section, const std::string &key, const std::string &def) const
{
	return ReadString(section.c_str(), key.c_str(), def.c_str());
}

void TIniFile::WriteString(const char *section, const char *key, const char *value) const
{
	if(WritePrivateProfileString(section, key, value, file.c_str()) == 0)
		throw TException(this, "Ошибка записи строки");
}

void TIniFile::WriteString(const std::string &section, const std::string &key, const std::string &value) const
{
	WriteString(section.c_str(), key.c_str(), value.c_str());
}

std::string TIniFile::ReadPassword(const std::string &section, const std::string &key, const std::string &def) const
{
	std::stringstream		res;
	int						c, c1;
	std::string				s;
	std::string::size_type	a;

	s = ReadString(section, key, def);
	if(s == def)
		return s;

	// Восстановление
	for(c1 = 0x95, a = 0; a < s.length(); a += 2)
	{	std::stringstream	ss(s.substr(a, 2), std::ios_base::in);
		ss>>std::hex>>c;
		c ^= c1;
		res<<static_cast<char>(c);
		c1 = c;
	}

	return res.str();
}

std::string TIniFile::ReadPassword(const std::string &section,
	const std::string &openPwdKey, const std::string &hiddenPwdKey, const std::string &def) const
{
	if(KeyExists(section, openPwdKey))
		return ReadString(section, openPwdKey, def);
	else
		return ReadPassword(section, hiddenPwdKey, def);
}

void TIniFile::WritePassword(const std::string &section, const std::string &key, std::string value) const
{
	std::stringstream		ss(std::ios_base::out);
	std::string::size_type	a;
	int						c1, c2;
	
	// Скрытие от просмотра
	for(c1 = 0x95, a = 0; a < value.length(); ++a)
	{	c2 = value[a];
		value[a] ^= c1;
		c1 = c2;
	}

	// Сохранение в виде набора шестнадцатиричных чисел
	for(a = 0; a < value.length(); ++a) 
		ss<<std::hex<<std::setw(2)<<std::setfill('0')<<std::right<<std::noshowbase<<std::uppercase
			<<static_cast<unsigned int>(static_cast<unsigned char>(value[a]));
	WriteString(section, key, ss.str());
}

int TIniFile::ReadInteger(const char *section, const char *key, int def) const throw()
{
	TAutoBuffer	buf(MAX_NUMBER_BUF);

	GetPrivateProfileString(section, key, "", buf.buffer, MAX_NUMBER_BUF, file.c_str());
	int res(def);
	if(*buf.buffer != '\0')
	{
		res = atoi(buf.buffer);
	}
	return res;
}

int TIniFile::ReadHexInteger(const char *section, const char *key, int def) const throw()
{
  TAutoBuffer	buf(MAX_NUMBER_BUF);

  GetPrivateProfileString(section, key, "", buf.buffer, MAX_NUMBER_BUF, file.c_str());
  int res(def);
  if(*buf.buffer != '\0')
  {
    res = std::strtol(buf.buffer, 0, 16);
  }
  return res;
}

int TIniFile::ReadInteger(const std::string &section, const std::string &key, int def) const
{
	return ReadInteger(section.c_str(), key.c_str(), def);
}

void TIniFile::WriteInteger(const char *section, const char *key, int value) const
{
	TAutoBuffer	buf(MAX_NUMBER_BUF);

#if _MSC_VER >= 1800
	sprintf_s(buf.buffer, MAX_NUMBER_BUF, "%d", value);
#else
	sprintf(buf.buffer, "%d", value);
#endif
	if(WritePrivateProfileString(section, key, buf.buffer, file.c_str()) == 0)
	{
		throw TException(this, "Ошибка записи числа");
	}
}
void TIniFile::WriteInteger(const std::string &section, const std::string &key, int value) const
{
	WriteInteger(section.c_str(), key.c_str(), value);
}

unsigned long TIniFile::ReadULong(const char *section, const char *key, unsigned long def) const throw()
{
	TAutoBuffer	buf(MAX_NUMBER_BUF);
	char	*p;

	GetPrivateProfileString(section, key, "", buf.buffer, MAX_NUMBER_BUF, file.c_str());
	unsigned long res(def);
	if(*buf.buffer != '\0')
	{
		res = strtoul(buf.buffer, &p, 10);
	}
	return res;
}

unsigned long TIniFile::ReadULong(const std::string &section, const std::string &key, unsigned long def) const
{
	return ReadULong(section.c_str(), key.c_str(), def);
}

void TIniFile::WriteULong(const char *section, const char *key, unsigned long value) const
{
	TAutoBuffer	buf(MAX_NUMBER_BUF);
#if _MSC_VER >= 1800
	_ultoa_s(value, buf.buffer, MAX_NUMBER_BUF, 10);
#else
	ultoa(value, buf.buffer, 10);
#endif
	if(WritePrivateProfileString(section, key, buf.buffer, file.c_str()) == 0)
	{
		throw TException(this, "Ошибка записи числа");
	}
}

void TIniFile::WriteULong(const std::string &section, const std::string &key, unsigned long value) const
{
	WriteULong(section.c_str(), key.c_str(), value);
}

bool TIniFile::ReadBool(const char *section, const char *key, bool def) const throw()
{
	int	i;
		
	i = ReadInteger(section, key, -1);
	if(i == -1)
		return def;
	else
		return (i != 0);
}

bool TIniFile::ReadBool(const std::string &section, const std::string &key, bool def) const
{
	return ReadBool(section.c_str(), key.c_str(), def);
}

void TIniFile::WriteBool(const char *section, const char *key, bool value) const
{
	WriteInteger(section, key, value? 1: 0);
}

void TIniFile::WriteBool(const std::string &section, const std::string &key, bool value) const
{
	return WriteBool(section.c_str(), key.c_str(), value);
}

double TIniFile::ReadDouble(const char *section, const char *key, double def) const throw()
{
	TAutoBuffer	buf(MAX_NUMBER_BUF);

	GetPrivateProfileString(section, key, "", buf.buffer, MAX_NUMBER_BUF, file.c_str());
	double res(def);
	if(*buf.buffer != '\0')
	{
		res = atof(buf.buffer);
	}
	return res;
}

double TIniFile::ReadDouble(const std::string& section, const std::string& key, double def) const
{
	return ReadDouble(section.c_str(), key.c_str(), def);
}

void TIniFile::WriteDouble(const char *section, const char *key, double value) const
{
	TAutoBuffer	buf(MAX_NUMBER_BUF);
#if _MSC_VER >= 1800
	sprintf_s(buf.buffer, MAX_NUMBER_BUF, "%f", value);
#else
	sprintf(buf.buffer, "%f", value);
#endif
	if(WritePrivateProfileString(section, key, buf.buffer, file.c_str()) == 0)
	{
		throw TException(this, "Ошибка записи значения с плавающей точкой");
	}
}

void TIniFile::WriteDouble(const std::string& section, const std::string& key, double value) const
{
	WriteDouble(section.c_str(), key.c_str(), value);
}

// Чтение time_t в виде строки
time_t TIniFile::ReadTimeStr(const std::string &section, const std::string &key, time_t def) const
{
	std::string	s = ReadString(section, key, "");
	return s.length() == 0? def: string_To_time_t(s);
}

// Запись time_t в виде строки
void TIniFile::WriteTimeStr(const std::string &section, const std::string &key, time_t t) const
{
	WriteString(section, key, time_t_To_string(t));
}


bool TIniFile::SectionExists(const char *section) const throw()
{
	int			size(MAX_READ_BUF);
	TAutoBuffer	buf(1);
	bool		res(false);

	int	r(0);
	do
	{
		size *= 2;
		buf.resize(size);
		r = GetPrivateProfileSectionNames(buf.buffer, size, file.c_str());
	}
	while (r == (size - 2));

	const char	*curSection = buf.buffer;
	while(*curSection != '\0')
	{
#if _MSC_VER >=1800
		if(_stricmp(section, curSection) == 0)
#else
		if (stricmp(section, curSection) == 0)
#endif
		{
			res = true;
			break;
		}
		curSection += strlen(curSection) + 1;
	}
	return res;
}

bool TIniFile::SectionExists(const std::string &section) const throw()
{
	return SectionExists(section.c_str());
}

bool TIniFile::KeyExists(const char *section, const char *key) const throw()
{
	int			size(MAX_READ_BUF);
	TAutoBuffer	buf(1);
	bool		res(false);

	int	r(0);
	do
	{
		size *= 2;
		buf.resize(size);
		r = GetPrivateProfileString(section, NULL, "", buf.buffer, size, file.c_str());
	}
	while ( r == (size - 2));

	const char	*curKey = buf.buffer;
	while(*curKey != '\0')
	{
#if _MSC_VER >= 1800
		if(_stricmp(key, curKey) == 0)
#else
		if (stricmp(key, curKey) == 0)
#endif
		{
			res = true;
		}
		curKey += strlen(curKey) + 1;
	}
	return res;
}

bool TIniFile::KeyExists(const std::string &section, const std::string &key) const throw()
{
	return KeyExists(section.c_str(), key.c_str());
}

void TIniFile::ClearFile() const
{
	if(!DeleteFile(file.c_str()))
		throw TException(this, "Ошибка при удалении файла");
}

void TIniFile::ClearFileNoDelete() const
{
	ClearFile();
	CreateSection("mock");
	EraseSection("mock");
}


void TIniFile::CreateSection(const char *section) const
{
	if (!SectionExists(section))
	{
		if (WritePrivateProfileSection(section, "\0\0", file.c_str()) == 0)
			throw TException(this, "Ошибка создания секции");
	}
}

void TIniFile::CreateSection(const std::string &section) const
{
	CreateSection(section.c_str());
}

void TIniFile::EraseSection(const char *section) const
{
	if(WritePrivateProfileString(section, NULL, "", file.c_str()) == 0)
		throw TException(this, "Ошибка удаления секции");
}

void TIniFile::EraseSection(const std::string &section) const
{
	EraseSection(section.c_str());
}


void TIniFile::DeleteKey(const char *section, const char *key) const
{
	if(WritePrivateProfileString(section, key, NULL, file.c_str()) == 0)
		throw TException(this, "Ошибка удаления ключа");
}

void TIniFile::DeleteKey(const std::string& section, const std::string& key) const
{
	DeleteKey(section.c_str(), key.c_str());
}


void TIniFile::EnumSections(TStringList &sections) const throw()
{
	int			size(MAX_READ_BUF);
	TAutoBuffer	buf(1);

	sections.clear();
	int	r(0);
	do
	{
		size *= 2;
		buf.resize(size);
		r = GetPrivateProfileSectionNames(buf.buffer, size, file.c_str());
	}
	while (r == (size - 2));

	const char	*curSection = buf.buffer;
	while(*curSection != '\0')
	{	sections.push_back(curSection);
		curSection += strlen(curSection) + 1;
	}
}

// Данные методы пока компилируются только для 32-х разрядных приложений, так как у нас отсутствует 64-х разрядная библиотека PCRE
// Перечисление секций, соответствующих заданной подстроке/маске
void TIniFile::EnumSections(const char *mask, TStringList &sections) const throw()
{
	int			size(MAX_READ_BUF);
	TAutoBuffer	buf(1);

	sections.clear();
	int	r(0);
	do
	{
		size *= 2;
		buf.resize(size);
		r = GetPrivateProfileSectionNames(buf.buffer, size, file.c_str());
	} while (r == (size - 2));

	const char	*curSection = buf.buffer;
	while (*curSection != '\0')
	{
		if (TildeKDLCompareString(mask, curSection) == 0)
		{
			sections.push_back(curSection);
		}
		curSection += strlen(curSection) + 1;
	}
}
void TIniFile::EnumSections(const std::string &mask, TStringList &sections) const throw()
{
	EnumSections(mask.c_str(), sections);
}


void TIniFile::EnumKeys(const char *section, TStringList &keys) const throw()
{
	int			size(MAX_READ_BUF);
	TAutoBuffer	buf(1);

	keys.clear();
	int	r(0);
	do
	{
		size *= 2;
		buf.resize(size);
		r = GetPrivateProfileString(section, NULL, "", buf.buffer, size, file.c_str());
	}
	while (r == (size - 2));

	const char	*curKey = buf.buffer;
	while(*curKey != '\0')
	{
		keys.push_back(curKey);
		curKey += strlen(curKey) + 1;
	}
}

// Данные методы пока компилируются только для 32-х разрядных приложений, так как у нас отсутствует 64-х разрядная библиотека PCRE
// Перечисление ключей в секции
void TIniFile::EnumKeys(const char *section, const char *mask, TStringList &keys) const throw()
{
	int			size(MAX_READ_BUF);
	TAutoBuffer	buf(1);

	keys.clear();
	int	r(0);
	do
	{
		size *= 2;
		buf.resize(size);
		r = GetPrivateProfileString(section, NULL, "", buf.buffer, size, file.c_str());
	} while (r == (size - 2));

	const char	*curKey = buf.buffer;
	while (*curKey != '\0')
	{
		if (TildeKDLCompareString(mask, curKey) == 0)
		{
			keys.push_back(curKey);
		}
		curKey += strlen(curKey) + 1;
	}
}
void TIniFile::EnumKeys(const std::string &section, const std::string &mask, TStringList &keys) const throw()
{
	EnumKeys(section.c_str(), mask.c_str(), keys);
}

bool TIniFile::IsEmpty()
{
	TStringList sections;
	EnumSections(sections);
	if (sections.empty())
		return true;

	for (TStringList::iterator section = sections.begin(); section != sections.end(); ++section)
	{
		TStringList keys;
		EnumKeys(section->c_str(), keys);
		if (!keys.empty())
			return false;
	}
	return true;
}



TMultiIniFile::TMultiIniFile(const std::map<unsigned long, std::string> &listFiles)
{
	SetFileNames(listFiles);
}

void TMultiIniFile::SetFileNames(const std::map<unsigned long, std::string> &listFiles)
{
	for (std::map<unsigned long, std::string>::const_iterator ilf = listFiles.begin(); ilf != listFiles.end(); ++ilf)
	{
		iniFiles[ilf->first] = TIniFile(ilf->second);
	}
}

// Чтение/запись строки
std::string TMultiIniFile::ReadString(const std::string &section, const std::string &key, const std::string &def, unsigned long &fileIndex) const
{
	std::string	res(def);
	fileIndex = 0;
	for (std::map<unsigned long, TIniFile>::const_iterator iif = iniFiles.begin(); iif != iniFiles.end(); ++iif)
	{
		if (iif->second.KeyExists(section, key) == false)
			continue;

		res = iif->second.ReadString(section, key, "");
		fileIndex = iif->first;
	}
	return res;
}

void TMultiIniFile::WriteString(const unsigned long fileIndex, const std::string &section, const std::string &key, const std::string &value)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	unsigned long index = 0;
	std::string prevValue = ReadString(section, key, "", index);
	if (prevValue == value)
		return;

	iniFiles[fileIndex].WriteString(section, key, value);
	if (fileIndex == 1 && iniFiles.find(2) != iniFiles.end())
		iniFiles[2].DeleteKey(section, key);
}

// Чтение числовых значений в виде строки
std::string TMultiIniFile::ReadIntegerString(const std::string &section, const std::string &key, const std::string &def, unsigned long &fileIndex) const
{
	std::string	res(def);
	fileIndex = 0;
	for (std::map<unsigned long, TIniFile>::const_iterator iif = iniFiles.begin(); iif != iniFiles.end(); ++iif)
	{
		if (iif->second.KeyExists(section, key) == false)
			continue;

		std::string tmp = iif->second.ReadString(section, key, "");
		if (tmp.empty())
			continue;

		res = tmp;
		fileIndex = iif->first;
	}
	return res;
}

// Чтение/запись числа
int TMultiIniFile::ReadInteger(const std::string &section, const std::string &key, int def, unsigned long &fileIndex) const
{
	std::string	tmp = ReadIntegerString(section, key, "", fileIndex);
	if (tmp.empty())
		return def;

	return atoi(tmp.c_str());
}

int TMultiIniFile::ReadHexInteger(const std::string &section, const std::string &key, int def, unsigned long &fileIndex) const
{
	std::string	tmp = ReadIntegerString(section, key, "", fileIndex);
	if (tmp.empty())
		return def;

	return std::strtol(tmp.c_str(), 0, 16);
}

void TMultiIniFile::WriteInteger(const unsigned long fileIndex, const std::string &section, const std::string &key, int value)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	unsigned long index = 0;
	int prevValue = ReadInteger(section, key, INT_MAX, index);
	if (prevValue == value)
		return;

	iniFiles[fileIndex].WriteInteger(section, key, value);
	if (fileIndex == 1 && iniFiles.find(2) != iniFiles.end())
		iniFiles[2].DeleteKey(section, key);
}

//Чтение/запись беззнакового числа
unsigned long TMultiIniFile::ReadULong(const std::string &section, const std::string &key, unsigned long def, unsigned long &fileIndex) const
{
	std::string	tmp = ReadIntegerString(section, key, "", fileIndex);
	if (tmp.empty())
		return def;

	return std::strtoul(tmp.c_str(), NULL, 10);
}

void TMultiIniFile::WriteULong(const unsigned long fileIndex, const std::string &section, const std::string &key, unsigned long value)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	unsigned long index = 0;
	unsigned long prevValue = ReadULong(section, key, ULONG_MAX, index);
	if (prevValue == value)
		return;

	iniFiles[fileIndex].WriteULong(section, key, value);
	if (fileIndex == 1 && iniFiles.find(2) != iniFiles.end())
		iniFiles[2].DeleteKey(section, key);
}

// Чтение/запись числа с плавающей запятой
double TMultiIniFile::ReadDouble(const std::string &section, const std::string &key, double def, unsigned long &fileIndex) const
{
	std::string	tmp = ReadIntegerString(section, key, "", fileIndex);
	if (tmp.empty())
		return def;

	return strtod(tmp.c_str(),0);
}

void TMultiIniFile::WriteDouble(const unsigned long fileIndex, const std::string &section, const std::string &key, double value)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	unsigned long index = 0;
	double prevValue = ReadDouble(section, key, INT_MAX, index);
	if (prevValue == value)
		return;

	iniFiles[fileIndex].WriteDouble(section, key, value);
	if (fileIndex == 1 && iniFiles.find(2) != iniFiles.end())
		iniFiles[2].DeleteKey(section, key);
}

// Чтение/запись булевой переменной
bool TMultiIniFile::ReadBool(const std::string &section, const std::string &key, bool def, unsigned long &fileIndex) const
{
	int	tmp = ReadInteger(section, key, -1, fileIndex);
	if (tmp == -1)
		return def;

	return (tmp != 0);
}

void TMultiIniFile::WriteBool(const unsigned long fileIndex, const std::string &section, const std::string &key, bool value)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	unsigned long index = 0;
	bool prevValue = ReadBool(section, key, false, index);
	if (prevValue == value)
		return;

	iniFiles[fileIndex].WriteBool(section, key, value);
	if (fileIndex == 1 && iniFiles.find(2) != iniFiles.end())
		iniFiles[2].DeleteKey(section, key);
}

// Чтение/запись time_t в виде строки
time_t TMultiIniFile::ReadTimeStr(const std::string &section, const std::string &key, time_t def, unsigned long &fileIndex) const
{
	std::string	s = ReadIntegerString(section, key, "", fileIndex);
	return s.empty() == true ? def : string_To_time_t(s);
}

void TMultiIniFile::WriteTimeStr(unsigned long fileIndex, const std::string &section, const std::string &key, time_t value)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	unsigned long index = 0;
	time_t prevValue = ReadTimeStr(section, key, LONG_MAX, index);
	if (prevValue == value)
		return;

	iniFiles[fileIndex].WriteTimeStr(section, key, value);
	if (fileIndex == 1 && iniFiles.find(2) != iniFiles.end())
		iniFiles[2].DeleteKey(section, key);
}

// Чтение/запись парольной (закрытой от прямого просмотра) строки
std::string TMultiIniFile::ReadPassword(const std::string &section, const std::string &key, const std::string &def, unsigned long &fileIndex) const
{
	std::stringstream		res;
	int						c, c1;
	std::string				s;
	std::string::size_type	a;

	s = ReadString(section, key, def, fileIndex);
	if (s == def)
		return s;

	// Восстановление
	for (c1 = 0x95, a = 0; a < s.length(); a += 2)
	{
		std::stringstream	ss(s.substr(a, 2), std::ios_base::in);
		ss >> std::hex >> c;
		c ^= c1;
		res << static_cast<char>(c);
		c1 = c;
	}

	return res.str();
}

void TMultiIniFile::WritePassword(const unsigned long fileIndex, const std::string &section, const std::string &key, std::string value)
{
	std::stringstream		ss(std::ios_base::out);
	std::string::size_type	a;
	int						c1, c2;

	// Скрытие от просмотра
	for (c1 = 0x95, a = 0; a < value.length(); ++a)
	{
		c2 = value[a];
		value[a] ^= c1;
		c1 = c2;
	}

	// Сохранение в виде набора шестнадцатиричных чисел
	for (a = 0; a < value.length(); ++a)
		ss << std::hex << std::setw(2) << std::setfill('0') << std::right << std::noshowbase << std::uppercase
		<< static_cast<unsigned int>(static_cast<unsigned char>(value[a]));
	WriteString(fileIndex, section, key, ss.str());
}

// Проверка существования секции
bool TMultiIniFile::SectionExists(unsigned long fileIndex, const std::string &section)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	return iniFiles[fileIndex].SectionExists(section);
}

// Проверка существования ключа
bool TMultiIniFile::KeyExists(unsigned long fileIndex, const std::string &section, const std::string &key)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	return iniFiles[fileIndex].KeyExists(section, key);
}

// Очистка всего файла (при инициализации необходимо указать полное имя файла)
void TMultiIniFile::ClearFile(unsigned long fileIndex)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	iniFiles[fileIndex].ClearFile();
}

// Очистка всего файла. Сам файл не удаляется в отличие от функции ClearFile(), которая просто удаляет файл
void TMultiIniFile::ClearFileNoDelete(unsigned long fileIndex)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	iniFiles[fileIndex].ClearFileNoDelete();
}

// Удаление секции во всех файлах
void TMultiIniFile::EraseSection(const std::string &section)
{
	std::map<unsigned long, TIniFile2>::const_iterator	file = iniFiles.begin();
    std::map<unsigned long, TIniFile2>::const_iterator	fileEnd = iniFiles.end();
    for ( ; file != fileEnd; ++file)
    {
        file->second.EraseSection(section);
    }
}
// Удаление секции в заданном файле
void TMultiIniFile::EraseSection(unsigned long fileIndex, const std::string &section)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	iniFiles[fileIndex].EraseSection(section);
}

// Удаление ключа во всех файлах
void TMultiIniFile::DeleteKey(const std::string &section, const std::string &key)
{
	std::map<unsigned long, TIniFile2>::const_iterator	file = iniFiles.begin();
    std::map<unsigned long, TIniFile2>::const_iterator	fileEnd = iniFiles.end();
    for ( ; file != fileEnd; ++file)
    {
        file->second.DeleteKey(section, key);
    }
}
// Удаление ключа в заданном файле
void TMultiIniFile::DeleteKey(unsigned long fileIndex, const std::string &section, const std::string &key)
{
	if (fileIndex > iniFiles.size())
		throw TWin32Error("Задан некорректный индекс файла");

	iniFiles[fileIndex].DeleteKey(section.c_str(), key.c_str());
}

// Перечисление секций
void TMultiIniFile::EnumSections(TStringList &sections) const
{
    std::set<std::string>	sectionsFullList;

	std::map<unsigned long, TIniFile2>::const_iterator	file = iniFiles.begin();
    std::map<unsigned long, TIniFile2>::const_iterator	fileEnd = iniFiles.end();
    for ( ; file != fileEnd; ++file)
    {
        TStringList fileSections;
    	file->second.EnumSections(fileSections);
    	sectionsFullList.insert(fileSections.begin(), fileSections.end());
    }

    sections.clear();
    sections.assign(sectionsFullList.begin(), sectionsFullList.end());
}
// Перечисление ключей в секции
void TMultiIniFile::EnumKeys(const std::string &section, TStringList &keys) const
{
	std::set<std::string>	keysFullList;

	std::map<unsigned long, TIniFile2>::const_iterator	file = iniFiles.begin();
    std::map<unsigned long, TIniFile2>::const_iterator	fileEnd = iniFiles.end();
    for ( ; file != fileEnd; ++file)
    {
        TStringList fileKeys;
    	file->second.EnumKeys(section.c_str(), fileKeys);
    	keysFullList.insert(fileKeys.begin(), fileKeys.end());
    }

    keys.clear();
    keys.assign(keysFullList.begin(), keysFullList.end());
}
