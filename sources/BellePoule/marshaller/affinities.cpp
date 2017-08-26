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

#include "util/player.hpp"

#include "affinities.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  Affinities::Affinities (Player *player)
    : Object ("Affinities")
  {
    _affinity_names = NULL;
    _checksums    = NULL;

    SetAffinity (player, "country");
    SetAffinity (player, "league");
    SetAffinity (player, "club");
  }

  // --------------------------------------------------------------------------------
  Affinities::~Affinities ()
  {
    g_list_free (_affinity_names);
    g_list_free (_checksums);
  }

  // --------------------------------------------------------------------------------
  void Affinities::Destroy (Affinities *affinities)
  {
    affinities->Release ();
  }

  // --------------------------------------------------------------------------------
  GList *Affinities::GetAffinityNames ()
  {
    return _affinity_names;
  }

  // --------------------------------------------------------------------------------
  guint Affinities::KinshipWith (Affinities *with)
  {
    GList *current      = _checksums;
    GList *with_current = with->_checksums;
    guint  kinship      = 0;

    for (guint i = 0; current != NULL; i++)
    {
      if (current->data == with_current->data)
      {
        kinship |= 1<<i;
      }

      current      = g_list_next (current);
      with_current = g_list_next (with_current);
    }

    return kinship;
  }

  // --------------------------------------------------------------------------------
  void Affinities::SetAffinity (Player      *player,
                                const gchar *affinity)
  {
    Player::AttributeId  attr_id (affinity);
    Attribute           *attr   = player->GetAttribute (&attr_id);
    guint64              digest = 0;

    if (attr)
    {
      gchar *checksum = g_compute_checksum_for_string (G_CHECKSUM_SHA1,
                                                       attr->GetStrValue (),
                                                       -1);

      checksum[8] = 0;
      digest = g_ascii_strtoull (checksum,
                                 NULL,
                                 16);
      g_free (checksum);
    }

    _affinity_names = g_list_prepend (_affinity_names,
                                      (void *) affinity);
    _checksums = g_list_prepend (_checksums,
                                 GUINT_TO_POINTER (digest));
  }
}
