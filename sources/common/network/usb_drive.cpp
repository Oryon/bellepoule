// Copyright (C) 2009 Yannick Le Roux.
// This file is part of BellePoule.
//
//   BellePoule is free software: you can redistribute it and/or modify
//   it under the terms of the GNU General Public License as published by
//   the Free Software Foundation, either version 3 of the License, or
//   (at your option) any later version.
//
//   BellePoule is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License for more details.
//
//   You should have received a copy of the GNU General Public License
//   along with BellePoule.  If not, see <http://www.gnu.org/licenses/>.

#include "util/global.hpp"

#include "usb_drive.hpp"

namespace Net
{
  // ----------------------------------------------------------------------------------------------
  UsbDrive::UsbDrive (gchar *slot,
                      gchar *product,
                      gchar *manufacturer,
                      gchar *serial_number)
    : Object ("UsbDrive")
  {
     _slot         = slot;
     _product      = product;
     _manufacturer = manufacturer;

    {
      _serial_number = g_new0 (char, 256+1);

      g_strlcpy (_serial_number,
                 serial_number,
                 256+1);

      g_free (serial_number);
    }
  }

  // ----------------------------------------------------------------------------------------------
  UsbDrive::~UsbDrive ()
  {
    g_free (_slot);
    g_free (_product);
    g_free (_manufacturer);
    g_free (_serial_number);
  }

  // ----------------------------------------------------------------------------------------------
  const gchar *UsbDrive::GetSlot ()
  {
    return _slot;
  }

  // ----------------------------------------------------------------------------------------------
  const gchar *UsbDrive::GetProduct ()
  {
    return _product;
  }

  // ----------------------------------------------------------------------------------------------
  const gchar *UsbDrive::GetManufacturer ()
  {
    return _manufacturer;
  }

  // ----------------------------------------------------------------------------------------------
  gchar *UsbDrive::GetSerialNumber ()
  {
    return _serial_number;
  }
}
