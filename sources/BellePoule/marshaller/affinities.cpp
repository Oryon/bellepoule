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
  GList *Affinities::_titles     = nullptr;
  GData *Affinities::_colors     = nullptr;
  GData *Affinities::_validities = nullptr;

  // --------------------------------------------------------------------------------
  Affinities::Affinities (Player *player)
    : Object ("Affinities")
  {
    _checksums    = nullptr;
    _shareholders = nullptr;

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
    g_list_free (_shareholders);
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
    if (_titles == nullptr)
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

    for (guint i = 0; current != nullptr; i++)
    {
      if (of & 1<<i)
      {
        return GetColor ((const gchar *) current->data);
      }
      current = g_list_next (current);
    }

    return nullptr;
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
    if (with && with->_shareholders)
    {
      return with->SimpleKinshipWith (this);
    }

    return SimpleKinshipWith (with);
  }

  // --------------------------------------------------------------------------------
  guint Affinities::SimpleKinshipWith (Affinities *with)
  {
    guint kinship = 0;

    if (with)
    {
      if (_shareholders)
      {
        GList *current = _shareholders;

        while (current)
        {
          Affinities *shareholder = (Affinities *) current->data;

          kinship |= shareholder->SimpleKinshipWith (with);

          current = g_list_next (current);
        }
      }
      else
      {
        GList *current_key  = _titles;
        GList *current      = _checksums;
        GList *with_current = with->_checksums;

        for (guint i = 0; current != nullptr; i++)
        {
          if (current->data && (current->data == with_current->data))
          {
            if (g_datalist_get_data (&_validities,
                                     (const gchar *) current_key->data) != nullptr)
            {
              kinship |= 1<<i;
            }
          }

          current_key  = g_list_next (current_key);
          current      = g_list_next (current);
          with_current = g_list_next (with_current);
        }
      }
    }

    return kinship;
  }

  // --------------------------------------------------------------------------------
  void Affinities::ShareWith (Affinities *with)
  {
    with->_shareholders = g_list_prepend (with->_shareholders,
                                          this);
  }

  // --------------------------------------------------------------------------------
  void Affinities::SetAffinity (Player      *player,
                                const gchar *affinity)
  {
    Player::AttributeId  attr_id (affinity);
    Attribute           *attr   = player->GetAttribute (&attr_id);
    guint64              digest = 0;

    if (attr && attr->GetStrValue ())
    {
      gchar *checksum = g_compute_checksum_for_string (G_CHECKSUM_SHA1,
                                                       attr->GetStrValue (),
                                                       -1);

      checksum[8] = 0;
      digest = g_ascii_strtoull (checksum,
                                 nullptr,
                                 16);
      g_free (checksum);
    }

    _checksums = g_list_prepend (_checksums,
                                 GUINT_TO_POINTER (digest));
  }
}
