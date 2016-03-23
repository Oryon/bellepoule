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

#include "util/module.hpp"
#include "util/attribute.hpp"
#include "util/player.hpp"

#include "players_store.hpp"

namespace People
{
  class PlayersList : public Module
  {
    public:
      static Player *GetPlayer (GtkTreeModel *model,
                                GtkTreeIter  *iter);

      PlayersList (const gchar *glade_file,
                   guint        rights = SORTABLE | MODIFIABLE);

      virtual void Add (Player *player);

      void Wipe ();

      void SetFilter (Filter *filter);

      void OnAttrListUpdated ();

      void Update (Player *player);

      Player *GetPlayerFromRef (guint ref);

      GSList *GetList ();

      virtual guint PreparePrint (GtkPrintOperation *operation,
                                  GtkPrintContext   *context);
      virtual void DrawPage (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             gint               page_nr);
      virtual void OnEndPrint (GtkPrintOperation *operation,
                               GtkPrintContext   *context);
      void DrawBarePage (GtkPrintOperation *operation,
                         GtkPrintContext   *context,
                         gint               page_nr);

      void ExpandAll ();

      void CollapseAll ();

      void Disclose (const gchar *as);

      void Spread ();

      void DumpToHTML (FILE *file);

      static const guint NO_RIGHT   = 0x00000000;
      static const guint SORTABLE   = 0x00000001;
      static const guint MODIFIABLE = 0x00000002;

    protected:
      GtkTreeView *_tree_view;
      GSList      *_player_list;

      typedef gboolean (*CustomFilter) (Player *player, PlayersList *owner);

      virtual ~PlayersList ();

      void UpdateHierarchy (Player *player);

      void RemoveSelection ();

      void Remove (Player *player);

      void SetSensitiveState (gboolean sensitive_value);

      GSList *CreateCustomList (CustomFilter  filter,
                                PlayersList  *owner);

      GSList *GetSelectedPlayers ();

      void SetAttributeRight (const gchar *name,
                              gboolean     modifiable);

      virtual void OnListChanged ();

      Player *GetPlayerWithAttribute (Player::AttributeId *attr_id,
                                      Attribute           *attr);

      GtkAction *GetAction (const gchar *name);

      virtual void SelectTreeMode ();

      virtual void SelectFlatMode ();

      virtual void TogglePlayerAttr (Player              *player,
                                     Player::AttributeId *attr_id,
                                     gboolean             new_value,
                                     gboolean             popup_on_error = FALSE);

      void SetPasteVisibility (gboolean visibility);

    private:
      static GSList *_clipboard;

      guint         _rights;
      guint         _nb_player_per_page;
      gdouble       _print_scale;
      guint         _nb_pages;
      gint          _selector_column;
      gdouble      *_column_width;
      gboolean      _flat_print;
      PlayersStore *_store;
      GtkUIManager *_ui_manager;

      void RefreshDisplay ();

      virtual gboolean IsTableBorder (guint place);

      void SetColumn (guint           id,
                      Filter::Layout *attr_layout,
                      gint            at);

      Player *GetPlayer (const gchar *path_string);

      Player *GetNextPrintablePlayer (GtkTreeModel *model,
                                      GtkTreeIter  *iter,
                                      gboolean      print_full_list);

      gboolean IterNextNode (GtkTreeModel *model,
                             GtkTreeIter  *iter);

      virtual gboolean PlayerIsPrintable (Player *player);

      void OnCellToggled (gchar         *path_string,
                          gboolean       is_active,
                          AttributeDesc *desc);

      void PrintHeader (GooCanvasItem *root_item,
                        gboolean       update_column_width);

      guint PrintAllPlayers (GtkPrintOperation *operation,
                             GooCanvas         *canvas,
                             GtkPrintContext   *context,
                             gint               page_nr);

      void PrintPlayer (GooCanvasItem   *table,
                        GtkPrintContext *context,
                        GtkTreePath     *path,
                        Player          *player,
                        guint            row,
                        gboolean         update_column_width);

      void GetPrintScale (GtkPrintOperation *operation,
                          GtkPrintContext   *context,
                          guint             *players_count,
                          gdouble           *scale,
                          gdouble           *w,
                          gdouble           *h);

      virtual void OnPlayerRemoved (Player *player) {};

      static void OnCopySelection (GtkWidget   *w,
                                   PlayersList *players_list);

      static void OnPasteSelection (GtkWidget   *w,
                                    PlayersList *players_list);

      static void OnCellDataFunc (GtkTreeViewColumn *tree_column,
                                  GtkCellRenderer   *cell,
                                  GtkTreeModel      *tree_model,
                                  GtkTreeIter       *iter,
                                  PlayersList       *players_list);

      virtual void CellDataFunc (GtkTreeViewColumn *tree_column,
                                 GtkCellRenderer   *cell,
                                 GtkTreeModel      *tree_model,
                                 GtkTreeIter       *iter);

      static void on_cell_toggled (GtkCellRendererToggle *cell,
                                   gchar                 *path_string,
                                   gpointer               user_data);

      static gint CompareIterator (GtkTreeModel        *model,
                                   GtkTreeIter         *a,
                                   GtkTreeIter         *b,
                                   Player::AttributeId *attr_id);

      static gint OnButtonPress (GtkWidget *widget,
                                 GdkEvent  *event);
  };
}

#endif
