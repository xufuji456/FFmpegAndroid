#ifndef HAVE_COREAUDIO
/*
 * SoX bit-rot detection file; cobbled together
 */

enum {
  kAudioHardwarePropertyProcessIsMaster,
  kAudioHardwarePropertyIsInitingOrExiting,
  kAudioHardwarePropertyDevices,
  kAudioHardwarePropertyDefaultInputDevice,
  kAudioHardwarePropertyDefaultOutputDevice,
  kAudioHardwarePropertyDefaultSystemOutputDevice,
  kAudioHardwarePropertyDeviceForUID,
  kAudioHardwarePropertySleepingIsAllowed,
  kAudioHardwarePropertyUnloadingIsAllowed,
  kAudioHardwarePropertyHogModeIsAllowed,
  kAudioHardwarePropertyRunLoop,
  kAudioHardwarePropertyPlugInForBundleID
};

enum {
  kAudioObjectPropertyClass,
  kAudioObjectPropertyOwner,
  kAudioObjectPropertyCreator,
  kAudioObjectPropertyName,
  kAudioObjectPropertyManufacturer,
  kAudioObjectPropertyElementName,
  kAudioObjectPropertyElementCategoryName,
  kAudioObjectPropertyElementNumberName,
  kAudioObjectPropertyOwnedObjects,
  kAudioObjectPropertyListenerAdded,
  kAudioObjectPropertyListenerRemoved
};

enum {
  kAudioDevicePropertyDeviceName,
  kAudioDevicePropertyDeviceNameCFString = kAudioObjectPropertyName,
  kAudioDevicePropertyDeviceManufacturer,
  kAudioDevicePropertyDeviceManufacturerCFString =
      kAudioObjectPropertyManufacturer,
  kAudioDevicePropertyRegisterBufferList,
  kAudioDevicePropertyBufferSize,
  kAudioDevicePropertyBufferSizeRange,
  kAudioDevicePropertyChannelName,
  kAudioDevicePropertyChannelNameCFString = kAudioObjectPropertyElementName,
  kAudioDevicePropertyChannelCategoryName,
  kAudioDevicePropertyChannelCategoryNameCFString =
      kAudioObjectPropertyElementCategoryName,
  kAudioDevicePropertyChannelNumberName,
  kAudioDevicePropertyChannelNumberNameCFString =
      kAudioObjectPropertyElementNumberName,
  kAudioDevicePropertySupportsMixing,
  kAudioDevicePropertyStreamFormat,
  kAudioDevicePropertyStreamFormats,
  kAudioDevicePropertyStreamFormatSupported,
  kAudioDevicePropertyStreamFormatMatch,
  kAudioDevicePropertyDataSourceNameForID,
  kAudioDevicePropertyClockSourceNameForID,
  kAudioDevicePropertyPlayThruDestinationNameForID,
  kAudioDevicePropertyChannelNominalLineLevelNameForID
};

enum {
  kAudioDevicePropertyPlugIn,
  kAudioDevicePropertyConfigurationApplication,
  kAudioDevicePropertyDeviceUID,
  kAudioDevicePropertyModelUID,
  kAudioDevicePropertyTransportType,
  kAudioDevicePropertyRelatedDevices,
  kAudioDevicePropertyClockDomain,
  kAudioDevicePropertyDeviceIsAlive,
  kAudioDevicePropertyDeviceHasChanged,
  kAudioDevicePropertyDeviceIsRunning,
  kAudioDevicePropertyDeviceIsRunningSomewhere,
  kAudioDevicePropertyDeviceCanBeDefaultDevice,
  kAudioDevicePropertyDeviceCanBeDefaultSystemDevice,
  kAudioDeviceProcessorOverload,
  kAudioDevicePropertyHogMode,
  kAudioDevicePropertyLatency,
  kAudioDevicePropertyBufferFrameSize,
  kAudioDevicePropertyBufferFrameSizeRange,
  kAudioDevicePropertyUsesVariableBufferFrameSizes,
  kAudioDevicePropertyStreams,
  kAudioDevicePropertySafetyOffset,
  kAudioDevicePropertyIOCycleUsage,
  kAudioDevicePropertyStreamConfiguration,
  kAudioDevicePropertyIOProcStreamUsage,
  kAudioDevicePropertyPreferredChannelsForStereo,
  kAudioDevicePropertyPreferredChannelLayout,
  kAudioDevicePropertyNominalSampleRate,
  kAudioDevicePropertyAvailableNominalSampleRates,
  kAudioDevicePropertyActualSampleRate
};

enum {
  kAudioFormatLinearPCM,
  kAudioFormatAC3,
  kAudioFormat60958AC3,
  kAudioFormatAppleIMA4,
  kAudioFormatMPEG4AAC,
  kAudioFormatMPEG4CELP,
  kAudioFormatMPEG4HVXC,
  kAudioFormatMPEG4TwinVQ,
  kAudioFormatMACE3,
  kAudioFormatMACE6,
  kAudioFormatULaw,
  kAudioFormatALaw,
  kAudioFormatQDesign,
  kAudioFormatQDesign2,
  kAudioFormatQUALCOMM,
  kAudioFormatMPEGLayer1,
  kAudioFormatMPEGLayer2,
  kAudioFormatMPEGLayer3,
  kAudioFormatDVAudio,
  kAudioFormatVariableDurationDVAudio,
  kAudioFormatTimeCode,
  kAudioFormatMIDIStream,
  kAudioFormatParameterValueStream,
  kAudioFormatAppleLossless
};

