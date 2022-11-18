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
#include "util/data.hpp"
#include "util/xml_scheme.hpp"
#include "network/message.hpp"
#include "score.hpp"

#include "match.hpp"

GTimeSpan Match::_clock_offset = 0;

// --------------------------------------------------------------------------------
Match::Match (Data     *max_score,
              gboolean  overflow_allowed)
  : Object ("Match")
{
  _referee_list  = nullptr;
  _start_time    = nullptr;
  _duration_sec  = 0;
  _duration_span = 1;
  _piste         = 0;
  _dirty         = TRUE;
  _falsified     = FALSE;
  _overflow_allowed = overflow_allowed;

  _max_score = max_score;

  for (guint i = 0; i < 2; i++)
  {
    _opponents[i]._fencer   = nullptr;
    _opponents[i]._is_known = FALSE;
    _opponents[i]._score    = new Score (max_score,
                                         overflow_allowed);
  }

  _name       = g_strdup ("");
  _name_space = g_strdup ("");

  _number = 0;
}

// --------------------------------------------------------------------------------
Match::Match  (Player  *A,
               Player  *B,
               Data    *max_score,
              gboolean  overflow_allowed)
  : Match (max_score, overflow_allowed)
{
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
  if (_opponents[position]._fencer != fencer)
  {
    _dirty = TRUE;
  }

  _opponents[position]._fencer   = fencer;
  _opponents[position]._is_known = TRUE;

  if (fencer == nullptr)
  {
    _opponents[position]._score->Set (0, FALSE);
    _opponents[!position]._score->Set (0, TRUE);
  }
}

