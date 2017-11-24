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
#include "util/attribute.hpp"

#include "affinities.hpp"

namespace Marshaller
{
  GList *Affinities::_titles     = NULL;
  GData *Affinities::_colors     = NULL;
  GData *Affinities::_validities = NULL;

  // --------------------------------------------------------------------------------
  Affinities::Affinities (Player *player)
    : Object ("Affinities")
  {
    _checksums = NULL;

    {
      GList *current = g_list_last (_titles);

      while (current)
      {
        SetAffinity (player, (const gchar *) current->data);

        current = g_list_previous (current);
      }
    }
  }

  // --------------------------------------------------------------------------------
  Affinities::~Affinities ()
  {
    g_list_free (_checksums);
  }

  // --------------------------------------------------------------------------------
  void Affinities::Destroy (Affinities *affinities)
  {
    affinities->Release ();
  }

  // --------------------------------------------------------------------------------
  void Affinities::Manage (const gchar *title,
                           const gchar *color)
  {
    if (_titles == NULL)
    {
      g_datalist_init (&_colors);
      g_datalist_init (&_validities);
    }

    _titles = g_list_prepend (_titles,
                              (void *) title);

    {
      GdkColor *gdk_color = g_new (GdkColor, 1);

      gdk_color_parse (color,
                       gdk_color);
      g_datalist_set_data (&_colors,
                           title,
                           gdk_color);
    }

    g_datalist_set_data (&_validities,
                         title,
                         (gpointer) TRUE);
  }

  // --------------------------------------------------------------------------------
  GList *Affinities::GetTitles ()
  {
    return _titles;
  }

  // --------------------------------------------------------------------------------
  GdkColor *Affinities::GetColor (const gchar *of)
  {
    return (GdkColor *) g_datalist_get_data (&_colors,
                                             of);
  }

  // --------------------------------------------------------------------------------
  GdkColor *Affinities::GetColor (guint of)
  {
    GList *current = _titles;

    for (guint i = 0; current != NULL; i++)
    {
      if (of & 1<<i)
      {
        return GetColor ((const gchar *) current->data);
      }
      current = g_list_next (current);
    }

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void Affinities::SetValidity (const gchar *of,
                                gboolean     validity)
  {
    g_datalist_set_data (&_validities,
                         of,
                         (gpointer) validity);
  }

  // --------------------------------------------------------------------------------
  guint Affinities::KinshipWith (Affinities *with)
  {
    GList *current_key  = _titles;
    GList *current      = _checksums;
    GList *with_current = with->_checksums;
    guint  kinship      = 0;

    for (guint i = 0; current != NULL; i++)
    {
      if (current->data == with_current->data)
      {
        if (g_datalist_get_data (&_validities,
                                 (const gchar *) current_key->data) != 0)
        {
          kinship |= 1<<i;
        }
      }

      current_key  = g_list_next (current_key);
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

    _checksums = g_list_prepend (_checksums,
                                 GUINT_TO_POINTER (digest));
  }
}
