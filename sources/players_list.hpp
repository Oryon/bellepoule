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

#ifndef players_list_hpp
#define players_list_hpp

#include <gtk/gtk.h>
#include <goocanvas.h>

#include "module.hpp"
#include "attribute.hpp"
#include "player.hpp"

class PlayersList : public Module
{
  public:
    virtual void Add (Player *player);

    void Wipe ();

  protected:
    static const guint NO_RIGHT   = 0x00000000;
    static const guint SORTABLE   = 0x00000001;
    static const guint MODIFIABLE = 0x00000002;

    GSList *_player_list;

    typedef gboolean (*CustomFilter) (Player *player);

    PlayersList (gchar *glade_file,
                 guint  rights = SORTABLE | MODIFIABLE);

    ~PlayersList ();

    void RemoveSelection ();

    void SetSensitiveState (gboolean sensitive_value);

    void Update (Player *player);

    void OnAttrListUpdated ();

    GSList *CreateCustomList (CustomFilter filter);

    GSList *GetSelectedPlayers ();

    void SetFilter (Filter *filter);

    void SetAttributeRight (gchar    *name,
                            gboolean  modifiable);

    virtual void OnListChanged () {};

  protected:
    GtkListStore *_store;

  public:
    static Player *GetPlayer (GtkTreeModel *model,
                              GtkTreeIter  *iter);

  private:
    void PrintHeader (GooCanvasItem *root_item,
                      gboolean       update_column_width);
    void PrintPlayer (GooCanvasItem   *table,
                      GtkPrintContext *context,
                      Player          *player,
                      guint            row,
                      gboolean         update_column_width);
    void GetPrintScale (GtkPrintContext *context,
                        gdouble         *scale,
                        gdouble         *w,
                        gdouble         *h);
    void OnBeginPrint (GtkPrintOperation *operation,
                       GtkPrintContext   *context);
    void OnDrawPage (GtkPrintOperation *operation,
                     GtkPrintContext   *context,
                     gint               page_nr);
    void OnEndPrint (GtkPrintOperation *operation,
                     GtkPrintContext   *context);

  private:
    GtkWidget    *_tree_view;
    guint         _rights;
    guint         _nb_player_per_page;
    gdouble       _print_scale;
    guint         _nb_pages;
    gdouble      *_column_width;

    void SetColumn (guint          id,
                    AttributeDesc *desc,
                    gint           at);

    GtkTreeRowReference *GetPlayerRowRef (GtkTreeIter *iter);

    Player *GetPlayer (const gchar *path_string);

    virtual gboolean PlayerIsPrintable (Player *player);

    void OnCellEdited (gchar         *path_string,
                       gchar         *new_text,
                       AttributeDesc *desc);

    void OnCellToggled (gchar         *path_string,
                        gboolean       is_active,
                        AttributeDesc *desc);

    virtual void OnPlayerRemoved (Player *player) {};

    static void on_cell_toggled (GtkCellRendererToggle *cell,
                                 gchar                 *path_string,
                                 gpointer               user_data);

    static void on_cell_edited (GtkCellRendererText *cell,
                                gchar               *path_string,
                                gchar               *new_text,
                                gpointer             user_data);

    static gint CompareIterator (GtkTreeModel        *model,
                                 GtkTreeIter         *a,
                                 GtkTreeIter         *b,
                                 Player::AttributeId *attr_id);
};

#endif
