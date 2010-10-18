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
#include "player.hpp"
#include "pool.hpp"

#include "swapper.hpp"

// --------------------------------------------------------------------------------
Swapper::ValueUsage::ValueUsage (gchar *image,
                                 guint  nb_pool)
{
  _image      = image;
  _nb_similar = 1;

  _in_pool = (guint *) g_malloc0 (nb_pool * sizeof (guint));
}

// --------------------------------------------------------------------------------
Swapper::ValueUsage::~ValueUsage ()
{
  g_free (_image);
  g_free (_in_pool);
}

// --------------------------------------------------------------------------------
Swapper::Swapper (Object *owner)
  : Object ("Swapper")
{
  _player_list = NULL;
  _criteria_id = NULL;
  _array       = NULL;
  _owner       = owner;
}

// --------------------------------------------------------------------------------
Swapper::~Swapper ()
{
  Object::TryToRelease (_criteria_id);
  FreeArray ();
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
void Swapper::SetCriteria (Player::AttributeId *criteria_id)
{
  Object::TryToRelease (_criteria_id);
  _criteria_id = criteria_id;
  _criteria_id->Retain ();
}

// --------------------------------------------------------------------------------
void Swapper::SetPlayerList (GSList *player_list)
{
  _player_list = player_list;
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

          value_usage = new ValueUsage (value_image,
                                        nb_pool);
          position = _array->len + 1;
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

        {
          guint pool_nb = (guint) player->GetData (_owner, "Pool No");

          value_usage->_in_pool[pool_nb-1]++;
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

      value_usage->_max_by_pool       = value_usage->_nb_similar / nb_pool;
      value_usage->_nb_asymetric_pool = value_usage->_nb_similar % nb_pool;
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Swapper::Swap (Player *playerA,
                        Pool   *to_pool)
{
  guint pool_numberA = (guint) playerA->GetData (_owner, "Pool No");

  if (to_pool->GetNumber () != pool_numberA)
  {
    Pool *pool_A   = (Pool *) g_slist_nth_data (_pools, pool_numberA-1);
    gint  position = pool_A->GetPosition (playerA);

    if (position > -1)
    {
      Player *playerB = to_pool->GetPlayer (position);

      if (playerB)
      {
        gboolean   result       = FALSE;
        Attribute *attrA        = playerA->GetAttribute (_criteria_id);
        gchar     *value_imageA = attrA->GetUserImage ();
        Attribute *attrB        = playerB->GetAttribute (_criteria_id);

        if (attrB)
        {
          gchar *value_imageB = attrB->GetUserImage ();

          if (strcmp (value_imageA, value_imageB) != 0)
          {
            guint       pool_numberB  = to_pool->GetNumber ();
            ValueUsage *value_usage_A = GetValueUsage (value_imageA);
            ValueUsage *value_usage_B = GetValueUsage (value_imageB);

            if (   CanSwap (value_usage_B, pool_numberA, pool_numberB)
                && CanSwap (value_usage_A, pool_numberB, pool_numberA))
            {
              pool_A->RemovePlayer  (playerA);
              value_usage_A->_in_pool[pool_numberA-1]--;

              to_pool->RemovePlayer (playerB);
              value_usage_B->_in_pool[pool_numberB-1]--;

              pool_A->AddPlayer  (playerB, _owner);
              value_usage_A->_in_pool[pool_numberB-1]++;

              to_pool->AddPlayer (playerA, _owner);
              value_usage_B->_in_pool[pool_numberA-1]++;

              result = TRUE;
            }
          }
          g_free (value_imageB);
        }

        g_free (value_imageA);

        return result;
      }
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Swapper::CanSwap (ValueUsage *value,
                           guint       to_pool,
                           guint       from_pool)
{
  if (value->_in_pool[to_pool-1] >= value->_max_by_pool)
  {
    guint nb_pool      = g_slist_length (_pools);
    guint nb_asymetric = 0;

    for (guint p = 0; p < nb_pool; p++)
    {
      if (p == from_pool-1)
      {
        if (value->_in_pool[p] > value->_max_by_pool+1)
        {
          nb_asymetric++;
        }
      }
      else if (p == to_pool-1)
      {
        if (value->_in_pool[p] > value->_max_by_pool)
        {
          return FALSE;
        }
        else if (value->_in_pool[p] == value->_max_by_pool-1)
        {
          nb_asymetric++;
        }
      }
      else if (value->_in_pool[p] > value->_max_by_pool)
      {
        nb_asymetric++;
      }

      if (nb_asymetric > value->_nb_asymetric_pool)
      {
        return FALSE;
      }
    }
  }

  return TRUE;
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
void Swapper::Swap (GSList  *pools,
                    guint32  rand_seed)
{
  GSList *players_to_swap = NULL;

  _pools = pools;
  Update ();

  // Which players?
  for (guint i = 0; i < _array->len; i++)
  {
    Swapper::ValueUsage *data;

    data = g_array_index (_array,
                          Swapper::ValueUsage *,
                          i);
    if (data->_nb_similar > 1)
    {
      GSList *current_pool = pools;

      while (current_pool)
      {
        Pool  *pool;
        guint  nb_similar_value;
        guint  nb_max;

        pool = (Pool *) current_pool->data;

        nb_similar_value = 0;
        nb_max           = data->_nb_similar / pool->GetNbPlayers ();
        if (data->_nb_similar % pool->GetNbPlayers ())
        {
          nb_max++;
        }

        for (guint p = 0; p < pool->GetNbPlayers (); p++)
        {
          Player    *player;
          Attribute *attr;

          player = pool->GetPlayer (p);
          attr   = player->GetAttribute (_criteria_id);
          if (attr)
          {
            gchar *user_image = attr->GetUserImage ();

            if (user_image && (strcmp (user_image, data->_image) == 0))
            {
              nb_similar_value++;
              if (nb_similar_value > nb_max)
              {
                players_to_swap = g_slist_prepend (players_to_swap,
                                                   player);
              }
            }
            g_free (user_image);
          }
        }

        current_pool = g_slist_next (current_pool);
      }
    }
  }

  {
    Player::AttributeId attr_id ("previous_stage_rank",
                                 _owner);

    attr_id.MakeRandomReady (rand_seed);
    players_to_swap = g_slist_sort_with_data (players_to_swap,
                                              (GCompareDataFunc) Player::Compare,
                                              &attr_id);

    for (guint i = 0; i < g_slist_length (players_to_swap); i++)
    {
      Player *player;

      player = (Player *) g_slist_nth_data (players_to_swap, i);
      {
        GSList *current_pool = pools;

        while (   current_pool
               && (Swap (player, (Pool *) current_pool->data) == FALSE))
        {
          current_pool = g_slist_next (current_pool);
        }
      }
    }
  }

  g_slist_free (players_to_swap);
}
