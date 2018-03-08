//OSAbstraction.cpp

#ifndef _WIN32
#include <pthread.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#include <wchar.h>
#include <stdarg.h>

#include "Core/Defines.h"
#else
#define _WIN32_WINNT 0x0400// necessary for IsDebuggerPresent
#include <Windows.h>
#endif
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int OIsDebuggerPresent(void)
{
#ifdef _WIN32
	return IsDebuggerPresent() ? 1 : 0;
#else
	return false;
#endif
}

#ifndef _WIN32
unsigned long OGetThreadID()
{
	return (unsigned long)pthread_self();
}

unsigned long OGetSystemTime()
{
	struct timeval tv;

	if (gettimeofday(&tv, NULL) != 0)
		return 0;

	unsigned int seconds = tv.tv_sec;
	unsigned int microseconds = tv.tv_usec;

	return (seconds * 1000) + (microseconds / 1000);
}

unsigned long long OGetSystemTime64()
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0)
		return 0;

	unsigned long long systemTime = tv.tv_sec;
	systemTime *= 1000;
	systemTime += (tv.tv_usec / 1000);

	return systemTime;
}

void OSleep(unsigned long dwMS)
{
	usleep(dwMS * 1000);
}

char* strrev(char* str)
{
	char *p1, *p2;

	if (!str || !*str)
		return str;
	for (p1 = str, p2 = str + strlen(str) - 1; p2 > p1; ++p1, --p2)
	{
		*p1 ^= *p2;
		*p2 ^= *p1;
		*p1 ^= *p2;
	}
	return str;
}
#else
char* Oultoa(unsigned long value, char* str, int radix)
{
	return ultoa(value, str, radix);
}
void OSleep(unsigned long lTime)
{
	Sleep(lTime);
}
#endif

#ifndef _WIN32
char* Oitoa(int value, char* str, int radix)
{
#if defined(linux) || defined(QNX_OS)
	int rem = 0;
	int pos = 0;
	char ch = '!';
	do
	{
		rem = value % radix;
		value /= radix;
		if (16 == radix)
		{
			if (rem >= 10 && rem <= 15)
			{
				switch (rem)
				{
					case 10:
						ch = 'a';
						break;
					case 11:
						ch = 'b';
						break;
					case 12:
						ch = 'c';
						break;
					case 13:
						ch = 'd';
						break;
					case 14:
						ch = 'e';
						break;
					case 15:
						ch = 'f';
						break;
				}
			}
		}
		if ('!' == ch)
		{
			str[pos++] = (char)(rem + 0x30);
		}
		else
		{
			str[pos++] = ch;
		}
	} while (value != 0);
	str[pos] = '\0';
	return strrev(str);
#else

	return itoa(value, str, radix);

#endif
}

char* Oultoa(unsigned long value, char* str, int radix)
{
#if defined(linux) || defined(QNX_OS)
	int rem = 0;
	int pos = 0;
	char ch = '!';

	do
	{
		rem = value % radix;
		value /= radix;

		if (16 == radix)
		{
			if (rem >= 10 && rem <= 15)
			{
				switch (rem)
				{
					case 10:
						ch = 'a';
						break;
					case 11:
						ch = 'b';
						break;
					case 12:
						ch = 'c';
						break;
					case 13:
						ch = 'd';
						break;
					case 14:
						ch = 'e';
						break;
					case 15:
						ch = 'f';
						break;
				}
			}
		}
		if ('!' == ch)
		{
			str[pos++] = (char)(rem + 0x30);
		}
		else
		{
			str[pos++] = ch;
		}
	} while (value != 0);

	str[pos] = '\0';

	return strrev(str);

#else

	return ultoa(value, str, radix);

#endif
}

// a quick and dirty implementation of _wtoi.
// there are better ways to do this --jq 9/22/10
int Owtoi(const wchar_t* str)
{
#ifdef ANDROID//Added new implementation for Android. Previously everything was flipped (12345 was 54321)
	return (int)wcstol(str, NULL, 10);
#elif defined(linux) || defined(QNX_OS)
	int res = 0, mult = 1;

	if (*str == L'\0')
		return 0;
	do
	{
		if (*str > L'0' && *str <= L'9')
			res += (((int)(*str - L'0')) * mult);
	} while (mult *= 10, *++str != '\0');
	return res;
#else
	return _wtoi(str);
#endif
}

