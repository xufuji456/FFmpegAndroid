/*
 * SoX bit-rot detection file; cobbled together
 */
#define HWAVEIN HANDLE
#define HWAVEOUT HANDLE
#define MMRESULT UINT
#define MMVERSION UINT
#define UNALIGNED

enum {
  MMSYSERR_NOERROR,
  MAXPNAMELEN,
  WAVE_FORMAT_PCM,
  WAVE_FORMAT_EXTENSIBLE,
  WAVE_FORMAT_QUERY,
  WAVE_MAPPER,
  WAVERR_STILLPLAYING,
  WHDR_DONE,
  WHDR_INQUEUE
};

typedef struct wavehdr_tag {
  LPSTR lpData;
  DWORD dwBufferLength;
  DWORD dwBytesRecorded;
  DWORD dwUser;
  DWORD dwFlags;
  DWORD dwLoops;
  struct wavehdr_tag *lpNext;
  DWORD reserved;
} WAVEHDR,*PWAVEHDR,*LPWAVEHDR;

typedef struct _WAVEFORMATEX {
  WORD   wFormatTag;
  WORD   nChannels;
  DWORD  nSamplesPerSec;
  DWORD  nAvgBytesPerSec;
  WORD   nBlockAlign;
  WORD   wBitsPerSample;
  WORD   cbSize;
} WAVEFORMATEX, *PWAVEFORMATEX, *NPWAVEFORMATEX, *LPWAVEFORMATEX;

typedef struct tagWAVEINCAPSA {
  WORD wMid;
  WORD wPid;
  MMVERSION vDriverVersion;
  CHAR szPname[MAXPNAMELEN];
  DWORD dwFormats;
  WORD wChannels;
  WORD wReserved1;
} WAVEINCAPSA,*PWAVEINCAPSA,*LPWAVEINCAPSA;

typedef struct tagWAVEOUTCAPSA {
  WORD wMid;
  WORD wPid;
  MMVERSION vDriverVersion;
  CHAR szPname[MAXPNAMELEN];
  DWORD dwFormats;
  WORD wChannels;
  WORD wReserved1;
  DWORD dwSupport;
} WAVEOUTCAPSA,*PWAVEOUTCAPSA,*LPWAVEOUTCAPSA;

MMRESULT WINAPI waveInAddBuffer(HWAVEIN,LPWAVEHDR,UINT);
MMRESULT WINAPI waveInClose(HWAVEIN);
MMRESULT WINAPI waveInGetDevCapsA(UINT,LPWAVEINCAPSA,UINT);
MMRESULT WINAPI waveInGetNumDevs(void);
MMRESULT WINAPI waveInOpen(HWAVEIN*,UINT,WAVEFORMATEX*,DWORD*,DWORD*,DWORD);
MMRESULT WINAPI waveInPrepareHeader(HWAVEIN,LPWAVEHDR,UINT);
MMRESULT WINAPI waveInReset(HWAVEIN);
MMRESULT WINAPI waveInStart(HWAVEIN);
MMRESULT WINAPI waveOutClose(HWAVEOUT);
MMRESULT WINAPI waveOutGetDevCapsA(UINT,LPWAVEOUTCAPSA,UINT);
MMRESULT WINAPI waveOutGetNumDevs(void);
MMRESULT WINAPI waveOutOpen(HWAVEOUT*,UINT,WAVEFORMATEX*,DWORD*,DWORD*,DWORD);
MMRESULT WINAPI waveOutPrepareHeader(HWAVEOUT,LPWAVEHDR,UINT);
MMRESULT WINAPI waveOutReset(HWAVEOUT);
MMRESULT WINAPI waveOutWrite(HWAVEOUT,LPWAVEHDR,UINT);

typedef struct {
  WAVEFORMATEX Format;
  union {
    WORD wValidBitsPerSample;
    WORD wSamplesPerBlock;
    WORD wReserved;
  } Samples;
  DWORD        dwChannelMask;
  GUID         SubFormat;
} WAVEFORMATEXTENSIBLE, *PWAVEFORMATEXTENSIBLE;
