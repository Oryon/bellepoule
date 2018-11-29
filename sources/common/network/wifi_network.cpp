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

#include "wifi_network.hpp"

namespace Net
{
  // --------------------------------------------------------------------------------
  WifiNetwork::WifiNetwork ()
    : Object ("WifiNetwork")
  {
    _SSID       = nullptr;
    _passphrase = nullptr;
    _encryption = nullptr;
  }

  // --------------------------------------------------------------------------------
  WifiNetwork::~WifiNetwork ()
  {
    g_free (_SSID);
    g_free (_passphrase);
  }

  // --------------------------------------------------------------------------------
  gboolean WifiNetwork::IsValid ()
  {
    return (_SSID && (_SSID[0] != 0));
  }

  // --------------------------------------------------------------------------------
  const gchar *WifiNetwork::GetSSID ()
  {
    return _SSID;
  }

  // --------------------------------------------------------------------------------
  const gchar *WifiNetwork::GetPassphrase ()
  {
    return _passphrase;
  }

  // --------------------------------------------------------------------------------
  const gchar *WifiNetwork::GetEncryption ()
  {
    return _encryption;
  }

  // --------------------------------------------------------------------------------
  void WifiNetwork::SetSSID (const gchar *SSID)
  {
    g_free (_SSID);
    _SSID = g_strdup (SSID);
  }

  // --------------------------------------------------------------------------------
  void WifiNetwork::SetPassphrase (const gchar *passphrase)
  {
    g_free (_passphrase);
    _passphrase = g_strdup (passphrase);
  }

  // --------------------------------------------------------------------------------
  void WifiNetwork::SetEncryption (const gchar *encryption)
  {
    g_free (_encryption);
    _encryption = g_strdup (encryption);
  }
}
