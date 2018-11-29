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
#include "util/glade.hpp"
#include "util/wifi_code.hpp"
#include "network/wifi_network.hpp"
#include "network/greg_uploader.hpp"

#include "smartphones.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  SmartPhones::SmartPhones (Glade *glade)
    : Object ("SmartPhones")
  {
    _glade = glade;

    g_signal_connect (_glade->GetWidget ("SSID_entry"),
                      "changed",
                      G_CALLBACK (RefreshScannerCode), this);
    g_signal_connect (_glade->GetWidget ("security_combobox"),
                      "changed",
                      G_CALLBACK (RefreshScannerCode), this);
    g_signal_connect (_glade->GetWidget ("passphrase_entry"),
                      "changed",
                      G_CALLBACK (RefreshScannerCode), this);

    // WIFI credentials
    {
      _wifi_network    = new Net::WifiNetwork ();
      _admin_wifi_code = new WifiCode ("Administrator");

      gtk_entry_set_visibility (GTK_ENTRY (_glade->GetWidget ("passphrase_entry")),
                                FALSE);

      RefreshScannerCode (nullptr, this);
    }
  }

  // --------------------------------------------------------------------------------
  SmartPhones::~SmartPhones ()
  {
    _admin_wifi_code->Release ();
    _wifi_network->Release ();
  }

  // --------------------------------------------------------------------------------
  void SmartPhones::RefreshScannerCode (GtkEditable *widget,
                                        SmartPhones *smartphones)
  {
    GtkEntry *ssid_w       = GTK_ENTRY (smartphones->_glade->GetWidget ("SSID_entry"));
    GtkEntry *passphrase_w = GTK_ENTRY (smartphones->_glade->GetWidget ("passphrase_entry"));

    smartphones->_wifi_network->SetSSID       ((gchar *) gtk_entry_get_text (ssid_w));
    smartphones->_wifi_network->SetPassphrase ((gchar *) gtk_entry_get_text (passphrase_w));
    smartphones->_wifi_network->SetEncryption ("WPA");

    smartphones->_admin_wifi_code->SetWifiNetwork (smartphones->_wifi_network);

    {
      GdkPixbuf *pixbuf = smartphones->_admin_wifi_code->GetPixbuf ();

      gtk_image_set_from_pixbuf (GTK_IMAGE (smartphones->_glade->GetWidget ("scanner_code_image")),
                                 pixbuf);
      g_object_ref (pixbuf);
    }
  }

  // --------------------------------------------------------------------------------
  WifiCode *SmartPhones::GetAdminCode ()
  {
    return _admin_wifi_code;
  }
}
