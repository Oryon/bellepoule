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

#include <stdlib.h>
#include <string.h>
#include <goocanvas.h>

#include "util/canvas.hpp"
#include "util/player.hpp"
#include "util/fie_time.hpp"
#include "network/message.hpp"

#include "match.hpp"

// --------------------------------------------------------------------------------
Match::Match (Data *max_score)
  : Object ("Match")
{
  Init (max_score);
}

// --------------------------------------------------------------------------------
Match::Match  (Player *A,
               Player *B,
               Data   *max_score)
  : Object ("Match")
{
  Init (max_score);

  _opponents[0]._fencer   = A;
  _opponents[0]._is_known = TRUE;

  _opponents[1]._fencer   = B;
  _opponents[1]._is_known = TRUE;
}

// --------------------------------------------------------------------------------
Match::~Match ()
{
  for (guint i = 0; i < 2; i++)
  {
    Object::TryToRelease (_opponents[i]._score);
  }

  g_free (_name);
  g_free (_name_space);
}

// --------------------------------------------------------------------------------
void Match::Init (Data *max_score)
{
  _referee_list = NULL;
  _start_time   = NULL;
  _piste        = 0;

  _max_score = max_score;

  for (guint i = 0; i < 2; i++)
  {
    _opponents[i]._fencer   = NULL;
    _opponents[i]._is_known = FALSE;
    _opponents[i]._score    = new Score (max_score);
  }

  _name       = g_strdup ("");
  _name_space = g_strdup ("");

  _number = 0;
}

