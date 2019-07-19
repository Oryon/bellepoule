#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#pragma GCC diagnostic ignored "-Wpedantic"
#include <libusb.h>
#pragma GCC diagnostic pop

static bool                           _stopped         = true;
static pthread_t                      _thread;
static libusb_hotplug_callback_handle _callback_handle;

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
static unsigned char *GetField (libusb_device_handle *handle,
                                int                   field)
{
  const int      length = 1024;
  unsigned char *buffer = (unsigned char *) malloc (length);

  memset (buffer,
          '\0',
          length);

  libusb_get_string_descriptor_ascii (handle,
                                      field,
                                      buffer,
                                      length);

  return buffer;
}

// --------------------------------------------------------------------------------
static bool DeviceHasClass (libusb_device *device,
                            int            klass)
{
  int                      rc;
  libusb_device_descriptor desc;

  rc = libusb_get_device_descriptor (device,
                                     &desc);
  if (rc < 0)
  {
    ReportError ("libusb_get_device_descriptor:",
                 libusb_error_name (rc));
  }
  else if (desc.bDeviceClass == LIBUSB_CLASS_PER_INTERFACE)
  {
    struct libusb_config_descriptor *config;

    libusb_get_active_config_descriptor (device,
                                         &config);

    for (int i = 0 ; i < config->bNumInterfaces ; i++)
    {
      for (int k = 0 ; k < config->interface[i].num_altsetting ; k++)
      {
        const struct libusb_interface_descriptor *inter = &config->interface[i].altsetting[k];

        if (inter->bInterfaceClass == LIBUSB_CLASS_MASS_STORAGE)
        {
          return true;
        }
      }
    }
  }
  else if (desc.bDeviceClass == LIBUSB_CLASS_MASS_STORAGE)
  {
    return true;
  }

  return false;
}

// --------------------------------------------------------------------------------
static int OnHotplug (struct libusb_context *ctx,
                      struct libusb_device  *device,
                      libusb_hotplug_event   event,
                      void                  *user_data)
{
  struct libusb_device_descriptor desc;

  libusb_get_device_descriptor (device,
                                &desc);

  if (event == LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT)
  {
    fprintf (stderr,
             "{\n"
             "  \"Event\": \"UNPLUG\",\n"
             "  \"Slot\":  \"%04x:%04x\"\n"
             "}\n"
             ".\n",
             libusb_get_bus_number (device), libusb_get_port_number (device));
    fflush (stderr);
  }
  else if (DeviceHasClass (device,
                           LIBUSB_CLASS_MASS_STORAGE))
  {
    libusb_device_handle *handle = nullptr;
    int                   rc;

    rc = libusb_open (device, &handle);
    if (rc != LIBUSB_SUCCESS)
    {
      ReportError ("libusb_open",
                   libusb_error_name (rc));
    }
    else
    {
      unsigned char *iProduct      = GetField (handle, desc.iProduct);
      unsigned char *iManufacturer = GetField (handle, desc.iManufacturer);
      unsigned char *iSerialNumber = GetField (handle, desc.iSerialNumber);

      fprintf (stderr,
               "{\n"
               "  \"Event\":        \"PLUG\",\n"
               "  \"Slot\":         \"%04x:%04x\",\n"
               "  \"Product\":      \"%s\",\n"
               "  \"Manufacturer\": \"%s\",\n"
               "  \"SerialNumber\": \"%s\"\n"
               "}\n"
               ".\n",
               libusb_get_bus_number (device), libusb_get_port_number (device),
               iProduct, iManufacturer, iSerialNumber);
      fflush  (stderr);

      free (iProduct);
      free (iManufacturer);
      free (iSerialNumber);
      libusb_close (handle);
    }
  }

  return 0;
}

// --------------------------------------------------------------------------------
static void *Looper (void *p_data)
{
  while (_stopped == false)
  {
    libusb_handle_events (nullptr);
  }

  return nullptr;
}

// --------------------------------------------------------------------------------
static void Stop ()
{
  if (_stopped == false)
  {
    _stopped = true;

    libusb_hotplug_deregister_callback (nullptr,
                                        _callback_handle);

    pthread_join (_thread,
                  nullptr);
  }
}

// --------------------------------------------------------------------------------
int main (int argc, char **argv)
{
  int rc;

  libusb_init (nullptr);

  rc = libusb_hotplug_register_callback (nullptr,
                                         (libusb_hotplug_event) (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT),
                                         LIBUSB_HOTPLUG_NO_FLAGS,
                                         LIBUSB_HOTPLUG_MATCH_ANY,
                                         LIBUSB_HOTPLUG_MATCH_ANY,
                                         LIBUSB_HOTPLUG_MATCH_ANY,
                                         (libusb_hotplug_callback_fn) OnHotplug,
                                         nullptr,
                                         &_callback_handle);
  if (rc != LIBUSB_SUCCESS)
  {
    ReportError ("libusb_hotplug_register_callback",
                 libusb_error_name (rc));
  }
  else
  {
    _stopped = false;
    rc = pthread_create (&_thread,
                         nullptr,
                         Looper,
                         nullptr);
    if (rc)
    {
      _stopped = true;
      ReportError ("pthread_create",
                   strerror (rc));
    }

    getc (stdin);
  }

  libusb_exit (nullptr);
  return 0;
}
