/**************************************************************************************************
* voip_types.h
*
* Types definition, used by VoIP API
*
**************************************************************************************************/
#ifndef   _LIBVOIP_TYPES_H
#define   _LIBVOIP_TYPES_H

#ifdef __cplusplus
namespace voip  {

#ifdef  VOIP_EXPORT_DLL
  #define VOIP_EXPORT  _declspec(dllexport)
#elif   VOIP_IMPORT_DLL
  #define VOIP_EXPORT _declspec(dllimport)
#else
  #define VOIP_EXPORT
#endif

#ifndef NULL
  #define NULL 0
#endif

enum {
   kRelayGuidLen = 16
};

/////////////////////////////////////////////////////////////////////////////////////////
// error types
enum  eResult
{
  VR_OK                =  0,    // success
  VR_FAILED            = -1,    // unknown error
  VR_INVALID_PARAM     = -2,    // parameter check failed
  VR_NOT_SUPPORTED     = -3,    // functionality not supported on current platform
  VR_INVALID_STATE     = -4,    
  VR_DEVICE_ERROR      = -5,    // generic hardware i/o error
  VR_FILE_IO_ERROR     = -6     //
};

/////////////////////////////////////////////////////////////////////////////////////////
// PeerInfo -  platform-dependent parameters required for proper IO operation 
union  SystemObjects
{
  struct 
  {
    void* javaVM;
    void* context;
    bool  useOpenSLES;
  }Android;

  struct
  {
    int*  captureAngle;               // pointer must be valid during voip engine life
    void* IMouseEventsSource;         // ptr to store events sink
    const char *deviceManufacturer;   //  Microsoft.Phone.Info.DeviceStatus.DeviceManufacturer
    const char *deviceName;           //  Microsoft.Phone.Info.DeviceStatus.DeviceName
    const char *osVersion;            //  System.Environment.OSVersion.Version.ToString()
  }WinPhone;

  struct
  {
    void  *ICoreDispatcher;     // use "Windows::UI::Core::CoreWindow::GetForCurrentThread()->Dispatcher" from GUI thread to get this ref ptr;
  }WinRT;
};

union SystemVibroPattern
{
    struct
    {
        enum { kPatternSize = 16 };
       // class android.os.Vibrator (developer.android.com):
       //
       // Pass in an array of ints that are the durations for which to turn on or off
       // the vibrator in milliseconds.  The first value indicates the number of milliseconds
       // to wait before turning the vibrator on.  The next value indicates the number of milliseconds
       // for which to keep the vibrator on before turning it off.  Subsequent values alternate
       // between durations in milliseconds to turn the vibrator off or to turn the vibrator on.
        long long pattern[kPatternSize];
        bool loop;
    } Android;
};

struct SoundNotificationParams {
    SystemVibroPattern vibro;
    bool loudspeaker;
};

/////////////////////////////////////////////////////////////////////////////////////////
// StreamState -  audio/video incoming stream state
enum StreamState
{
  SS_Disabled       = 0,        // peer has disabled the stream (e.g. switched off camera) or it is not supported by local side
  SS_NotConnected   = 1,        // stream is enabled, waiting for connection to be established
  SS_UDP            = 2,        // receiving stream via UDP
  SS_TCP            = 3,        // receiving stream via TCP
  SS_Relay          = 4,        // receiving stream using intermediate relay server
};
/////////////////////////////////////////////////////////////////////////////////////////
// ProxyType - as name stands, the type of Proxy used
enum  ProxyType
{
  ProxyType_None  = 0,    // Proxy is not used
  ProxyType_HTTP  = 1,
  ProxyType_HTTPS = 2,
  ProxyType_SOCKS4= 3,
  ProxyType_SOCKS5= 4
};

enum ConnectionState {
  CS_NotConnected   = 1,        // stream is enabled, waiting for connection to be established
  CS_UDP            = 2,        // receiving stream via UDP
  CS_TCP            = 3,        // receiving stream via TCP
  CS_Relay          = 4,        // receiving stream using intermediate relay server
};


/////////////////////////////////////////////////////////////////////////////////////////
// List of compatible display orientations
enum OrientationMode{  
    omPortrait          = 0, // default
    omLandscapeRight    = 1,
    omLandscapeLeft     = 2
};

enum  AudioDeviceType {
    RecordingDevice = 0,
    PlaybackDevice  = 1,
    CommonDevice    = 2
};

enum {
    kMaxDeviceNameLen = 1024
};

enum ConnectionQuality {
    ConnectionQuality_Bad,
    ConnectionQuality_Audio_Ok,
    ConnectionQuality_Audio_Video_Ok
};

/////////////////////////////////////////////////////////////////////////////////////////
// Graphical point type
typedef void* hwnd_t; // platform dependent window handle
typedef void* hbmp_t; // platform dependent image handle

/////////////////////////////////////////////////////////////////////////////////////////
// Available buttons
enum ButtonType {
    ButtonType_Close = 0,
    //~~~~~~~
    ButtonType_Total
};
enum ButtonState {
    ButtonState_Normal = 0,
    ButtonState_Highlited,
    ButtonState_Pressed,
    ButtonState_Disabled,
    ButtonState_Invisible,
    //~~~~~~
    ButtonState_Total
};
enum ButtonPosition {
    ButtonPosition_TopRight = 0
};
struct ButtonContext {
    hbmp_t normal;
    hbmp_t highlighted;
    hbmp_t pressed;
    hbmp_t disabled;
};

/////////////////////////////////////////////////////////////////////////////////////////
// Status messages
struct ChannelStatusContext {
    hbmp_t allocating;       // outgoing call initialization
    hbmp_t calling;          // invite sent, waiting for accept/decline/timeout
    hbmp_t ringing;          // invite sent and recived, waiting for accept/decline/timeout
    hbmp_t inviting;         // incoming call is waiting for local accept
    hbmp_t confInviting;     // incoming conference call is waiting for local accept
    hbmp_t connecting;       // connecting
    hbmp_t connected;
    hbmp_t hold;
    hbmp_t oldVersion;
    hbmp_t videoPaused;
    hbmp_t closedByDecline;
    hbmp_t closedByTimeout;
    hbmp_t closedByError;
    hbmp_t ended;
};

#if __PLATFORM_WINPHONE || WINDOWS_PHONE
  // See http://developer.nokia.com/Community/Wiki/How_to_create_a_DirectX_texture_with_a_picture
  // to find out how to get pixel data from BitmapImage object
  struct WP8BitmapDesc  // must be passed to CreateBitmap()
  {
    unsigned int   *pixels;
    unsigned       width;
    unsigned       height;
  };
#endif

};  //namespace voip
#endif

#ifdef __OBJC__
  #if TARGET_OS_IPHONE || __PLATFORM_IOS
    #define WebrtcRenderView RenderViewIOS
    #import <UIKit/UIKit.h>
    @interface WebrtcRenderView : UIView {}
    @end
  #else
    #define WebrtcRenderView RenderViewOSX
    #import <Cocoa/Cocoa.h>
    @interface WebrtcRenderView : NSView {}
    @end
  #endif
#endif

#endif