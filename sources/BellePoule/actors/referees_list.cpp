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

#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "util/attribute.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "application/weapon.hpp"
#include "referee.hpp"
#include "tally_counter.hpp"

#include "referees_list.hpp"

namespace People
{
  // --------------------------------------------------------------------------------
  RefereesList::RefereesList (Listener *listener)
    : Object ("RefereesList"),
      People::Checkin ("referees.glade", "Referee", NULL)
  {
    _weapon   = NULL;
    _listener = listener;

    {
      GSList *attr_list;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
#endif
                                          "IP",
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
                                          NULL);

      {
        _collapsed_filter = new Filter (attr_list,
                                        this);
        _collapsed_filter->ShowAttribute ("name");
        _collapsed_filter->ShowAttribute ("workload_rate");
        _collapsed_filter->ShowAttribute  ("club");
        _collapsed_filter->ShowAttribute  ("league");
        _collapsed_filter->ShowAttribute  ("country");

        SetFilter (_collapsed_filter);
      }

      {
        _expanded_filter = new Filter (g_slist_copy (attr_list),
                                       this);

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
    SetPopupVisibility ("PlayersList::WriteAction",
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
    GiveRefereeAnId ((Referee *) referee);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnFormEvent (Player                  *referee,
                                  People::Form::FormEvent  event)
  {
    GiveRefereeAnId ((Referee *) referee);

    Checkin::OnFormEvent (referee,
                          event);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::GiveRefereesAnId ()
  {
    GList *current = GetList ();

    while (current)
    {
      Player  *player  = (Player *) current->data;
      Referee *referee = dynamic_cast <Referee *> (player);

      GiveRefereeAnId (referee);

      current = g_list_next (current);
    }
  }

  // --------------------------------------------------------------------------------
  void RefereesList::GiveRefereeAnId (Referee *referee)
  {
    referee->GiveAnId ();

    if (_weapon)
    {
      Player::AttributeId weapon_attr_id ("weapon");

      referee->SetAttributeValue (&weapon_attr_id,
                                  _weapon->GetXmlImage ());
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

    return NULL;
  }

  // --------------------------------------------------------------------------------
  void RefereesList::Monitor (Player *referee)
  {
    Checkin::Monitor (referee);

    referee->SetChangeCbk ("connection",
                           (Player::OnChange) OnConnectionChanged,
                           this);
    referee->SetChangeCbk ("workload_rate",
                           (Player::OnChange) OnParticipationRateChanged,
                           this);
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
  void RefereesList::OnConnectionChanged (Player    *referee,
                                          Attribute *attr,
                                          Object    *owner,
                                          guint      step)
  {
    Checkin *checkin = dynamic_cast <Checkin *> (owner);

    checkin->Update (referee);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::OnParticipationRateChanged (Player    *referee,
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
    GList   *selected    = GetSelectedPlayers ();
    Player  *referee     = (Player *) selected->data;
    guint32  referee_ref = referee->GetRef ();

    gtk_selection_data_set (data,
                            gtk_selection_data_get_target (data),
                            32,
                            (guchar *) &referee_ref,
                            sizeof (referee_ref));
  }

  // --------------------------------------------------------------------------------
  void RefereesList::Expand ()
  {
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
    return (GetFilter () == _collapsed_filter);
  }

  // --------------------------------------------------------------------------------
  void RefereesList::Collapse ()
  {
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
  extern "C" G_MODULE_EXPORT void on_checkin_clicked (GtkToolButton *widget,
                                                      Object        *owner)
  {
    RefereesList *rl = dynamic_cast <RefereesList *> (owner);

    rl->OnCheckinClicked ();
  }
}
