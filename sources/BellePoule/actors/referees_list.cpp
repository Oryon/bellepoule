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

#include <gdk/gdkkeysyms.h>
#include <ctype.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "util/attribute.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "util/dnd_config.hpp"
#include "util/glade.hpp"
#include "util/canvas.hpp"
#include "application/weapon.hpp"
#include "referee.hpp"
#include "tally_counter.hpp"

#include "referees_list.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  RefereesList::RefereesList (Listener *listener)
    : Object ("RefereesList"),
      People::Checkin ("referees.glade", "Referee", nullptr)
  {
    _weapon   = nullptr;
    _listener = listener;

    g_object_set (_tree_view,
                  "rules-hint", FALSE,
                  NULL);

    {
      GSList *attr_list;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
                                          "plugin_ID",
                                          "cyphered_password",
                                          "IP",
                                          "password",
#endif
                                          "incident",
                                          "HS",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "pool_nr",
                                          "stage_start_rank",
                                          "promoted",
                                          "rank",
                                          "ranking",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          "score_quest",
                                          "strip",
                                          "time",
                                          NULL);

      {
        _collapsed_filter = new Filter (GetKlassName (),
                                        attr_list);

        _collapsed_filter->ShowAttribute ("name");
        _collapsed_filter->ShowAttribute ("workload_rate");
        _collapsed_filter->ShowAttribute ("club");
        _collapsed_filter->ShowAttribute ("league");
        _collapsed_filter->ShowAttribute ("region");
        _collapsed_filter->ShowAttribute ("country");

        SetFilter (_collapsed_filter);
      }

      {
        _expanded_filter = new Filter (GetKlassName (),
                                       g_slist_copy (attr_list));

        _expanded_filter->ShowAttribute ("connection");
        _expanded_filter->ShowAttribute ("attending");
        _expanded_filter->ShowAttribute ("name");
        _expanded_filter->ShowAttribute ("workload_rate");
        _expanded_filter->ShowAttribute ("club");
        _expanded_filter->ShowAttribute ("league");
        _expanded_filter->ShowAttribute ("country");

        CreateForm (_expanded_filter,
                    "Referee");
      }

      SetVisibileFunc ((GtkTreeModelFilterVisibleFunc) RefereeIsVisible,
                       this);
    }

    {
      _dnd_config->AddTarget ("bellepoule/referee", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

      _dnd_config->SetOnAWidgetSrc (GTK_WIDGET (_tree_view),
                                    GDK_MODIFIER_MASK,
                                    GDK_ACTION_COPY);

      ConnectDndSource (GTK_WIDGET (_tree_view));
      gtk_drag_source_set_icon_name (GTK_WIDGET (_tree_view),
                                     "preferences-desktop-theme");
    }

    // Popup menu
    SetPopupVisibility ("PlayersList::PasteLinkAction",
                        TRUE);

    RefreshAttendingDisplay ();
  }

  // --------------------------------------------------------------------------------
  RefereesList::~RefereesList ()
  {
    _collapsed_filter->Release ();
    _expanded_filter->Release  ();

    Object::TryToRelease (_weapon);
  }

  // --------------------------------------------------------------------------------
  GtkWidget *RefereesList::GetHeader ()
  {
    return _glade->GetWidget ("tab_header");
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnPlayerLoaded (Player *referee,
                                     Player *owner)
  {
    ((Referee*) referee)->GiveAnId ();
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnFormEvent (Player              *referee,
                                  People::Form::Event  event)
  {
    ((Referee*) referee)->GiveAnId ();

    Checkin::OnFormEvent (referee,
                          event);

    if (_listener)
    {
      _listener->OnRefereeUpdated (this,
                                   referee);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereesList::TogglePlayerAttr (Player              *player,
                                       Player::AttributeId *attr_id,
                                       gboolean             new_value,
                                       gboolean             popup_on_error)
  {
    Checkin::TogglePlayerAttr (player,
                               attr_id,
                               new_value,
                               popup_on_error);

    if (_listener)
    {
      _listener->OnRefereeUpdated (this,
                                   player);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereesList::SetWeapon (Weapon *weapon)
  {
    _weapon = weapon;
    _weapon->Retain ();

    gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("weapon")),
                        gettext (weapon->GetImage ()));
  }

  // --------------------------------------------------------------------------------
  const gchar *RefereesList::GetWeaponCode ()
  {
    if (_weapon)
    {
      return _weapon->GetXmlImage ();
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  void RefereesList::Monitor (Player *referee)
  {
    Checkin::Monitor (referee);

    referee->SetChangeCbk ("connection",
                           (Player::OnChange) OnRefereeChanged,
                           this);
    referee->SetChangeCbk ("workload_rate",
                           (Player::OnChange) OnRefereeChanged,
                           this);
    referee->SetChangeCbk ("weapon",
                           (Player::OnChange) OnRefereeChanged,
                           this);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::Add (Player *player)
  {
    if (_weapon)
    {
      Player::AttributeId weapon_attr_id ("weapon");
      Attribute *weapon_attr = player->GetAttribute (&weapon_attr_id);
      Referee   *referee     = (Referee*) player;
      GString   *digest      = nullptr;

      if (weapon_attr)
      {
        gchar *upper = g_ascii_strup (weapon_attr->GetStrValue (),
                                      -1);

        if ((g_strrstr (upper, _weapon->GetXmlImage ()) == nullptr))
        {
          digest = g_string_new (nullptr);
          digest = g_string_append (digest,
                                    weapon_attr->GetStrValue ());
        }

        g_free (upper);
      }
      else
      {
        digest = g_string_new (nullptr);
      }

      if (digest)
      {
        digest = g_string_append (digest,
                                  _weapon->GetXmlImage ());

        referee->SetAttributeValue (&weapon_attr_id,
                                    digest->str);

        g_string_free (digest,
                       TRUE);
      }
    }

    Checkin::Add (player);
  }

  // --------------------------------------------------------------------------------
  gboolean RefereesList::RefereeIsVisible (GtkTreeModel *model,
                                           GtkTreeIter  *iter,
                                           RefereesList *referee_list)
  {
    if (referee_list->_filter == referee_list->_expanded_filter)
    {
      return TRUE;
    }
    else
    {
      Player *referee = GetPlayer (model,
                                   iter);

      if (referee)
      {
        Player::AttributeId  attending_attr_id ("attending");
        Attribute           *attending_attr = referee->GetAttribute (&attending_attr_id);

        return (attending_attr->GetUIntValue () != 0);
      }
    }
    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnRefereeChanged (Player    *referee,
                                       Attribute *attr,
                                       Object    *owner,
                                       guint      step)
  {
    Checkin *checkin = dynamic_cast <Checkin *> (owner);

    checkin->Update (referee);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnDragDataGet (GtkWidget        *widget,
                                    GdkDragContext   *drag_context,
                                    GtkSelectionData *data,
                                    guint             key,
                                    guint             time)
  {
    GList   *selected    = RetreiveSelectedPlayers ();
    Player  *referee     = (Player *) selected->data;
    guint32  referee_ref = referee->GetRef ();

    gtk_selection_data_set (data,
                            gtk_selection_data_get_target (data),
                            32,
                            (guchar *) &referee_ref,
                            sizeof (referee_ref));

    g_list_free (selected);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::Expand ()
  {
    g_object_set (_tree_view,
                  "rules-hint", TRUE,
                  NULL);

    {
      GtkWidget *panel = _glade->GetWidget ("edit_panel");

      gtk_widget_show (panel);
    }

    SetFilter (_expanded_filter);

    SetPopupVisibility ("RefereesList::JobActions",
                        FALSE);

    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  gboolean RefereesList::IsCollapsed ()
  {
    return (_filter == _collapsed_filter);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::Collapse ()
  {
    g_object_set (_tree_view,
                  "rules-hint", FALSE,
                  NULL);

    {
      GtkWidget *panel = _glade->GetWidget ("edit_panel");

      gtk_widget_hide (panel);
    }

    SetFilter (_collapsed_filter);

    SetPopupVisibility ("RefereesList::JobActions",
                        TRUE);

    OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnCheckinClicked ()
  {
    if (_listener)
    {
      _listener->OnOpenCheckin (this);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereesList::RefreshAttendingDisplay ()
  {
    Checkin::RefreshAttendingDisplay ();

    {
      gchar *text;

      text = g_strdup_printf ("%d", _tally_counter->GetPresentsCount ());
      gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("attending_tab")),
                          text);
      g_free (text);

      text = g_strdup_printf ("%d", _tally_counter->GetAbsentsCount ());
      gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("absent_tab")),
                          text);
      g_free (text);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereesList::DrawContainerPage (GtkPrintOperation *operation,
                                        GtkPrintContext   *context,
                                        gint               page_nr)
  {
    if (_weapon)
    {
      GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);

      goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                           _weapon->GetImage (),
                           10.0, 0.8,
                           80.0,
                           GTK_ANCHOR_NORTH,
                           "alignment", PANGO_ALIGN_CENTER,
                           "fill-color", "black",
                           "font", BP_FONT "Bold 5px", NULL);

      goo_canvas_render (canvas,
                         gtk_print_context_get_cairo_context (context),
                         nullptr,
                         1.0);

      gtk_widget_destroy (GTK_WIDGET (canvas));
    }

    {
      cairo_t *cr = gtk_print_context_get_cairo_context (context);

      cairo_translate (cr,
                       0.0,
                       GetPrintHeaderSize (context, SizeReferential::ON_SHEET));
    }
  }

  // --------------------------------------------------------------------------------
  void RefereesList::CellDataFunc (GtkTreeViewColumn *tree_column,
                                   GtkCellRenderer   *cell,
                                   GtkTreeModel      *tree_model,
                                   GtkTreeIter       *iter)
  {
    if (GTK_IS_CELL_RENDERER_TEXT (cell))
    {
      Player *referee = GetPlayer (tree_model,
                                   iter);

      GdkColor *color = (GdkColor *) referee->GetPtrData (nullptr,
                                                          "RefereesList::CellColor");

      if (color)
      {
        g_object_set (cell,
                      "weight",         600,
                      "weight-set",     TRUE,
                      "foreground-gdk", color,
                      "foreground-set", TRUE,
                      NULL);
      }
      else
      {
        g_object_set (cell,
                      "weight-set",     FALSE,
                      "foreground-set", FALSE,
                      NULL);
      }

      {
        Player::AttributeId  weapon_attr_id ("weapon");
        Attribute           *weapon_attr = referee->GetAttribute (&weapon_attr_id);
        gchar               *weapons     = weapon_attr->GetStrValue ();

        for (gchar *w = weapons; *w != '\0'; w++)
        {
          if (g_ascii_toupper (w[0]) == GetWeaponCode ()[0])
          {
            if (g_ascii_isupper (w[0]))
            {
              g_object_set (cell,
                            "strikethrough-set", FALSE,
                            NULL);
            }
            else
            {
              g_object_set (cell,
                            "strikethrough-set", TRUE,
                            "strikethrough",     TRUE,
                            NULL);
            }
            break;
          }
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_checkin_clicked (GtkToolButton *widget,
                                                      Object        *owner)
  {
    RefereesList *rl = dynamic_cast <RefereesList *> (owner);

    rl->OnCheckinClicked ();
  }
}
