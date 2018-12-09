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

#include "util/attribute.hpp"
#include "actors/checkin.hpp"
#include "actors/form.hpp"

#include "pillow_dialog.hpp"

namespace Pool
{
  const gchar *PillowDialog::_revert_context = "PillowDialog::RevertContext";

  // --------------------------------------------------------------------------------
  PillowDialog::PillowDialog (Module      *owner,
                              Listener    *listener,
                              const gchar *name)
    : Object ("PillowDialog")
  {
     _parent_window    = nullptr;
     _postponed_revert = nullptr;
     _listener         = listener;

    _checkin = new People::Checkin ("pillow_list.glade",
                                    "Fencer",
                                    nullptr);

    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                          "ref",
                                          "plugin_ID",
#endif
                                          "IP",
                                          "password",
                                          "cyphered_password",
                                          "HS",
                                          "exported",
                                          "final_rank",
                                          "global_status",
                                          "indice",
                                          "level",
                                          "workload_rate",
                                          "pool_nr",
                                          "promoted",
                                          "rank",
                                          "status",
                                          "team",
                                          "victories_count",
                                          "bouts_count",
                                          "victories_ratio",
                                          NULL);
      filter = new Filter (GetKlassName (),
                           attr_list);

      filter->ShowAttribute ("attending");
      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("country");
      filter->ShowAttribute ("region");
      filter->ShowAttribute ("league");
      filter->ShowAttribute ("club");

      _checkin->SetFilter (filter);

      filter->Release ();
    }

    {
      gchar     *hook_name = g_strdup_printf ("%s_list", name);
      GtkWidget *hook      = GTK_WIDGET (owner->GetGObject (hook_name));

      owner->Plug (_checkin,
                   hook);

      {
        GtkWidget *root = gtk_widget_get_ancestor (hook,
                                                   GTK_TYPE_WINDOW);

        gtk_window_set_resizable (GTK_WINDOW (root),
                                  TRUE);
      }

      g_free (hook_name);
    }

    {
      gchar *box_name = g_strdup_printf ("%s_warning_box", name);

      _warning_box = GTK_WIDGET (owner->GetGObject (box_name));

      g_free (box_name);
    }

    {
      gchar *label_name = g_strdup_printf ("%s_warning_label", name);

      _warning_label = GTK_LABEL (owner->GetGObject (label_name));

      g_free (label_name);
    }
  }

  // --------------------------------------------------------------------------------
  PillowDialog::~PillowDialog ()
  {
    _checkin->Release ();
  }

  // --------------------------------------------------------------------------------
  void PillowDialog::AddFormButton (Module *player_owner)
  {
    Filter *checkin_filter = _checkin->GetFilter ();
    GSList *attr_list      = g_slist_copy (checkin_filter->GetAttrList ());

    for (GSList *current = attr_list; current; current = g_slist_next (current))
    {
      AttributeDesc *attr = (AttributeDesc *) current->data;

      if (g_strcmp0 (attr->_code_name, "attending") == 0)
      {
        attr_list = g_slist_delete_link (attr_list,
                                         current);
        break;
      }
    }

    {
      Filter       *form_filter;
      People::Form *form;

      form_filter = new Filter (GetKlassName (),
                                attr_list);

      form = _checkin->CreateForm (form_filter,
                                   "Fencer",
                                   player_owner);

      form->AddListener (this);
      form->CloseOnAdd ();

      form_filter->Release ();
    }

    checkin_filter->Release ();
  }

  // --------------------------------------------------------------------------------
  void PillowDialog::SetParentWindow (GtkWindow *parent)
  {
    _parent_window = parent;
  }

  // --------------------------------------------------------------------------------
  void PillowDialog::Populate (GSList *fencers,
                               guint   nb_pool)
  {
     _nb_pool = nb_pool;

    _checkin->Wipe ();

    for (GSList *current = fencers; current; current = g_slist_next (current))
    {
      Player *fencer = (Player *) current->data;

      _checkin->Add (fencer);

      fencer->SetChangeCbk ("attending",
                            (Player::OnChange) OnAttendingChanged,
                            this);
    }

    if (fencers)
    {
      Player              *sample = (Player *) fencers->data;
      Player::AttributeId  attending_attr_id ("attending");
      Attribute           *attending_attr = sample->GetAttribute (&attending_attr_id);

      _revert_value = attending_attr->GetUIntValue ();
    }

    gtk_widget_set_visible (_warning_box,
                            FALSE);
  }

  // --------------------------------------------------------------------------------
  void PillowDialog::RevertAll ()
  {
    Player::AttributeId attending_attr_id ("attending");

    for (GList *current = _checkin->GetList (); current; current = g_list_next (current))
    {
      Player *fencer = (Player *) current->data;

      fencer->SetAttributeValue (&attending_attr_id,
                                 _revert_value);
    }
  }

  // --------------------------------------------------------------------------------
  void PillowDialog::Clear ()
  {
    for (GList *current = _checkin->GetList (); current; current = g_list_next (current))
    {
      Player *fencer = (Player *) current->data;

      fencer->RemoveCbkOwner (this);
      fencer->RemoveData (this,
                          _revert_context);
    }
  }

  // --------------------------------------------------------------------------------
  void PillowDialog::DisplayForm ()
  {
    _checkin->RaiseForm (_parent_window);
  }

  // --------------------------------------------------------------------------------
  void PillowDialog::OnAttendingChanged (Player    *player,
                                         Attribute *attr,
                                         Object    *owner,
                                         guint      step)
  {
    PillowDialog *pillow = dynamic_cast <PillowDialog *> (owner);
    gboolean      value  = attr->GetUIntValue ();

    gtk_widget_set_visible (pillow->_warning_box,
                            FALSE);

    if (pillow->_listener->OnAttendeeToggled (pillow,
                                              player,
                                              value) == FALSE)
    {
      player->Retain ();
      pillow->_postponed_revert = g_list_prepend (pillow->_postponed_revert,
                                                  player);
      g_idle_add ((GSourceFunc) DeferedRevert,
                  pillow);
    }
  }

  // --------------------------------------------------------------------------------
  guint PillowDialog::GetNbPools ()
  {
    return _nb_pool;
  }

  // --------------------------------------------------------------------------------
  gboolean PillowDialog::DeferedRevert (PillowDialog *pillow)
  {
    for (GList *current = pillow->_postponed_revert; current; current = g_list_next (current))
    {
      Player              *fencer = (Player *) current->data;
      Player::AttributeId attending_attr_id ("attending");

      fencer->SetAttributeValue (&attending_attr_id,
                                 pillow->_revert_value);
      pillow->_checkin->Update (fencer);
      fencer->Release ();
    }

    g_list_free (pillow->_postponed_revert);
    pillow->_postponed_revert = nullptr;

    {
      gchar *warning = g_strdup_printf (gettext ("Impossible to apply with this configuration of pools"));

      gtk_label_set_text (pillow->_warning_label,
                          warning);
      gtk_widget_set_visible (pillow->_warning_box,
                              TRUE);

      g_free (warning);
    }

    return G_SOURCE_REMOVE;
  }

  // --------------------------------------------------------------------------------
  const gchar *PillowDialog::GetRevertContext ()
  {
    return _revert_context;
  }

  // --------------------------------------------------------------------------------
  void PillowDialog::OnFormEvent (Player              *player,
                                  People::Form::Event  event)
  {
    if (event == People::Form::Event::NEW_PLAYER)
    {
      player->SetChangeCbk ("attending",
                            (Player::OnChange) OnAttendingChanged,
                            this);
    }
  }
}