// --------------------------------------------------------------------------------
gboolean Match::IsDropped ()
{
  for (guint i = 0; i < 2; i++)
  {
    if (_opponents[i]._score->IsOut ())
    {
      return TRUE;
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void Match::SetOpponent (guint   position,
                         Player *fencer)
{
  _opponents[position]._fencer   = fencer;
  _opponents[position]._is_known = TRUE;

  if (fencer == NULL)
  {
    _opponents[position]._score->Set (0, FALSE);
    _opponents[!position]._score->Set (0, TRUE);
  }
}

// --------------------------------------------------------------------------------
void Match::RemoveOpponent (guint position)
{
  _opponents[position]._fencer   = NULL;
  _opponents[position]._is_known = FALSE;

  _opponents[position]._score->Clean ();
  _opponents[!position]._score->Clean ();
}

// --------------------------------------------------------------------------------
Player *Match::GetOpponent (guint position)
{
  return _opponents[position]._fencer;
}

// --------------------------------------------------------------------------------
void Match::DropFencer (Player *fencer,
                        gchar  *reason)
{
  for (guint i = 0; i < 2; i++)
  {
    if (_opponents[i]._fencer == fencer)
    {
      _opponents[i]._score->Drop (reason);
      _opponents[!i]._score->SynchronizeWith (_opponents[i]._score);
      return;
    }
  }
}

// --------------------------------------------------------------------------------
void Match::RestoreFencer (Player *fencer)
{
  for (guint i = 0; i < 2; i++)
  {
    if (_opponents[i]._fencer == fencer)
    {
      _opponents[i]._score->Restore ();
      _opponents[!i]._score->SynchronizeWith (_opponents[i]._score);
      return;
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Match::ExemptedMatch ()
{
  for (guint i = 0; i < 2; i++)
  {
    if (   (_opponents[i]._fencer == NULL)
        && _opponents[i]._is_known)
    {
      return TRUE;
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
Player *Match::GetWinner ()
{
  for (guint i = 0; i < 2; i++)
  {
    if (   (_opponents[i]._fencer == NULL)
        && _opponents[i]._is_known)
    {
      return _opponents[!i]._fencer;
    }
  }

  if (_opponents[0]._score->IsKnown () && _opponents[1]._score->IsKnown ())
  {
    if (IsDropped ())
    {
      for (guint i = 0; i < 2; i++)
      {
        if (   (_opponents[i]._score->IsOut ())
            && (_opponents[!i]._score->IsOut () == FALSE))
        {
          return _opponents[!i]._fencer;
        }
      }
      return NULL;
    }
    else if (HasError () == FALSE)
    {
      if (_opponents[0]._score->Get () > _opponents[1]._score->Get ())
      {
        return _opponents[0]._fencer;
      }
      else if (_opponents[0]._score->Get () < _opponents[1]._score->Get ())
      {
        return _opponents[1]._fencer;
      }

      for (guint i = 0; i < 2; i++)
      {
        if (_opponents[i]._score->IsTheBest ())
        {
          return _opponents[i]._fencer;
        }
      }
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
Player *Match::GetLooser ()
{
  Player *winner = GetWinner ();

  if (winner)
  {
    for (guint i = 0; i < 2; i++)
    {
      if (_opponents[i]._fencer != winner)
      {
        return _opponents[i]._fencer;
      }
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gboolean Match::HasFencer (Player *fencer)
{
  for (guint i = 0; i < 2; i++)
  {
    if (_opponents[i]._fencer == fencer)
    {
      return TRUE;
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Match::IsOver ()
{
  for (guint i = 0; i < 2; i++)
  {
    if (_opponents[i]._is_known == FALSE)
    {
      return FALSE;
    }
    if (_opponents[i]._score->IsKnown () == FALSE)
    {
      return FALSE;
    }
  }

  return (HasError () == FALSE);
}

// --------------------------------------------------------------------------------
gboolean Match::HasError ()
{
  for (guint i = 0; i < 2; i++)
  {
    if (_opponents[i]._score->IsValid () == FALSE)
    {
      return TRUE;
    }
  }

  return (_opponents[0]._score->IsConsistentWith (_opponents[1]._score) == FALSE);
}

// --------------------------------------------------------------------------------
void Match::SetScore (Player   *fencer,
                      gint      score,
                      gboolean  is_the_best)
{
  for (guint i = 0; i < 2; i++)
  {
    if (_opponents[i]._fencer == fencer)
    {
      _opponents[i]._score->Set (score, is_the_best);
      break;
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Match::ScoreIsNumber (gchar *score)
{
  guint len = strlen (score);

  if (len == 0)
  {
    return FALSE;
  }

  for (guint i = 0; i < len; i++)
  {
    if (g_ascii_isalpha (score[i]))
    {
      return FALSE;
    }
  }

  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Match::SetScore (Player *fencer,
                          gchar  *score)
{
  gboolean result = FALSE;

  if (score)
  {
    if (   (strlen (score) == 1)
        && (g_ascii_toupper (score[0]) == 'V'))
    {
      SetScore (fencer,
                _max_score->_value,
                TRUE);
      result = TRUE;
    }
    else
    {
      gchar    *score_value = score;
      gboolean  is_the_best = FALSE;

      if (   (strlen (score) > 1)
          && (g_ascii_toupper (score[0]) == 'W'))
      {
        score_value = &score[1];
        is_the_best = TRUE;
      }

      if (ScoreIsNumber (score_value))
      {
        gchar *max_str        = g_strdup_printf ("%d", _max_score->_value);
        gchar *one_digit_more = g_strdup_printf ("%s0", score_value);

        SetScore (fencer,
                  atoi (score_value),
                  is_the_best);
        if (strlen (score_value) >= strlen (max_str))
        {
          result = TRUE;
        }
        else if ((guint) atoi (one_digit_more) > _max_score->_value)
        {
          result = TRUE;
        }
        else
        {
          result = FALSE;
        }

        g_free (one_digit_more);
        g_free (max_str);
      }
    }
  }

  return result;
}

// --------------------------------------------------------------------------------
Score *Match::GetScore (Player *fencer)
{
  for (guint i = 0; i < 2; i++)
  {
    if (_opponents[i]._fencer == fencer)
    {
      return _opponents[i]._score;
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
Score *Match::GetScore (guint fencer)
{
  return _opponents[fencer]._score;
}

// --------------------------------------------------------------------------------
void Match::Save (xmlTextWriter *xml_writer)
{
  if (_number)
  {
    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST "Match");

    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "ID",
                                       "%d", _number);

    if (_parcel)
    {
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "NetID",
                                         "%x", _parcel->GetNetID ());
    }

    if (_piste)
    {
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "Piste",
                                         "%d", _piste);
    }

    if (_start_time)
    {
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "Date",
                                         "%s", _start_time->GetXmlImage ());
    }

    {
      GSList *current = _referee_list;

      while (current)
      {
        Player *referee = (Player *) current->data;

        xmlTextWriterStartElement (xml_writer,
                                   BAD_CAST referee->GetXmlTag ());
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "REF",
                                           "%d", referee->GetRef ());
        xmlTextWriterEndElement (xml_writer);

        current = g_slist_next (current);
      }
    }

    for (guint i = 0; i < 2; i++)
    {
      Save (xml_writer,
            _opponents[i]._fencer);
    }

    xmlTextWriterEndElement (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void Match::Save (xmlTextWriter *xml_writer,
                  Player        *fencer)
{
  if (fencer)
  {
    Score *score = GetScore (fencer);

    xmlTextWriterStartElement (xml_writer,
                               BAD_CAST fencer->GetXmlTag ());
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "REF",
                                       "%d", fencer->GetRef ());
    {
      const gchar *status_image = score->GetStatusImage ();

      if (status_image)
      {
        if ((status_image[0] == 'V') || (status_image[0] == 'D'))
        {
          xmlTextWriterWriteFormatAttribute (xml_writer,
                                             BAD_CAST "Score",
                                             "%d", score->Get ());
        }

        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "Statut",
                                     BAD_CAST status_image);
      }
    }
    xmlTextWriterEndElement (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void Match::Load (xmlNode *node_a,
                  Player  *fencer_a,
                  xmlNode *node_b,
                  Player  *fencer_b)
{
  _opponents[0]._fencer = fencer_a;
  Load (node_a,
        fencer_a);

  _opponents[1]._fencer = fencer_b;
  Load (node_b,
        fencer_b);

  for (guint i = 0; i < 2; i++)
  {
    if (_opponents[i]._score->IsOut ())
    {
      _opponents[!i]._score->SynchronizeWith (_opponents[i]._score);
    }
  }
}

// --------------------------------------------------------------------------------
void Match::Load (xmlNode *node,
                  Player  *fencer)
{
  gboolean  victory = FALSE;
  Score    *score   = GetScore (fencer);
  gchar    *attr;

  attr = (gchar *) xmlGetProp (node, BAD_CAST "Statut");
  if (attr)
  {
    if (attr[0] == 'V')
    {
      victory = TRUE;
    }
    else if (attr[0] == 'D')
    {
      victory = FALSE;
    }
    else
    {
      score->Drop (attr);
    }
    xmlFree (attr);
  }

  attr = (gchar *) xmlGetProp (node, BAD_CAST "Score");
  if (attr)
  {
    SetScore (fencer,
              atoi (attr),
              victory);

    xmlFree (attr);
  }
}

// --------------------------------------------------------------------------------
void Match::DiscloseWithIdChain (va_list chain_id)
{
  Disclose ("Job");

  while (gchar *key = va_arg (chain_id, gchar *))
  {
    guint value = va_arg (chain_id, guint);

    _parcel->Set (key, value);
  }
}

// --------------------------------------------------------------------------------
void Match::ChangeIdChain (guint batch_id,
                           guint netid)
{
  _parcel->Set ("batch", batch_id);
  _parcel->SetNetID (netid);
}

// --------------------------------------------------------------------------------
void Match::SetPiste (guint piste)
{
  _piste = piste;
}

// --------------------------------------------------------------------------------
void Match::SetStartTime (FieTime *time)
{
  Object::TryToRelease (_start_time);
  _start_time = time;
}

// --------------------------------------------------------------------------------
guint Match::GetPiste ()
{
  return _piste;
}

// --------------------------------------------------------------------------------
FieTime *Match::GetStartTime ()
{
  return _start_time;
}

// --------------------------------------------------------------------------------
guint Match::GetNetID ()
{
  return _parcel->GetNetID ();
}

// --------------------------------------------------------------------------------
void Match::CleanScore ()
{
  for (guint i = 0; i < 2; i++)
  {
    _opponents[i]._score->Clean ();
  }
}

// --------------------------------------------------------------------------------
void Match::SetNameSpace (const gchar *name_space)
{
  g_free (_name_space);
  _name_space = g_strdup (name_space);

  g_free (_name);
  _name = g_strdup_printf ("%s%d", _name_space, _number);
}

// --------------------------------------------------------------------------------
void Match::SetNumber (gint number)
{
  _number = number;

  g_free (_name);
  _name = g_strdup_printf ("%s%d", _name_space, _number);
}

// --------------------------------------------------------------------------------
const gchar *Match::GetName ()
{
  if (_number)
  {
    return _name;
  }
  else
  {
    return NULL;
  }
}

// --------------------------------------------------------------------------------
gint Match::GetNumber ()
{
  return _number;
}

// --------------------------------------------------------------------------------
GooCanvasItem *Match::GetScoreTable (GooCanvasItem *parent,
                                     gdouble        size)
{
  if (_max_score->_value <= 15)
  {
    gchar         *font = g_strdup_printf (BP_FONT "Bold %fpx", 1.5/2.0*(size));
    GooCanvasItem *score_table = goo_canvas_table_new (parent,
                                                       "column-spacing", size/10.0,
                                                       NULL);

    Canvas::NormalyzeDecimalNotation (font);

    for (guint i = 0; i < _max_score->_value; i++)
    {
      GooCanvasItem *text_item;
      GooCanvasItem *goo_rect;
      gchar         *number = g_strdup_printf ("%d", i+1);

      text_item = Canvas::PutTextInTable (score_table,
                                          number,
                                          0,
                                          i);
      g_free (number);
      Canvas::SetTableItemAttribute (text_item, "x-align", 0.5);
      Canvas::SetTableItemAttribute (text_item, "y-align", 0.5);

      g_object_set (G_OBJECT (text_item),
                    "font", font,
                    "anchor", GTK_ANCHOR_CENTER,
                    "alignment", PANGO_ALIGN_CENTER,
                    NULL);

      goo_rect = goo_canvas_rect_new (score_table,
                                      0.0,
                                      0.0,
                                      size,
                                      size,
                                      "line-width", size/10.0,
                                      NULL);
      Canvas::PutInTable (score_table,
                          goo_rect,
                          0,
                          i);
      Canvas::SetTableItemAttribute (goo_rect, "x-align", 0.5);
      Canvas::SetTableItemAttribute (goo_rect, "y-align", 0.5);
    }
    g_free (font);

    return score_table;
  }

  return NULL;
}

// --------------------------------------------------------------------------------
void Match::AddReferee (Player *referee)
{
  if (referee == NULL)
  {
    return;
  }

  if (   (_referee_list == NULL)
      || (g_slist_find (_referee_list,
                        referee) == NULL))
  {
    _referee_list = g_slist_prepend (_referee_list,
                                     referee);
  }
}

// --------------------------------------------------------------------------------
void Match::RemoveReferee (Player *referee)
{
  if (_referee_list)
  {
    _referee_list = g_slist_remove (_referee_list,
                                    referee);
  }
}

// --------------------------------------------------------------------------------
void Match::RemoveAllReferees ()
{
  if (_referee_list)
  {
    g_slist_free (_referee_list);
    _referee_list = NULL;
  }
}

// --------------------------------------------------------------------------------
GSList *Match::GetRefereeList ()
{
  return _referee_list;
}

// --------------------------------------------------------------------------------
void Match::FeedParcel (Net::Message *parcel)
{
  xmlBuffer *xml_buffer = xmlBufferCreate ();

  {
    xmlTextWriter *xml_writer = xmlNewTextWriterMemory (xml_buffer, 0);

    xmlTextWriterStartDocument (xml_writer,
                                NULL,
                                "UTF-8",
                                NULL);
    Save (xml_writer);
    xmlTextWriterEndDocument (xml_writer);
    xmlFreeTextWriter (xml_writer);
  }

  parcel->Set ("xml", (const gchar *) xml_buffer->content);
  xmlBufferFree (xml_buffer);

  parcel->Set ("workload_units",
               _max_score->_value);
}

// --------------------------------------------------------------------------------
gchar *Match::GetGuiltyParty ()
{
  return g_strdup_printf (gettext ("Match %s"), GetName ());
}

// --------------------------------------------------------------------------------
const gchar *Match::GetReason ()
{
  return gettext ("No winner!");
}
