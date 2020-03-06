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

#include "util/glade.hpp"
#include "util/player.hpp"
#include "../../match.hpp"
#include "hall.hpp"

namespace Quest
{
  enum class ColumnId
  {
    PISTE_str,
    FENCER1_str,
    FENCER2_str,
    MATCH_ptr,
    PISTE_uint,
    STATUS_str
  };

  // --------------------------------------------------------------------------------
  Hall::Hall (Listener *listener)
    : Object ("Quest::Hall"),
    Module ("mini_hall.glade")
  {
    _store    = GTK_LIST_STORE (_glade->GetGObject ("hall_liststore"));
    _listener = listener;
  }

  // --------------------------------------------------------------------------------
  Hall::~Hall ()
  {
    gtk_list_store_clear (_store);
  }

  // --------------------------------------------------------------------------------
  void Hall::Dump ()
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_store),
                                                               &iter);

    while (iter_is_valid)
    {
      Match *match;
      guint  piste;

      gtk_tree_model_get (GTK_TREE_MODEL (_store), &iter,
                          ColumnId::PISTE_uint, &piste,
                          ColumnId::MATCH_ptr,  &match,
                          -1);

      if (match)
      {
        printf ("%d ==> %s\n", piste, match->GetName ());
      }
      else
      {
        printf ("%d ==> nullptr\n", piste);
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_store),
                                                &iter);
    }
    printf ("\n");
  }

  // --------------------------------------------------------------------------------
  void Hall::Clear ()
  {
    GtkTreeIter iter;
    gboolean    iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_store),
                                                               &iter);

    while (iter_is_valid)
    {
      gtk_list_store_set (_store, &iter,
                          ColumnId::FENCER1_str, nullptr,
                          ColumnId::FENCER2_str, nullptr,
                          ColumnId::MATCH_ptr,   nullptr,
                          -1);

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_store),
                                                &iter);
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Hall::GetIter (Match       *match,
                          GtkTreeIter *iter)
  {
    gboolean iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (_store),
                                                            iter);

    while (iter_is_valid)
    {
      Match *current;

      gtk_tree_model_get (GTK_TREE_MODEL (_store), iter,
                          ColumnId::MATCH_ptr, &current,
                          -1);

      if (current == match)
      {
        return TRUE;
      }

      iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_store),
                                                iter);
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Hall::SetPisteCount (guint count)
  {
    // Remove excess entries
    {
      GtkTreeIter iter;
      gboolean    iter_is_valid = gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_store),
                                                                 &iter,
                                                                 nullptr,
                                                                 count);
      while (iter_is_valid)
      {
        iter_is_valid = gtk_list_store_remove (_store,
                                               &iter);
        iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (_store),
                                                  &iter);
      }
    }

    // Add missing entries
    {
      gint current_count = gtk_tree_model_iter_n_children (GTK_TREE_MODEL (_store),
                                                           nullptr);

      for (guint i = current_count; i < count; i++)
      {
        GtkTreeIter  iter;
        gchar       *piste_name = g_strdup_printf ("[%d]", i+1);

        gtk_list_store_append (_store,
                               &iter);
        gtk_list_store_set (_store, &iter,
                            ColumnId::PISTE_str,  piste_name,
                            ColumnId::PISTE_uint, i+1,
                            -1);
        g_free (piste_name);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::SetStatusIcon (Match       *match,
                            const gchar *stock_icon)
  {
    GtkTreeIter iter;

    if (GetIter (match,
                 &iter))
    {
      gtk_list_store_set (_store, &iter,
                          ColumnId::STATUS_str, stock_icon,
                          -1);
      gtk_widget_queue_resize (GetRootWidget ());
    }
  }

  // --------------------------------------------------------------------------------
  guint Hall::BookPiste (Match *owner)
  {
    GtkTreeIter iter;
    guint       piste = owner->GetPiste ();

    if (piste == 0)
    {
      if (GetIter (nullptr,
                   &iter))
      {
        gtk_tree_model_get (GTK_TREE_MODEL (_store), &iter,
                            ColumnId::PISTE_uint, &piste,
                            -1);
      }
    }
    else if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_store),
                                            &iter,
                                            nullptr,
                                            piste-1) == FALSE)
    {
      return 0;
    }

    if (piste)
    {
      Player *fencer1 = owner->GetOpponent (0);
      Player *fencer2 = owner->GetOpponent (1);
      gchar  *name1   = fencer1->GetName ();
      gchar  *name2   = fencer2->GetName ();

      owner->SetPiste (piste);
      gtk_list_store_set (_store, &iter,
                          ColumnId::FENCER1_str, name1,
                          ColumnId::FENCER2_str, name2,
                          ColumnId::MATCH_ptr,   owner,
                          -1);

      g_free (name1);
      g_free (name2);

      gtk_widget_queue_resize (GetRootWidget ());
    }

    return piste;
  }

  // --------------------------------------------------------------------------------
  void Hall::FreePiste (Match *owner)
  {
    GtkTreeIter iter;

    if (GetIter (owner,
                 &iter))
    {
      gtk_list_store_set (_store, &iter,
                          ColumnId::FENCER1_str, nullptr,
                          ColumnId::FENCER2_str, nullptr,
                          ColumnId::MATCH_ptr,   nullptr,
                          -1);
    }
  }

  // --------------------------------------------------------------------------------
  void Hall::OnPisteSelected ()
  {
    GtkWidget        *treeview = _glade->GetWidget ("treeview");
    GtkTreeSelection *selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
    GtkTreeIter       iter;

    if (gtk_tree_selection_get_selected (selection,
                                         nullptr,
                                         &iter))
    {
      Match *match;

      gtk_tree_model_get (GTK_TREE_MODEL (_store),
                          &iter,
                          ColumnId::MATCH_ptr, &match,
                          -1);

      if (match)
      {
        _listener->OnMatchSelected (match);

      }
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT gboolean on_treeview_button_release_event (GtkWidget *widget,
                                                                        GdkEvent  *event,
                                                                        Object    *owner)
  {
    Hall *h = dynamic_cast <Hall *> (owner);

    h->OnPisteSelected ();
    return FALSE;
  }
}