int Owcslen(const wchar_t* str)
{
	const wchar_t* s = str;
	for (; *s; ++s)
		;
	return (s - str);
}

wchar_t* Owcschr(const wchar_t* str, wchar_t c)
{
	const wchar_t* cur = str;
	while (*cur)
	{
		if (*cur == c)
			return (wchar_t*)cur;
		++cur;
	}
	return NULL;
}

wchar_t* Owcsrchr(const wchar_t* str, wchar_t c)
{
	int len = Owcslen(str);
	if (len <= 0)
		return NULL;
	const wchar_t* cur = str + len - 1;
	while (cur > str)
	{
		if (*cur == c)
			return (wchar_t*)cur;
		--cur;
	}
	return NULL;
}

int Owcsncmp(const wchar_t* string1, const wchar_t* string2, size_t count)
{
#if !defined(ANDROID) && !defined(QNX_OS)
	return wcsncmp(string1, string2, count);
#else
	if (string1 == string2)
		return 0;
	if (!string1)
		return -1;
	if (!string2)
		return 1;
	for (size_t i = 0; i < count; ++i)
	{
		int delta = (*string1++) - (*string2++);
		if (delta)
			return delta;
	}
	return 0;
#endif
}

wchar_t* Owcsncpy(wchar_t* strDest, const wchar_t* strSource, size_t count)
{
	wchar_t* pDestCur;
	wchar_t* pSrcCur;

	if (strSource == NULL || strDest == NULL)
		return NULL;

	pDestCur = strDest;
	pSrcCur = (wchar_t*)strSource;
	size_t nCopied = 0;
	while (1)
	{
		if (*pSrcCur == 0)
		{
			*pDestCur = 0;
			break;
		}
		if (nCopied == count)
			break;

		*pDestCur = *pSrcCur;

		pDestCur++;
		pSrcCur++;
		++nCopied;
	}

	return strDest;
}

#endif

void wchar_to_char(char* Dest, const wchar_t* Src)
{
	const char* result = Dest;
	while (1)
	{
		if (*Src == 0)
			break;
		*Dest++ = *Src++;
	}

	*Dest = 0;
}


void char_to_wchar(wchar_t* Dest, const char* Src)
{
	const wchar_t* result = Dest;
	while (1)
	{
		if (*Src == 0)
			break;
		*Dest++ = *Src++;
	}
	*Dest = 0;
}

#if !defined(_WIN32)
wchar_t* Owcscpy(wchar_t* strDestination, const wchar_t* strSource)
{
	wchar_t* pDestCur;
	wchar_t* pSrcCur;

	if (strSource == NULL || strDestination == NULL)
		return NULL;

	pDestCur = strDestination;
	pSrcCur = (wchar_t*)strSource;

	while (1)
	{
		if (*pSrcCur == 0)
		{
			*pDestCur = 0;
			break;
		}

		*pDestCur = *pSrcCur;

		pDestCur++;
		pSrcCur++;
	}

	return strDestination;
}

//Compare characters of two strings without regard to case.
int Owcsnicmp(const wchar_t* string1, const wchar_t* string2, size_t count)
{
#if !defined(ANDROID) && !defined(QNX_OS)
	return _wcsnicmp(string1, string2, count);
#else
	wchar_t char1, char2;
	for (size_t i = 0; i < count; ++i)
	{
		char1 = towupper(*string1++);
		char2 = towupper(*string2++);
		if(char1 != char2)
			return (char1 - char2);
	}
	return 0;
#endif
}

int Owcsicmp(const wchar_t* string1, const wchar_t* string2)
{
#if !defined(ANDROID) && !defined(QNX_OS)
	return _wcsicmp(string1, string2);
#else
	wchar_t char1, char2;
	while (*string1 || *string2)
	{
		char1 = towupper(*string1++);
		char2 = towupper(*string2++);
		if(char1 != char2)
			return (char1 - char2);
	}
	return 0;
#endif
}

