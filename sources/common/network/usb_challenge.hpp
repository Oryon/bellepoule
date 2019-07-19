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

#pragma once

#include "util/object.hpp"

namespace Net
{
  class Partner;

  class UsbChallenge : public Object
  {
    public:
      struct Listener
      {
        virtual void OnUsbPartnerTrusted  (Partner     *partner,
                                           const gchar *key) = 0;
        virtual void OnUsbPartnerDoubtful (Partner *partner) = 0;
      };

      static void Monitor (Partner     *partner,
                           const gchar *cypher,
                           const gchar *iv);

      static void CheckKey (const gchar *key,
                            const gchar *secret,
                            Listener    *listener);

    private:
      static GList *_challenges;

      Partner *_partner;
      gchar   *_cypher;
      gchar   *_iv;

      UsbChallenge (Partner     *partner,
                    const gchar *cypher,
                    const gchar *iv);

      ~UsbChallenge () override;
  };
}
