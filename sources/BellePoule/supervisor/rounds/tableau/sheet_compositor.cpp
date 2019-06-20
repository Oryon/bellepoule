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

#include "../../match.hpp"

#include "sheet_compositor.hpp"

namespace Table
{
  // --------------------------------------------------------------------------------
  Page::Page ()
    : Object ("Page")
  {
    _matches             = nullptr;
    _current_match       = nullptr;
    _accepted_size       = 0;
    _current_match_index = 0;
  }

  // --------------------------------------------------------------------------------
  Page::~Page ()
  {
    g_list_free (_matches);
  }

  // --------------------------------------------------------------------------------
  void Page::StartIterator ()
  {
    _current_match = _matches;
  }

  // --------------------------------------------------------------------------------
  Match *Page::GetNextMatch ()
  {
    if (_current_match)
    {
      Match *match = (Match *) _current_match->data;

      _current_match = g_list_next (_current_match);
      _current_match_index++;
      return match;
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  gboolean Page::CutterMarkReached ()
  {
    if (_accepted_size)
    {
      if (_current_match)
      {
        return (_current_match_index % _accepted_size) == 0;
      }
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  GList *Page::FillWith (GList *what)
  {
    if (_matches == nullptr)
    {
      for (guint i = 0; i < 4; i++)
      {
        if (what)
        {
          GList *node = what;

          what = g_list_remove_link (what,
                                     node);
          _matches = g_list_append (_matches,
                                    node->data);
        }
        else
        {
          _matches = g_list_append (_matches,
                                    nullptr);
        }
      }
    }

    return what;
  }

  // --------------------------------------------------------------------------------
  gboolean Page::Append (GList *what)
  {
    guint what_length = g_list_length (what);

    if ((_accepted_size == 0) || (_accepted_size == what_length))
    {
      if (g_list_length (_matches) + what_length <= 4)
      {
        _matches = g_list_concat (_matches,
                                  g_list_copy (what));
        _accepted_size = what_length;
        return TRUE;
      }
    }

    return FALSE;
  }
}

namespace Table
{
  // --------------------------------------------------------------------------------
  SheetCompositor::SheetCompositor ()
    : Object ("SheetCompositor")
  {
    _match_by_referee = nullptr;
    _pages            = nullptr;
  }

  // --------------------------------------------------------------------------------
  SheetCompositor::~SheetCompositor ()
  {
    Reset ();
  }

  // --------------------------------------------------------------------------------
  void SheetCompositor::Reset ()
  {
    if (_match_by_referee)
    {
      g_list_free_full (_match_by_referee,
                        (GDestroyNotify) g_list_free);
      _match_by_referee = nullptr;
    }

    if (_pages)
    {
      FreeFullGList (Page, _pages);
    }
  }

  // --------------------------------------------------------------------------------
  void SheetCompositor::AddMatch (Match *match)
  {
    GList  *list    = nullptr;
    Player *referee = match->GetFirstReferee ();

    for (GList *current = _match_by_referee; current; current = g_list_next (current))
    {
      GList *match_list  = (GList *) current->data;
      Match *first_match = (Match *) match_list->data;

      if (first_match->GetFirstReferee () == referee)
      {
        list = match_list;
        _match_by_referee = g_list_remove_link (_match_by_referee,
                                                current);
        break;
      }
    }

    list = g_list_insert_sorted (list,
                                 match,
                                 (GCompareFunc) Match::Compare);

    _match_by_referee = g_list_prepend (_match_by_referee,
                                        list);
  }

  // --------------------------------------------------------------------------------
  guint SheetCompositor::GetPageCount (gdouble paper_w,
                                       gdouble paper_h)
  {
    if (paper_w > paper_h)
    {
      _nb_match_per_sheet = 2;
    }
    else
    {
      _nb_match_per_sheet = 4;
    }

    _match_by_referee = g_list_sort (_match_by_referee,
                                     (GCompareFunc) CompareGListLength);

    for (GList *m = _match_by_referee; m; m = g_list_next (m))
    {
      GList *matchs = (GList *) m->data;

      if (g_list_length (matchs) >= 4)
      {
        while (matchs)
        {
          Page *page = CreatePage ();

          matchs = page->FillWith (matchs);
        }
      }
      else
      {
        for (GList *p = _pages; p; p = g_list_next (p))
        {
          Page *page = (Page *) p->data;

          if (page->Append (matchs))
          {
            matchs = nullptr;
            break;
          }
        }
      }

      if (matchs)
      {
        Page *page = CreatePage ();

        page->Append (matchs);
      }
    }

    return g_list_length (_pages);
  }

  // --------------------------------------------------------------------------------
  guint SheetCompositor::MatchPerSheet ()
  {
    return _nb_match_per_sheet;
  }

  // --------------------------------------------------------------------------------
  Page *SheetCompositor::GetPage (guint p)
  {
    Page *page = (Page *) g_list_nth_data (_pages,
                                           p);

    page->StartIterator ();

    return page;
  }

  // --------------------------------------------------------------------------------
  Page *SheetCompositor::CreatePage ()
  {
    Page *page = new Page ();

    _pages = g_list_append (_pages,
                            page);
    return page;
  }

  // --------------------------------------------------------------------------------
  gint SheetCompositor::CompareGListLength (GList *a,
                                            GList *b)
  {
    return g_list_length (b) - g_list_length (a);
  }
}