// --------------------------------------------------------------------------------
void Match::RemoveOpponent (guint position)
{
  _opponents[position]._fencer   = nullptr;
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
    if (   (_opponents[i]._fencer == nullptr)
        && _opponents[i]._is_known)
    {
      return TRUE;
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
gboolean Match::IsStarted ()
{
  for (guint i = 0; i < 2; i++)
  {
    if ((_opponents[i]._is_known) && _opponents[i]._score->IsKnown ())
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
    if (   (_opponents[i]._fencer == nullptr)
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
      return nullptr;
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

  return nullptr;
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

  return nullptr;
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
                          gchar  *value)
{
  gboolean result = FALSE;

  if (value)
  {
    if (   (strlen (value) == 1)
        && (g_ascii_toupper (value[0]) == 'V'))
    {
      SetScore (fencer,
                _max_score->GetValue (),
                TRUE);
      result = TRUE;
    }
    else
    {
      gchar    *score_value = value;
      gboolean  is_the_best = FALSE;

      if (   (strlen (value) > 1)
          && (g_ascii_toupper (value[0]) == 'W'))
      {
        score_value = &value[1];
        is_the_best = TRUE;
      }

      if (value[0] == 0)
      {
        Score *score = GetScore (fencer);

        score->Clean ();
      }

      if (ScoreIsNumber (score_value))
      {
        gchar *max_str        = g_strdup_printf ("%d", _max_score->GetValue ());
        gchar *one_digit_more = g_strdup_printf ("%s0", score_value);

        SetScore (fencer,
                  atoi (score_value),
                  is_the_best);
        if (strlen (score_value) >= strlen (max_str))
        {
          result = TRUE;
        }
        else if (!_overflow_allowed &&
                 ((guint) atoi (one_digit_more) > _max_score->GetValue ()))
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

  return nullptr;
}

// --------------------------------------------------------------------------------
Score *Match::GetScore (guint fencer)
{
  return _opponents[fencer]._score;
}

// --------------------------------------------------------------------------------
void Match::Save (XmlScheme *xml_scheme)
{
  if (_number)
  {
    xml_scheme->StartElement ("Match");

    xml_scheme->WriteFormatAttribute ("ID",
                                      "%d", _number);

    if (_parcel)
    {
      xml_scheme->WriteFormatAttribute ("NetID",
                                        "%x", _parcel->GetNetID ());
    }

    if (_piste)
    {
      xml_scheme->WriteFormatAttribute ("Piste",
                                        "%d", _piste);
    }

    if (_start_time)
    {
      xml_scheme->WriteFormatAttribute ("Date",
                                        "%s", _start_time->GetXmlDate ());
      xml_scheme->WriteFormatAttribute ("Heure",
                                        "%s", _start_time->GetXmlTime ());
    }

    if (_duration_sec > 0)
    {
      xml_scheme->WriteFormatAttribute ("Duree",
                                        "%d", _duration_sec);
    }

    if ((_duration_sec == 0) && (_duration_span > 1))
    {
      xml_scheme->WriteFormatAttribute ("Portee",
                                        "%d", _duration_span);
    }

    for (GSList *current = _referee_list; current; current = g_slist_next (current))
    {
      Player *referee = (Player *) current->data;

      xml_scheme->StartElement (referee->GetXmlTag ());
      xml_scheme->WriteFormatAttribute ("REF",
                                        "%d", referee->GetRef ());
      xml_scheme->EndElement ();
    }

    for (guint i = 0; i < 2; i++)
    {
      Save (xml_scheme,
            _opponents[i]._fencer);
    }

    xml_scheme->EndElement ();
  }
}

// --------------------------------------------------------------------------------
void Match::Save (XmlScheme *xml_scheme,
                  Player    *fencer)
{
  if (fencer)
  {
    Score *score = GetScore (fencer);

    xml_scheme->StartElement (fencer->GetXmlTag ());
    xml_scheme->WriteFormatAttribute ("REF",
                                      "%d", fencer->GetRef ());
    {
      const gchar *status_image = score->GetStatusImage ();

      if (status_image)
      {
        if ((status_image[0] == 'V') || (status_image[0] == 'D'))
        {
          xml_scheme->WriteFormatAttribute ("Score",
                                            "%d", score->Get ());
        }

        xml_scheme->WriteAttribute ("Statut",
                                    status_image);
      }
    }
    xml_scheme->EndElement ();
  }
}

// --------------------------------------------------------------------------------
void Match::CheckFalsification (xmlNode  *node,
                                Opponent *opponent)
{
  if (opponent && opponent->_is_known)
  {
    gchar *attr;

    attr = (gchar *) xmlGetProp (node, BAD_CAST "REF");
    if (attr)
    {
      if (opponent->_fencer->GetRef () != strtol (attr, nullptr, 10))
      {
        _falsified = TRUE;
      }

      xmlFree (attr);
    }
  }
}

// --------------------------------------------------------------------------------
void Match::Load (xmlNode *node_a,
                  Player  *fencer_a,
                  xmlNode *node_b,
                  Player  *fencer_b)
{
  {
    CheckFalsification (node_a,
                        &_opponents[0]);

    _opponents[0]._fencer = fencer_a;
    Load (node_a,
          fencer_a);
  }

  {
    CheckFalsification (node_b,
                        &_opponents[1]);

    _opponents[1]._fencer = fencer_b;
    Load (node_b,
          fencer_b);
  }

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

  for (guint i = 0; i < 2; i++)
  {
    if (_opponents[i]._score->IsOut ())
    {
      _opponents[!i]._score->SynchronizeWith (_opponents[i]._score);
    }
  }
}

// --------------------------------------------------------------------------------
void Match::DiscloseWithIdChain (va_list chain_id)
{
  Disclose ("BellePoule::Job");

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
  if (_piste != piste)
  {
    _piste = piste;
    _dirty = TRUE;
  }
}

// --------------------------------------------------------------------------------
void Match::SetStartTime (FieTime *time)
{
  Object::TryToRelease (_start_time);
  _start_time = time;
}

// --------------------------------------------------------------------------------
void Match::SetDuration (guint duration)
{
  _duration_sec = duration;
}

// --------------------------------------------------------------------------------
void Match::SetDurationSpan (guint span)
{
  _duration_span = span;
}

// --------------------------------------------------------------------------------
gboolean Match::AdjustRoadmap (Match *according_to)
{
  if (_opponents[0]._is_known && _opponents[1]._is_known)
  {
    if ((_start_time == nullptr) && (according_to->_duration_span > 1))
    {
      {
        GDateTime *start = g_date_time_add_full (according_to->_start_time->GetGDateTime (),
                                                 0,
                                                 0,
                                                 0,
                                                 0,
                                                 1,
                                                 according_to->_duration_sec);

        SetStartTime (new FieTime (start));
        g_date_time_unref (start);
      }

      _duration_span = according_to->_duration_span - 1;

      _piste = according_to->_piste;

      g_slist_free (_referee_list);
      _referee_list = g_slist_copy (according_to->_referee_list);

      return TRUE;
    }
  }

  return FALSE;
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
    return nullptr;
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
  if (_max_score->GetValue () <= 15)
  {
    gchar         *font = g_strdup_printf (BP_FONT "Bold %fpx", 1.5/2.0*(size));
    GooCanvasItem *score_table = goo_canvas_table_new (parent,
                                                       "column-spacing", size/10.0,
                                                       NULL);

    Canvas::NormalyzeDecimalNotation (font);

    for (guint i = 0; i < _max_score->GetValue (); i++)
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

  return nullptr;
}

// --------------------------------------------------------------------------------
void Match::AddReferee (Player *referee)
{
  if (referee == nullptr)
  {
    return;
  }

  if (   (_referee_list == nullptr)
      || (g_slist_find (_referee_list,
                        referee) == nullptr))
  {
    _referee_list = g_slist_prepend (_referee_list,
                                     referee);
    _dirty = TRUE;
  }
}

// --------------------------------------------------------------------------------
void Match::RemoveReferee (Player *referee)
{
  if (_referee_list)
  {
    _referee_list = g_slist_remove (_referee_list,
                                    referee);
    _dirty = TRUE;
  }
}

// --------------------------------------------------------------------------------
void Match::RemoveAllReferees ()
{
  if (_referee_list)
  {
    g_slist_free (_referee_list);
    _referee_list = nullptr;
    _dirty        = TRUE;
  }
}

// --------------------------------------------------------------------------------
GSList *Match::GetRefereeList ()
{
  return _referee_list;
}

// --------------------------------------------------------------------------------
Player *Match::GetFirstReferee ()
{
  if (_referee_list)
  {
    return (Player *) _referee_list->data;
  }

  return nullptr;
}

// --------------------------------------------------------------------------------
void Match::FeedParcel (Net::Message *parcel)
{
  xmlBuffer *xml_buffer = xmlBufferCreate ();

  {
    XmlScheme *xml_scheme = new XmlScheme (xml_buffer);

    Save (xml_scheme);
    xml_scheme->Release ();
  }

  parcel->Set ("xml", (const gchar *) xml_buffer->content);
  xmlBufferFree (xml_buffer);

  parcel->Set ("workload_units",
               _max_score->GetValue ());

  parcel->Set ("duration_sec",
               _duration_sec);

  parcel->Set ("duration_span",
               _duration_span);

  parcel->Set ("display_position",
               _number);
}

// --------------------------------------------------------------------------------
Error *Match::SpawnError ()
{
  gchar *guilty_party = g_strdup_printf (gettext ("Match %s"), GetName ());
  Error *error;

  error = new Error (Error::Level::MAJOR,
                     guilty_party,
                     gettext ("No winner!"));

  g_free (guilty_party);

  return error;
}

// --------------------------------------------------------------------------------
gboolean Match::IsDirty ()
{
  return _dirty;
}

// --------------------------------------------------------------------------------
gboolean Match::IsFalsified ()
{
  return _falsified;
}

// --------------------------------------------------------------------------------
void Match::Spread ()
{
  Object::Spread ();

  _dirty = FALSE;
}

// --------------------------------------------------------------------------------
void Match::Recall ()
{
  Object::Recall ();

  _dirty = TRUE;
}

// --------------------------------------------------------------------------------
void Match::Timestamp ()
{
  if (_start_time && IsOver ())
  {
    _duration_sec = GetDuration (_start_time->GetGDateTime ());
    Spread ();
  }
}

// --------------------------------------------------------------------------------
void Match::OnClockOffset (Net::Message *message)
{
  _clock_offset = message->GetSignedInteger ("offset");
}

// --------------------------------------------------------------------------------
guint Match::GetDuration (GDateTime *start_time)
{
  guint      duration;
  GDateTime *now_rounded;

  {
    GDateTime *now = g_date_time_new_now_local ();

    now_rounded = g_date_time_add_full (now,
                                        0,
                                        0,
                                        0,
                                        0,
                                        0,
                                        -g_date_time_get_second (now) + _clock_offset);
    g_date_time_unref (now);
  }

  {
    GTimeSpan span = g_date_time_difference (now_rounded,
                                             start_time);

    if (span < 0)
    {
      span = G_TIME_SPAN_SECOND;
    }
    duration = (guint) (span / G_TIME_SPAN_SECOND);

    g_date_time_unref (now_rounded);
  }

  return duration;
}

// --------------------------------------------------------------------------------
gint Match::Compare (Match *A,
                     Match *B)
{
  gint result;

  result = A->GetPiste () - B->GetPiste ();
  if (result != 0)
  {
    return result;
  }

  result = FieTime::Compare (A->_start_time, B->_start_time);
  if (result)
  {
    return result;
  }

  return A->_number - B->_number;
}