//Attempts to not new[] unless it has to
int OsprintfW(wchar_t* Buffer, int MaxChars, const wchar_t* format, ...)
{
	static const int STACKFORMAT_SIZE = 64;
	char stackFormat[STACKFORMAT_SIZE];
	static const int STACKBUFFER_SIZE = 1024;
	char stackBuffer[STACKBUFFER_SIZE];

	char* TempFormat = NULL;
	char* TempBuffer = NULL;
	char* finalFormat = stackFormat;
	char* finalBuffer = stackBuffer;
	int len = Owcslen(format);

	if (len >= STACKFORMAT_SIZE - 1)
		finalFormat = TempFormat = new char[len + 1];
	if (MaxChars >= STACKBUFFER_SIZE - 1)
		finalBuffer = TempBuffer = new char[MaxChars + 1];

	wchar_to_char(finalFormat, format);

	va_list args;
	va_start(args, format);
	
#ifdef ANDROID
	//Added for Android 4.4 support
	if(Owcsicmp(format, L"%ls") == 0){
		wchar_t* arg = va_arg(args, wchar_t*);
		len = Owcslen(arg);
		if(len > MaxChars)
			len = MaxChars;
		Owcsncpy(Buffer, arg, len);
		return len;
	}
#endif

	len = vsnprintf(finalBuffer, MaxChars, finalFormat, args);

	va_end(args);

	char_to_wchar(Buffer, finalBuffer);

	if (TempFormat)
		delete[] TempFormat;

	if (TempBuffer)
		delete[] TempBuffer;

	//if len was -1 then the sprintf was truncated becaues MaxChars was less
	//than the what the format string and parameters needed.
	return len == -1 ? MaxChars : len;
}

double Owcstod(const wchar_t* from, wchar_t** endptr)
{
#if defined(_WIN32)
	return wcstod(from, endptr);
#else
	int len = Owcslen(from);

	char* temp = new char[len + 1];
	wchar_to_char(temp, from);

	char* dummy;
	double ret = strtod(temp, &dummy);

	delete[] temp;

	if (endptr)
	{
		if (dummy == NULL)
			*endptr = NULL;
		else
			*endptr = (wchar_t*)from + (dummy - temp);
	}
	return ret;
#endif
}
#endif

long inline chartoval(const wchar_t c)
{
	if (c >= L'0' && c <= L'9')
		return (unsigned long)(c - L'0');
	else
	{
		switch (c)
		{
			case L'a':
			case L'A':
				return 10;
			case L'b':
			case L'B':
				return 11;
			case L'c':
			case L'C':
				return 12;
			case L'd':
			case L'D':
				return 13;
			case L'e':
			case L'E':
				return 14;
			case L'f':
			case L'F':
				return 15;
		}
	}
	return -1;
}

