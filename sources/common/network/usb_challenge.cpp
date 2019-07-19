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

#include "cryptor.hpp"
#include "partner.hpp"
#include "usb_challenge.hpp"

namespace Net
{
  GList *UsbChallenge::_challenges = nullptr;

  // ----------------------------------------------------------------------------------------------
  UsbChallenge::UsbChallenge (Partner     *partner,
                              const gchar *cypher,
                              const gchar *iv)
    : Object ("UsbChallenge")
  {
    _partner = partner;
    _partner->Retain ();

    _cypher = g_strdup (cypher);
    _iv     = g_strdup (iv);
  }

  // ----------------------------------------------------------------------------------------------
  UsbChallenge::~UsbChallenge ()
  {
    _partner->Release ();

    g_free (_cypher);
    g_free (_iv);
  }

  // ----------------------------------------------------------------------------------------------
  void UsbChallenge::Monitor (Partner     *partner,
                              const gchar *cypher,
                              const gchar *iv)
  {
    UsbChallenge *challenge = new UsbChallenge (partner,
                                                cypher,
                                                iv);

    _challenges = g_list_prepend (_challenges,
                                  challenge);
  }

  // ----------------------------------------------------------------------------------------------
  void UsbChallenge::CheckKey (const gchar *key,
                               const gchar *secret,
                               Listener    *listener)
  {
    GList *trusted = nullptr;

    for (GList *current = _challenges; current; current = g_list_next (current))
    {
      UsbChallenge *challenge = (UsbChallenge *) current->data;
      Cryptor      *cryptor   = new Cryptor ();
      gchar        *message;

      message = cryptor->Decrypt (challenge->_cypher,
                                  challenge->_iv,
                                  key);

      if (g_ascii_strcasecmp (message, secret) == 0)
      {
        trusted = g_list_prepend (trusted,
                                  challenge);

        listener->OnUsbPartnerTrusted (challenge->_partner,
                                       key);
      }

      g_free (message);
      cryptor->Release ();
    }

    for (GList *current = trusted; current; current = g_list_next (current))
    {
      UsbChallenge *challenge = (UsbChallenge *) current->data;

      _challenges = g_list_remove (_challenges,
                                   challenge);
      challenge->Release ();
    }
    g_list_free (trusted);
  }
}
