#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>
#include <dbt.h>

#define WND_CLASS_NAME TEXT("BellePouleWindowClass")

// --------------------------------------------------------------------------------
static void ReportError (const char *title,
                         const char *message)
{
  fprintf (stderr,
           "{\n"
           "  \"Event\":   \"ERROR\",\n"
           "  \"Message\": \"%s: %s\"\n"
           "}\n"
           ".\n", title, message);
  fflush (stderr);
}

// --------------------------------------------------------------------------------
void ErrorHandler (LPTSTR lpszFunction)
{
  LPVOID lpMsgBuf;
  LPVOID lpDisplayBuf;
  DWORD  dw           = GetLastError ();

  FormatMessage (FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM |
                 FORMAT_MESSAGE_IGNORE_INSERTS,
                 nullptr,
                 dw,
                 MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                 (LPTSTR) &lpMsgBuf,
                 0, nullptr);

  // Display the error message and exit the process.
  lpDisplayBuf = (LPVOID)LocalAlloc (LMEM_ZEROINIT,
                                     (lstrlen ((LPCTSTR)lpMsgBuf)
                                      + lstrlen ((LPCTSTR)lpszFunction)+40)
                                     * sizeof (TCHAR));
  StringCchPrintf ((LPTSTR)lpDisplayBuf,
                   LocalSize (lpDisplayBuf) / sizeof (TCHAR),
                   TEXT ("%s failed with error %d: %s"),
                   lpszFunction, dw, lpMsgBuf);

  ReportError ("Internal error",
               (const char *) lpDisplayBuf);

  LocalFree (lpMsgBuf);
  LocalFree (lpDisplayBuf);
}

// --------------------------------------------------------------------------------
INT_PTR WINAPI OnWindowMessage (HWND   window,
                                UINT   message,
                                WPARAM wParam,
                                LPARAM lParam)
{
  LRESULT           result       = 1;
  static HDEVNOTIFY notification = nullptr;

  switch (message)
  {
    case WM_CREATE:
    {
      DEV_BROADCAST_DEVICEINTERFACE device;

      ZeroMemory (&device, sizeof (device));

      device.dbcc_size       = sizeof (device);
      device.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
      device.dbcc_classguid  = {0xa5dcbf10, 0x6530, 0x11d2, {0x90, 0x1f, 0x00, 0xc0, 0x4f, 0xb9, 0x51, 0xed}};
      notification           = RegisterDeviceNotification (window,
                                                           &device,
                                                           DEVICE_NOTIFY_WINDOW_HANDLE);
    }
    break;

    case WM_DEVICECHANGE:
    {
      DEV_BROADCAST_DEVICEINTERFACE_A *device = (DEV_BROADCAST_DEVICEINTERFACE_A *) lParam;

      if (device && (device->dbcc_devicetype == DBT_DEVTYP_DEVICEINTERFACE))
      {
        if (device->dbcc_name)
        {
          char *dbcc_name_cursor;
          char *factory_cursor;
          char *vendor           = nullptr;
          char *product          = nullptr;
          char *serial_number    = nullptr;
          char *token            = strtok_r (device->dbcc_name, "#", &dbcc_name_cursor);

          for (int i = 0; token != nullptr; i++)
          {
            if (i == 1)
            {
              token  = strtok_r (token, "&", &factory_cursor);
              vendor = token;

              token   = strtok_r (nullptr, "&", &factory_cursor);
              product = token;
            }
            else if (i == 2)
            {
              serial_number = token;

              if (wParam == DBT_DEVICEARRIVAL)
              {
                fprintf (stderr,
                         "{\n"
                         "  \"Event\":        \"PLUG\",\n"
                         "  \"Slot\":         \"%s\",\n"
                         "  \"Manufacturer\": \"%s\",\n"
                         "  \"Product\":      \"%s\",\n"
                         "  \"SerialNumber\": \"%s\"\n"
                         "}\n"
                         ".\n",
                         serial_number, vendor, product, serial_number);
                fflush  (stderr);
              }
              else if (wParam == DBT_DEVICEREMOVECOMPLETE)
              {
                fprintf (stderr,
                         "{\n"
                         "  \"Event\": \"UNPLUG\",\n"
                         "  \"Slot\":  \"%s\"\n"
                         "}\n"
                         ".\n",
                         serial_number);
                fflush  (stderr);
              }
              break;
            }

            token = strtok_r (nullptr, "#", &dbcc_name_cursor);
          }
        }
      }
    }
    break;

    case WM_CLOSE:
    {
      if (notification)
      {
        UnregisterDeviceNotification (notification);
        notification = nullptr;
      }

      DestroyWindow (window);
    }
    break;

    case WM_DESTROY:
    {
      PostQuitMessage (0);
    }
    break;

    default:
    {
      result = DefWindowProc (window,
                              message,
                              wParam, lParam);
    }
    break;
  }

  return result;
}

// --------------------------------------------------------------------------------
int __stdcall _tWinMain (HINSTANCE hInstanceExe,
                         HINSTANCE,
                         PTSTR     lpstrCmdLine,
                         int       nCmdShow)
{
  {
    WNDCLASSEX window_class;

    window_class.cbSize        = sizeof (WNDCLASSEX);
    window_class.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    window_class.hInstance     = reinterpret_cast<HINSTANCE> (GetModuleHandle (0));
    window_class.lpfnWndProc   = reinterpret_cast<WNDPROC> (OnWindowMessage);
    window_class.cbClsExtra    = 0;
    window_class.cbWndExtra    = 0;
    window_class.hIcon         = LoadIcon (0,IDI_APPLICATION);
    window_class.hbrBackground = (HBRUSH) (COLOR_WINDOW+1);
    window_class.hCursor       = LoadCursor (0, IDC_ARROW);
    window_class.lpszClassName = WND_CLASS_NAME;
    window_class.lpszMenuName  = nullptr;
    window_class.hIconSm       = window_class.hIcon;

    if (!RegisterClassEx (&window_class))
    {
      ErrorHandler (TEXT ((char *) "RegisterClassEx"));
      return -1;
    }
  }

  {
    HWND window = CreateWindowEx (WS_EX_CLIENTEDGE | WS_EX_APPWINDOW,
                                  WND_CLASS_NAME,
                                  (const char *) "BellePoule",
                                  WS_OVERLAPPEDWINDOW,
                                  CW_USEDEFAULT, 0,
                                  640, 480,
                                  nullptr, nullptr,
                                  nullptr,
                                  nullptr);

    if (window == nullptr)
    {
      ErrorHandler (TEXT ((char *) "CreateWindowEx: main appwindow window"));
      return -1;
    }

    ShowWindow   (window, SW_HIDE);
    UpdateWindow (window);
  }

  {
    MSG msg;
    int result;

    while ((result = GetMessage (&msg, nullptr, 0, 0)) != 0)
    {
      if (result == -1)
      {
        ErrorHandler (TEXT ((char *) "GetMessage"));
        break;
      }
      else
      {
        TranslateMessage (&msg);
        DispatchMessage  (&msg);
      }
    }
  }

  return 0;
}

#if 0
int main (int argc, char **argv)
{
  getc (stdin);

  return 0;
}
#endif