template <typename INT_T>
INT_T Owcsto(const wchar_t* nptr, wchar_t** endptr, int base)
{
	const bool is64 = sizeof(INT_T) == 8;
	static const INT_T MAX = ~0;
	static const INT_T binPlaces[] = { 1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7, 1 << 8, 1 << 9, 1 << 10, 1 << 11, 1 << 12,
		1 << 13, 1 << 14, 1 << 15, 1 << 16, 1 << 17, 1 << 18, 1 << 19, 1 << 20, 1 << 21, 1 << 22, 1 << 23, 1 << 24, 1 << 25, 1 << 26,
		1 << 27, 1 << 28, 1 << 29, 1 << 30, ((uint64_t)(1ull << 31ull)), ((uint64_t)(1ull << 31ull)) << 1ull,
		((uint64_t)(1ull << 31ull)) << 2ull, ((uint64_t)(1ull << 31ull)) << 3ull, ((uint64_t)(1ull << 31ull)) << 4ull,
		((uint64_t)(1ull << 31ull)) << 5ull, ((uint64_t)(1ull << 31ull)) << 6ull, ((uint64_t)(1ull << 31ull)) << 7ull,
		((uint64_t)(1ull << 31ull)) << 8ull, ((uint64_t)(1ull << 31ull)) << 9ull, ((uint64_t)(1ull << 31ull)) << 10ull,
		((uint64_t)(1ull << 31ull)) << 11ull, ((uint64_t)(1ull << 31ull)) << 12ull, ((uint64_t)(1ull << 31ull)) << 13ull,
		((uint64_t)(1ull << 31ull)) << 14ull, ((uint64_t)(1ull << 31ull)) << 15ull, ((uint64_t)(1ull << 31ull)) << 16ull,
		((uint64_t)(1ull << 31ull)) << 17ull, ((uint64_t)(1ull << 31ull)) << 18ull, ((uint64_t)(1ull << 31ull)) << 19ull,
		((uint64_t)(1ull << 31ull)) << 20ull, ((uint64_t)(1ull << 31ull)) << 21ull, ((uint64_t)(1ull << 31ull)) << 22ull,
		((uint64_t)(1ull << 31ull)) << 23ull, ((uint64_t)(1ull << 31ull)) << 24ull, ((uint64_t)(1ull << 31ull)) << 25ull,
		((uint64_t)(1ull << 31ull)) << 26ull, ((uint64_t)(1ull << 31ull)) << 27ull, ((uint64_t)(1ull << 31ull)) << 28ull,
		((uint64_t)(1ull << 31ull)) << 29ull, ((uint64_t)(1ull << 31ull)) << 30ull, ((uint64_t)(1ull << 31ull)) << 31ull };
	static const INT_T decPlaces[] = { 1, 10, 100, 1000, 10000, 100000, 1000000, 10000000, 100000000, 1000000000, 10000000000ull,
		100000000000ull, 1000000000000ull, 10000000000000ull, 100000000000000ull, 1000000000000000ull, 10000000000000000ull,
		100000000000000000ull, 1000000000000000000ull, 10000000000000000000ull };
	static const INT_T hexPlaces[] = { 0x1, 0x10, 0x100, 0x1000, 0x10000, 0x100000, 0x1000000, 0x10000000, 0x100000000ull, 0x1000000000ull,
		0x10000000000ull, 0x100000000000ull, 0x1000000000000ull, 0x10000000000000ull, 0x100000000000000ull, 0x1000000000000000ull };

	const wchar_t* curptr = nptr;
	INT_T ret = 0;

	if (nptr)
	{
		int len = 0;

		while (*curptr)
		{
			const wchar_t c = *curptr++;
			long val = chartoval(c);
			if (val < 0 || val >= base)
				break;
			++len;
		}

		const INT_T* places = NULL;
		if (base == 10 && len <= (is64 ? 20 : 10))
			places = decPlaces;
		else if (base == 16 && len <= (is64 ? 16 : 8))
			places = hexPlaces;
		else if (base == 2 && len <= (is64 ? 64 : 32))
			places = binPlaces;

		if (places)
		{
			curptr = nptr;
			for (unsigned long i = 0; i < len; ++i)
			{
				INT_T place = chartoval(*curptr);
				unsigned long power = len - i - 1;
				if (power)
				{
					if (places)
						place *= places[power];
					else
					{
						while (power--)
							place *= base;
					}
				}
				INT_T sum = ret + place;
				if (sum < ret)//overflow
				{
					ret = MAX;
					break;
				}
				ret = sum;
				++curptr;
			}
		}
	}

	if (endptr)
		*endptr = const_cast<wchar_t*>(curptr);

	return ret;
}

#if !defined(_WIN32)
unsigned long Owcstoul(const wchar_t* nptr, wchar_t** endptr, int base)
{
#ifndef ANDROID
	return wcstoul(nptr, endptr, base);
#else
	return Owcsto<unsigned long>(nptr, endptr, base);
#endif
}

long Owcstol(const wchar_t* nptr, wchar_t** endptr, int base)
{
#if !defined(ANDROID)
	return wcstol(nptr, endptr, base);
#else
	if (nptr && (*nptr == L'-'))
	{
		unsigned long ret = Owcsto<unsigned long>(nptr + 1, endptr, base);
		if (ret == ULONG_MAX)
			return LONG_MIN;
		else
			return ((long)ret) * -1;
	}
	else
		return Owcsto<unsigned long>(nptr, endptr, base);
#endif
}
#endif

uint64_t Owcstoui64(const wchar_t* nptr, wchar_t** endptr, int base)
{
	return Owcsto<uint64_t>(nptr, endptr, base);
}

#ifndef _UI64_MAX
#define _UI64_MAX 0xffffffffffffffffull
#endif

