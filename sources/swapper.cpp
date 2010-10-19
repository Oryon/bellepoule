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

#include <string.h>
#include "pool.hpp"

#include "swapper.hpp"

// --------------------------------------------------------------------------------
Swapper::ValueUsage::ValueUsage (gchar *image)
{
  _image      = image;
  _nb_similar = 1;
}

// --------------------------------------------------------------------------------
Swapper::ValueUsage::~ValueUsage ()
{
  g_free (_image);
}

// --------------------------------------------------------------------------------
Swapper::Swapper (GSList *pools,
                  gchar  *criteria,
                  GSList *player_list)
: Object ("Swapper")
{
  _player_list = g_slist_copy (player_list);
  _array       = NULL;
  _pools       = pools;

  if (criteria)
  {
    _criteria_id = new Player::AttributeId (criteria);
  }
  else
  {
    _criteria_id = NULL;
  }

  Update ();
}

// --------------------------------------------------------------------------------
Swapper::~Swapper ()
{
  Object::TryToRelease (_criteria_id);
  FreeArray ();

  if (_player_list)
  {
    g_slist_free (_player_list);
  }
}

// --------------------------------------------------------------------------------
void Swapper::FreeArray ()
{
  if (_array)
  {
    for (guint i = 0; i < _array->len; i++)
    {
      Swapper::ValueUsage *value_usage;

      value_usage = g_array_index (_array,
                                   Swapper::ValueUsage *,
                                   i);
      value_usage->Release ();
    }
    g_array_free (_array, TRUE);
    _array = NULL;
  }
}

// --------------------------------------------------------------------------------
void Swapper::Update ()
{
  if (_criteria_id && _player_list)
  {
    GSList *current_player = _player_list;
    guint   nb_pool        = g_slist_length (_pools);

    RemoveAllData ();
    FreeArray     ();

    _array = g_array_new (FALSE,
                          FALSE,
                          sizeof (guint));

    while (current_player)
    {
      Player    *player;
      Attribute *value;

      player = (Player *) current_player->data;
      value  = player->GetAttribute (_criteria_id);

      if (value)
      {
        gchar      *value_image = value->GetUserImage ();
        ValueUsage *value_usage = GetValueUsage (value_image);

        // Number of players for a given value
        // _array   ==> array holding all existing value usage
        // position ==> array index where the value usage is located
        if (value_usage == NULL)
        {
          guint position;

          value_usage = new ValueUsage (value_image);
          position    = _array->len + 1;
          SetData (this,
                   value_image,
                   (void *) (position));

          _array = g_array_insert_val (_array,
                                       position-1,
                                       value_usage);
        }
        else
        {
          value_usage->_nb_similar++;
          g_free (value_image);
        }
      }

      current_player = g_slist_next (current_player);
    }

    // Max. value usage / pool
    for (guint i = 0; i < _array->len; i++)
    {
      Swapper::ValueUsage *value_usage;

      value_usage = g_array_index (_array,
                                   Swapper::ValueUsage *,
                                   i);

      value_usage->_max_by_pool        = value_usage->_nb_similar / nb_pool;
      value_usage->_nb_asymetric_pools = value_usage->_nb_similar % nb_pool;
    }
  }
}

// --------------------------------------------------------------------------------
Swapper::ValueUsage *Swapper::GetValueUsage (gchar *value_image)
{
  gint position = (guint) GetData (this, value_image);

  if (position && _array->len)
  {
    return g_array_index (_array,
                          ValueUsage *,
                          position-1);
  }
  else
  {
    return NULL;
  }
}

// --------------------------------------------------------------------------------
Player *Swapper::GetNextPlayer (Pool *for_pool)
{
  GSList *current = _player_list;
  GSList *result  = NULL;

  if (_criteria_id == NULL)
  {
    result = _player_list;
  }

  while (current && (result == NULL))
  {
    Attribute *attr;
    Player    *player;

    player = (Player *) current->data;
    attr = player->GetAttribute (_criteria_id);
    if (attr)
    {
      gchar *user_image = attr->GetUserImage ();

      if (user_image == NULL)
      {
        break;
      }
      else
      {
        ValueUsage *usage      = GetValueUsage (user_image);
        guint       nb_similar = 0;

        for (guint p = 0; p < for_pool->GetNbPlayers (); p++)
        {
          Player    *pool_player;
          Attribute *attr;

          pool_player = for_pool->GetPlayer (p);
          attr   = pool_player->GetAttribute (_criteria_id);
          if (attr)
          {
            gchar *image = attr->GetUserImage ();

            if (image && (strcmp (image, user_image) == 0))
            {
              nb_similar++;
            }
            g_free (image);
          }
        }

        if (nb_similar < usage->_max_by_pool)
        {
          result = current;
        }
        else if (   (nb_similar == usage->_max_by_pool)
                 && (usage->_nb_asymetric_pools > 0))
        {
          result = current;
          usage->_nb_asymetric_pools--;
        }
      }
      g_free (user_image);
    }
    current = g_slist_next (current);
  }
  if (result == NULL)
  {
    result = _player_list;
  }

  _player_list = g_slist_remove_link (_player_list,
                                      result);
  return (Player *) result->data;
}
