/*
 * SoX bit-rot detection file; cobbled together
 */

#define BYTE uint8_t
#define CHAR char
#define DWORD_PTR DWORD *
#define DWORD uint32_t
#define HANDLE void *
#define LPCSTR char *
#define LPCVOID void *
#define LPDWORD DWORD *
#define LPSTR char const *
#define UINT DWORD
#define WCHAR int16_t
#define WINAPI
#define WIN_BOOL int
#define WORD uint16_t
typedef char GUID[16];

enum {
  FALSE,
  TRUE,
  FORMAT_MESSAGE_FROM_SYSTEM,
  FORMAT_MESSAGE_IGNORE_INSERTS,
  INFINITE,
  CALLBACK_EVENT
};

DWORD CloseHandle(HANDLE);
DWORD FormatMessageA(DWORD,LPCVOID,DWORD,DWORD,LPSTR, DWORD,LPDWORD);
DWORD GetLastError(void);
DWORD WaitForSingleObject(HANDLE, DWORD);
HANDLE CreateEventA(LPCVOID,WIN_BOOL,WIN_BOOL,LPCSTR);