#ifndef _I64_MIN
#define _I64_MIN -9223372036854775808ull
#endif

int64_t Owcstoi64(const wchar_t* nptr, wchar_t** endptr, int base)
{
	if (nptr && (*nptr == L'-'))
	{
		uint64_t ret = Owcsto<uint64_t>(nptr + 1, endptr, base);
		if (ret == _UI64_MAX)
			return _I64_MIN;
		else
			return ((int64_t)ret) * -1;
	}
	else
		return Owcsto<uint64_t>(nptr, endptr, base);
}

#if !defined(_WIN32)
//Custom implementation of wcstombs that simply casts... this function's parameters are very weird!
int Owcstombs(char* mbstr, const wchar_t* wcstr, size_t max)
{
#ifdef ANDROID
	int ret = 0;
	if (wcstr == NULL)
		return -1;
	bool die = false;
	do
	{
		const wchar_t c = *wcstr++;
		if (mbstr)
		{
			*mbstr++ = (const char)c;
			if (--max == 0)
				die = true;
		}

		if (c == L'\0')
			die = true;
		else
			++ret;

	} while (!die);
	return ret;
#else
	return wcstombs(mbstr, wcstr, max);
#endif
}
#endif

#if !defined(_WIN32)
size_t Ombstowcs(wchar_t* dest, const char* src, size_t max)
{
	return mbstowcs(dest, src, max);
}
#endif

#if !defined(_WIN32)
int Owchdir(const wchar_t* dirname)
{
#if defined(ANDROID) || defined(linux) || defined(QNX_OS)
	if (dirname == NULL)
		return -1;
	int len = Owcslen(dirname);
	char* name = new char[len + 1];
	wchar_to_char(name, dirname);
	name[len] = '\0';
	int ret = chdir(name);
	delete[] name;
	return ret;
#else
	return _wchdir(dirname);
#endif
}

int Owmkdir(const wchar_t* dirname)
{
#if defined(ANDROID) || defined(linux) || defined(QNX_OS)
	if (dirname == NULL)
		return -1;
	int len = Owcslen(dirname);
	char* name = new char[len + 1];
	wchar_to_char(name, dirname);
	name[len] = '\0';
	int ret = mkdir(name, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	delete[] name;
	return ret;
#else
	return _wmkdir(dirname);
#endif
}

FILE* Owfopen(const wchar_t* file, const wchar_t* mode)
{
#if defined(ANDROID) || defined(linux) || defined(QNX_OS)
	if (file == NULL || mode == NULL)
		return NULL;
	int filelen = Owcslen(file);
	int modelen = Owcslen(mode);
	char* _file = new char[filelen + 1];
	char* _mode = new char[modelen + 1];
	wchar_to_char(_file, file);
	wchar_to_char(_mode, mode);
	_file[filelen] = L'\0';
	_mode[modelen] = L'\0';
	for (int i = 0; i < filelen; ++i)
	{
		if (_file[i] == '\\')
			_file[i] = '/';
	}
	FILE* ret = fopen(_file, _mode);
	delete[] _file;
	delete[] _mode;
	return ret;
#else
	return _wfopen(file, mode);
#endif
}

wchar_t* Owgetcwd(wchar_t* buffer, int maxlen)
{
#if defined(ANDROID) || defined(linux) || defined(QNX_OS)
	char* dest = new char[maxlen + 1];
	char* ret = getcwd(dest, maxlen);
	if (ret != NULL)
	{
		for (int i = 0; i < maxlen + 1; ++i)
		{
			buffer[i] = dest[i];
			if (dest[i] == '\0')
				break;
		}
		delete[] dest;
		return buffer;
	}
	else
	{
		delete[] dest;
		return NULL;
	}
#else
	return _wgetcwd(buffer, maxlen);
#endif
}

int Owunlink(const wchar_t* filename)
{
#if defined(ANDROID) || defined(linux) || defined(QNX_OS)
	int len = Owcslen(filename);
	char* fn = new char[len + 1];
	wchar_to_char(fn, filename);
	fn[len] = '\0';
	int ret = unlink(fn);
	delete[] fn;
	return ret;
#else
	return _wunlink(filename);
#endif
}
#endif