enum {
  kAudioFormatFlagIsFloat = (1L << 0),
  kAudioFormatFlagIsBigEndian = (1L << 1),
  kAudioFormatFlagIsSignedInteger = (1L << 2),
  kAudioFormatFlagIsPacked = (1L << 3),
  kAudioFormatFlagIsAlignedHigh = (1L << 4),
  kAudioFormatFlagIsNonInterleaved = (1L << 5),
  kAudioFormatFlagIsNonMixable = (1L << 6),

  kLinearPCMFormatFlagIsFloat = kAudioFormatFlagIsFloat,
  kLinearPCMFormatFlagIsBigEndian = kAudioFormatFlagIsBigEndian,
  kLinearPCMFormatFlagIsSignedInteger = kAudioFormatFlagIsSignedInteger,
  kLinearPCMFormatFlagIsPacked = kAudioFormatFlagIsPacked,
  kLinearPCMFormatFlagIsAlignedHigh = kAudioFormatFlagIsAlignedHigh,
  kLinearPCMFormatFlagIsNonInterleaved = kAudioFormatFlagIsNonInterleaved,
  kLinearPCMFormatFlagIsNonMixable = kAudioFormatFlagIsNonMixable,

  kAppleLosslessFormatFlag_16BitSourceData = 1,
  kAppleLosslessFormatFlag_20BitSourceData = 2,
  kAppleLosslessFormatFlag_24BitSourceData = 3,
  kAppleLosslessFormatFlag_32BitSourceData = 4
};

enum {
  kAudioFormatFlagsNativeEndian = kAudioFormatFlagIsBigEndian,
  kAudioFormatFlagsNativeFloatPacked =
      kAudioFormatFlagIsFloat | kAudioFormatFlagsNativeEndian |
      kAudioFormatFlagIsPacked
};

enum {
  kAudioDeviceUnknown
};

enum {
  kVariableLengthArray = 1
};

enum {
  kAudioHardwareNoError = 0,
  noErr = kAudioHardwareNoError
};

enum {
  false
};

typedef double Float64;
typedef float Float32;
typedef int SInt32;
typedef int Boolean;
typedef int OSErr;
typedef short SInt16;
typedef unsigned int UInt32;
typedef unsigned long int UInt64;

typedef SInt32 OSStatus;
typedef UInt32 AudioObjectID;
typedef UInt32 AudioHardwarePropertyID;
typedef UInt32 AudioDevicePropertyID;
typedef AudioObjectID AudioDeviceID;

struct AudioStreamBasicDescription {
  Float64 mSampleRate;
  UInt32 mFormatID;
  UInt32 mFormatFlags;
  UInt32 mBytesPerPacket;
  UInt32 mFramesPerPacket;
  UInt32 mBytesPerFrame;
  UInt32 mChannelsPerFrame;
  UInt32 mBitsPerChannel;
  UInt32 mReserved;
};
typedef struct AudioStreamBasicDescription AudioStreamBasicDescription;



struct SMPTETime {
  SInt16 mSubframes;
  SInt16 mSubframeDivisor;
  UInt32 mCounter;
  UInt32 mType;
  UInt32 mFlags;
  SInt16 mHours;
  SInt16 mMinutes;
  SInt16 mSeconds;
  SInt16 mFrames;
};
typedef struct SMPTETime SMPTETime;

struct AudioTimeStamp {
  Float64 mSampleTime;
  UInt64 mHostTime;
  Float64 mRateScalar;
  UInt64 mWordClockTime;
  SMPTETime mSMPTETime;
  UInt32 mFlags;
  UInt32 mReserved;
};
typedef struct AudioTimeStamp AudioTimeStamp;

struct AudioBuffer {
  UInt32 mNumberChannels;
  UInt32 mDataByteSize;
  void *mData;
};
typedef struct AudioBuffer AudioBuffer;

struct AudioBufferList {
  UInt32 mNumberBuffers;
  AudioBuffer mBuffers[kVariableLengthArray];
};
typedef struct AudioBufferList AudioBufferList;

typedef OSStatus(*AudioDeviceIOProc) (AudioDeviceID inDevice,
                                      const AudioTimeStamp * inNow,
                                      const AudioBufferList * inInputData,
                                      const AudioTimeStamp * inInputTime,
                                      AudioBufferList * outOutputData,
                                      const AudioTimeStamp * inOutputTime,
                                      void *inClientData);



OSStatus AudioHardwareGetProperty(AudioHardwarePropertyID inPropertyID,
                                  UInt32 * ioPropertyDataSize,
                                  void *outPropertyData);

OSStatus AudioHardwareGetPropertyInfo(AudioHardwarePropertyID inPropertyID,
                                  UInt32 * ioPropertyDataSize,
                                  void *outPropertyData);

OSStatus AudioDeviceSetProperty(AudioDeviceID inDevice,
                                const AudioTimeStamp * inWhen,
                                UInt32 inChannel, Boolean isInput,
                                AudioDevicePropertyID inPropertyID,
                                UInt32 inPropertyDataSize,
                                const void *inPropertyData);
OSStatus AudioDeviceGetProperty(AudioDeviceID inDevice, UInt32 inChannel,
                                Boolean isInput,
                                AudioDevicePropertyID inPropertyID,
                                UInt32 * ioPropertyDataSize,
                                void *outPropertyData);


OSStatus AudioDeviceAddIOProc(AudioDeviceID inDevice,
                              AudioDeviceIOProc inProc, void *inClientData);
OSStatus AudioDeviceStart(AudioDeviceID inDevice, AudioDeviceIOProc inProc);


OSStatus AudioDeviceStop(AudioDeviceID inDevice, AudioDeviceIOProc inProc);
#endif
