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

#include "canvas.hpp"
#include "player.hpp"
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

  _A = A;
  _B = B;

  _A_is_known = TRUE;
  _B_is_known = TRUE;
}

// --------------------------------------------------------------------------------
Match::~Match ()
{
  Object::TryToRelease (_A_score);
  Object::TryToRelease (_B_score);

  g_free (_name);
  g_free (_name_space);
}

// --------------------------------------------------------------------------------
void Match::Init (Data *max_score)
{
  _referee_list = NULL;

  _max_score = max_score;

  _A = NULL;
  _B = NULL;

  _A_is_known = FALSE;
  _B_is_known = FALSE;

  _A_is_dropped = FALSE;
  _B_is_dropped = FALSE;

  _name       = g_strdup ("");
  _name_space = g_strdup ("");

  _number = 0;

  _A_score = new Score (max_score);
  _B_score = new Score (max_score);
}

// --------------------------------------------------------------------------------
gboolean Match::IsDropped ()
{
  return (_A_is_dropped || _B_is_dropped);
}

// --------------------------------------------------------------------------------
gboolean Match::IsFake ()
{
  return (GetWinner () && (GetLooser () == NULL));
}

// --------------------------------------------------------------------------------
void Match::SetPlayerA (Player *fencer)
{
  _A          = fencer;
  _A_is_known = TRUE;
}

// --------------------------------------------------------------------------------
void Match::SetPlayerB (Player *fencer)
{
  _B          = fencer;
  _B_is_known = TRUE;
}

// --------------------------------------------------------------------------------
Player *Match::GetPlayerA ()
{
  return _A;
}

// --------------------------------------------------------------------------------
Player *Match::GetPlayerB ()
{
  return _B;
}

// --------------------------------------------------------------------------------
void Match::DropPlayer (Player *fencer)
{
  if (_A == fencer)
  {
    _A_is_dropped = TRUE;
  }
  else
  {
    _B_is_dropped = TRUE;
  }

  if (IsDropped ())
  {
    _A_score->Drop ();
    _B_score->Drop ();
  }
}

// --------------------------------------------------------------------------------
void Match::RestorePlayer (Player *fencer)
{
  if (_A == fencer)
  {
    _A_is_dropped = FALSE;
  }
  else if (_B == fencer)
  {
    _B_is_dropped = FALSE;
  }

  if (IsDropped () == FALSE)
  {
    _A_score->Restore ();
    _B_score->Restore ();
  }
}

// --------------------------------------------------------------------------------
Player *Match::GetWinner ()
{
  if (   (_B == NULL)
      && _B_is_known)
  {
    return _A;
  }
  else if (   (_A == NULL)
           && _A_is_known)
  {
    return _B;
  }
  else if (PlayerHasScore (_A) && PlayerHasScore (_B))
  {
    Score *score_A = GetScore (_A);
    Score *score_B = GetScore (_B);

    if (IsDropped ())
    {
      if (_A_is_dropped)
      {
        return _B;
      }
      else
      {
        return _A;
      }
    }
    else if (   score_A->IsValid ()
             && score_B->IsValid ()
             && score_A->IsConsistentWith (score_B))
    {
      if (_A_score->Get () > _B_score->Get ())
      {
        return _A;
      }
      else if (_A_score->Get () < _B_score->Get ())
      {
        return _B;
      }
      else if (_A_score->IsTheBest ())
      {
        return _A;
      }
      else
      {
        return _B;
      }
    }
  }

  return NULL;
}

// --------------------------------------------------------------------------------
Player *Match::GetLooser ()
{
  Player *winner = GetWinner ();

  if (winner && winner == _A)
  {
    return _B;
  }
  else if (winner && winner == _B)
  {
    return _A;
  }

  return NULL;
}

// --------------------------------------------------------------------------------
gboolean Match::HasPlayer (Player *fencer)
{
  return ((_A == fencer) || (_B == fencer));
}

// --------------------------------------------------------------------------------
gboolean Match::PlayerHasScore (Player *fencer)
{
  if (_A && (fencer == _A))
  {
    return (_A_score->IsKnown ());
  }
  else if (_B && (fencer == _B))
  {
    return (_B_score->IsKnown ());
  }
  else
  {
    return FALSE;
  }
}

// --------------------------------------------------------------------------------
void Match::SetScore (Player   *fencer,
                      gint      score,
                      gboolean  is_the_best)
{
  if (_A == fencer)
  {
    _A_score->Set (score, is_the_best);
  }
  else if (_B == fencer)
  {
    _B_score->Set (score, is_the_best);
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
  if (_A == fencer)
  {
    return _A_score;
  }
  else if (_B == fencer)
  {
    return _B_score;
  }

  return NULL;
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

    {
      GSList *current = _referee_list;

      while (current)
      {
        Player *referee = (Player *) current->data;

        xmlTextWriterStartElement (xml_writer,
                                   BAD_CAST "Arbitre");
        xmlTextWriterWriteFormatAttribute (xml_writer,
                                           BAD_CAST "REF",
                                           "%d", referee->GetRef ());
        xmlTextWriterEndElement (xml_writer);

        current = g_slist_next (current);
      }
    }

    Save (xml_writer,
          _A);
    Save (xml_writer,
          _B);

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
                               BAD_CAST "Tireur");
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "REF",
                                       "%d", fencer->GetRef ());

    if (score->IsKnown ())
    {
      xmlTextWriterWriteFormatAttribute (xml_writer,
                                         BAD_CAST "Score",
                                         "%d", score->Get ());
      if (GetWinner () == fencer)
      {
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "Statut",
                                     BAD_CAST "V");
      }
      else
      {
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "Statut",
                                     BAD_CAST "D");
      }
    }
    xmlTextWriterEndElement (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void Match::CleanScore ()
{
  _A_is_dropped = FALSE;
  _B_is_dropped = FALSE;
  _A_score->Clean ();
  _B_score->Clean ();
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
gchar *Match::GetName ()
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
  gchar         *font = g_strdup_printf ("Sans Bold %fpx", 1.5/2.0*(size));
  gchar         *decimal = strchr (font, ',');
  GooCanvasItem *score_table = goo_canvas_table_new (parent,
                                                     "column-spacing", size/10.0,
                                                     NULL);

  if (decimal)
  {
    *decimal = '.';
  }

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
  if (_referee_list == NULL)
  {
    _referee_list = g_slist_remove (_referee_list,
                                    referee);
  }
}

// --------------------------------------------------------------------------------
GSList *Match::GetRefereeList ()
{
  return _referee_list;
}
