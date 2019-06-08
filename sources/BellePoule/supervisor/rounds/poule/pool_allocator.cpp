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

#include <libxml/encoding.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "util/global.hpp"
#include "util/fie_time.hpp"
#include "util/dnd_config.hpp"
#include "util/attribute.hpp"
#include "util/attribute_desc.hpp"
#include "util/filter.hpp"
#include "util/player.hpp"
#include "util/glade.hpp"
#include "util/data.hpp"
#include "util/xml_scheme.hpp"
#include "network/message.hpp"
#include "network/advertiser.hpp"
#include "actors/players_list.hpp"

#include "neo_swapper/neo_swapper.hpp"
#include "dispatcher/dispatcher.hpp"
#include "../../attendees.hpp"
#include "../../contest.hpp"
#include "../../match.hpp"

#include "pool.hpp"
#include "pool_zone.hpp"

#include "pool_allocator.hpp"

#ifdef DEBUG
#define DEBUG_SWAPPING
#endif

namespace Pool
{
#define VALUE_INIT {0,{{0}}}

  enum class ComboboxColumn
  {
    POOL_SIZE,
    NB_POOLS,
    BEST_PIXMAP
  };

  enum class SwappingColumn
  {
    CRITERIA,
    CRITERIA_NAME
  };

  extern "C" G_MODULE_EXPORT void on_nb_pools_combobox_changed (GtkWidget *widget,
                                                                Object    *owner);
  extern "C" G_MODULE_EXPORT void on_pool_size_combobox_changed (GtkWidget *widget,
                                                                 Object    *owner);

  const gchar *Allocator::_class_name     = N_ ("Pools arrangement");
  const gchar *Allocator::_xml_class_name = "";

  GdkPixbuf *Allocator::_warning_pixbuf = nullptr;
  GdkPixbuf *Allocator::_moved_pixbuf   = nullptr;

  // --------------------------------------------------------------------------------
  Allocator::Allocator (StageClass *stage_class)
    : Object ("Pool::Allocator"),
      Stage (stage_class),
      CanvasModule ("pool_allocator.glade")
  {
    Disclose ("BellePoule::Batch");

    _has_marshaller         = FALSE;
    _config_list            = nullptr;
    _selected_config        = nullptr;
    _main_table             = nullptr;
    _swapping_criteria_list = nullptr;
    _loaded                 = FALSE;

    _max_score = new Data ("ScoreMax",
                           5);

    _seeding_balanced = new Data ("RepartitionEquilibre",
                                  TRUE);

    _swapping = new Data ("Decalage",
                          (gchar *) nullptr);

    _combobox_store = GTK_LIST_STORE (_glade->GetGObject ("combo_liststore"));

    _latecomer_dialog = new PillowDialog (this,
                                          this,
                                          "latecomer");

    _absent_dialog = new PillowDialog (this,
                                       this,
                                       "absent");

    // Sensitive widgets
    {
      AddSensitiveWidget (_glade->GetWidget ("nb_pools_combobox"));
      AddSensitiveWidget (_glade->GetWidget ("pool_size_combobox"));
      AddSensitiveWidget (_glade->GetWidget ("swapping_criteria_hbox"));
      AddSensitiveWidget (_glade->GetWidget ("latecomer_toolbutton"));
      AddSensitiveWidget (_glade->GetWidget ("absent_toolbutton"));

      _swapping_sensitivity_trigger = new SensitivityTrigger ();
      _swapping_sensitivity_trigger->AddWidget (_glade->GetWidget ("swapping_criteria_hbox"));
    }

    _dnd_config->AddTarget ("bellepoule/referee", GTK_TARGET_SAME_APP|GTK_TARGET_OTHER_WIDGET);

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
                                          "attending",
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
                                          "strip",
                                          "time",
                                          NULL);
      filter = new Filter (GetKlassName (),
                           attr_list);

      filter->ShowAttribute ("stage_start_rank");
      filter->ShowAttribute ("name");
#ifdef DEBUG
      filter->ShowAttribute ("country");
      filter->ShowAttribute ("region");
      filter->ShowAttribute ("league");
#else
      filter->ShowAttribute ("first_name");
#endif
      filter->ShowAttribute ("club");

      SetFilter (filter);
      filter->Release ();
    }

    _swapper = NeoSwapper::Swapper::Create (this);

    {
      GtkContainer *swapping_hbox = GTK_CONTAINER (_glade->GetGObject ("regular_swapping_hbox"));
      GSList       *attr          = AttributeDesc::GetSwappableList ();

      while (attr)
      {
        AttributeDesc *attr_desc = (AttributeDesc *) attr->data;

        if (attr_desc->_uniqueness == AttributeDesc::Uniqueness::NOT_SINGULAR)
        {
          GtkWidget *check = gtk_check_button_new_with_label (attr_desc->_user_name);

          g_signal_connect (check, "toggled",
                            G_CALLBACK (OnSwappingToggled), this);
          g_object_set_data (G_OBJECT (check),
                             "criteria_attribute", attr_desc);
          gtk_container_add (swapping_hbox,
                             check);
        }

        attr = g_slist_next (attr);
      }
    }

    {
      _fencer_list = new People::PlayersList ("classification.glade",
                                              People::PlayersList::SORTABLE);
      {
        GSList *attr_list;
        Filter *filter;

        AttributeDesc::CreateExcludingList (&attr_list,
#ifndef DEBUG
                                            "ref",
#endif
                                            "IP",
                                            "password",
                                            "cyphered_password",
                                            "HS",
                                            "attending",
                                            "exported",
                                            "final_rank",
                                            "global_status",
                                            "indice",
                                            "level",
                                            "workload_rate",
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

        filter->ShowAttribute ("name");
        filter->ShowAttribute ("first_name");
        filter->ShowAttribute ("club");
        filter->ShowAttribute ("pool_nr");
        filter->ShowAttribute ("strip");
        filter->ShowAttribute ("time");

        _fencer_list->SetFilter    (filter);
        _fencer_list->SetDataOwner (this);

        // Extra swapping
        {
          GtkListStore *swapping_store = GTK_LIST_STORE (_glade->GetGObject ("swapping_liststore"));
          GtkTreeIter   iter;

          for (GSList *current = filter->GetAttrList (); current; current = g_slist_next (current))
          {
            AttributeDesc *attr_desc  = (AttributeDesc *) current->data;
            GSList        *swappables = AttributeDesc::GetSwappableList ();

            if (   (attr_desc->_rights     == AttributeDesc::Rights::PUBLIC)
                && (attr_desc->_uniqueness == AttributeDesc::Uniqueness::NOT_SINGULAR))
            {
              if (g_slist_find (swappables,
                                current->data) == nullptr)
              {
                gtk_list_store_append (swapping_store, &iter);
                gtk_list_store_set (swapping_store, &iter,
                                    SwappingColumn::CRITERIA,      attr_desc,
                                    SwappingColumn::CRITERIA_NAME, attr_desc->_user_name,
                                    -1);
              }
            }
          }

          {
            GtkComboBox *combobox = GTK_COMBO_BOX (_glade->GetWidget ("extra_combobox"));

            gtk_combo_box_set_active (combobox,
                                      0);
            g_signal_connect (G_OBJECT (combobox), "changed",
                              G_CALLBACK (OnSwappingToggled), this);
          }

          {
            GtkToggleButton *togglebutton = GTK_TOGGLE_BUTTON (_glade->GetWidget ("extra_toggle"));

            g_signal_connect (G_OBJECT (togglebutton), "toggled",
                              G_CALLBACK (OnSwappingToggled), this);

          }
        }

        filter->Release ();
      }
    }

    Net::Ring::_broker->RegisterPartnerListener (this);
  }

  // --------------------------------------------------------------------------------
  Allocator::~Allocator ()
  {
    Net::Ring::_broker->UnregisterPartnerListener (this);

    DeletePools ();

    _max_score->Release        ();
    _swapping->Release         ();
    _seeding_balanced->Release ();
    _fencer_list->Release      ();
    _swapper->Delete           ();
    _latecomer_dialog->Release ();
    _absent_dialog->Release    ();

    _swapping_sensitivity_trigger->Release ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::Declare ()
  {
    RegisterStageClass (gettext (_class_name),
                        _xml_class_name,
                        CreateInstance);

    {
      GtkWidget *image;

      image = gtk_image_new ();
      g_object_ref_sink (image);
      _warning_pixbuf = gtk_widget_render_icon (image,
                                                GTK_STOCK_DIALOG_WARNING,
                                                GTK_ICON_SIZE_BUTTON,
                                                nullptr);
      g_object_unref (image);

      image = gtk_image_new ();
      g_object_ref_sink (image);
      _moved_pixbuf = gtk_widget_render_icon (image,
                                              GTK_STOCK_REFRESH,
                                              GTK_ICON_SIZE_BUTTON,
                                              nullptr);
      g_object_unref (image);
    }
  }

  // --------------------------------------------------------------------------------
  Stage *Allocator::CreateInstance (StageClass *stage_class)
  {
    return new Allocator (stage_class);
  }

  // --------------------------------------------------------------------------------
  const gchar *Allocator::GetInputProviderClient ()
  {
    return "pool_stage";
  }

  // --------------------------------------------------------------------------------
  void Allocator::ShareAttendees (Stage *with)
  {
    Stage::ShareAttendees (with);

    _latecomer_dialog->AddFormButton (_attendees->GetOwner ());
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnPlugged ()
  {
    CanvasModule::OnPlugged ();

    _dnd_config->SetOnAWidgetDest (GTK_WIDGET (GetCanvas ()),
                                   GDK_ACTION_COPY);

    ConnectDndDest (GTK_WIDGET (GetCanvas ()));
    EnableDndOnCanvas ();

    _owner->Plug (_fencer_list,
                  _glade->GetWidget ("fencer_list_hook"));
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnUnPlugged ()
  {
    CanvasModule::OnUnPlugged ();

    _fencer_list->UnPlug ();
  }

  // --------------------------------------------------------------------------------
  gboolean Allocator::OnMessage (Net::Message *message)
  {
    for (GSList *current = _drop_zones; current; current = g_slist_next (current))
    {
      Pool *pool = GetPoolOf (current);

      if (message->Is ("SmartPoule::ScoreSheetCall"))
      {
        if (pool->GetNumber () == message->GetInteger ("batch"))
        {
          Net::Message *response = new Net::Message ("BellePoule::ScoreSheet");
          Player       *referee  = _contest->GetRefereeFromRef (message->GetInteger ("referee_id"));
          gchar        *address  = message->GetString ("address");

          {
            xmlBuffer *xml_buffer = xmlBufferCreate ();

            {
              XmlScheme *xml_scheme = new XmlScheme (xml_buffer);

              _contest->SaveHeader (xml_scheme);
              SaveHeader (xml_scheme);
              pool->Save (xml_scheme);

              xml_scheme->EndElement ();
              xml_scheme->EndElement ();

              xml_scheme->Release ();
            }

            response->Set ("competition", _contest->GetNetID ());
            response->Set ("stage",       GetNetID ());
            response->Set ("batch",       pool->GetNumber ());
            response->Set ("xml",         (const gchar *) xml_buffer->content);

            xmlBufferFree (xml_buffer);
          }

          referee->SendMessage (response,
                                address);
          g_free (address);

          response->Release ();

          return TRUE;
        }
      }
      else if (message->Is ("BellePoule2D::Roadmap"))
      {
        if (pool->OnMessage (message))
        {
          OnPoolRoadmap (pool,
                         message);

          if (Locked ())
          {
            Stage *next_stage = GetNextStage ();

            next_stage->OnMessage (message);
          }
          return TRUE;
        }
      }
    }

    return FALSE;
  }

  // --------------------------------------------------------------------------------
  void Allocator::SpreadJobs ()
  {
    for (GSList *current = _drop_zones; current; current = g_slist_next (current))
    {
      Pool *pool = GetPoolOf (current);

      pool->Spread ();
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::FeedParcel (Net::Message *parcel)
  {
    Stage *next_stage = GetNextStage ();

    parcel->Set ("competition", _contest->GetNetID ());
    parcel->Set ("name",        GetName ());
    parcel->Set ("done",        next_stage->Locked ());
  }

  // --------------------------------------------------------------------------------
  void Allocator::RecallJobs ()
  {
    for (GSList *current = _drop_zones; current; current = g_slist_next (current))
    {
      Pool *pool = GetPoolOf (current);

      pool->Recall ();
    }
  }

  // --------------------------------------------------------------------------------
  Object *Allocator::GetDropObjectFromRef (guint32 ref,
                                           guint   key)
  {
    return _contest->GetRefereeFromRef (ref);
  }

  // --------------------------------------------------------------------------------
  void Allocator::DropObject (Object   *object,
                              DropZone *source_zone,
                              DropZone *target_zone)
  {
    Player   *floating_object  = (Player *) object;
    PoolZone *source_pool_zone = nullptr;
    PoolZone *target_pool_zone = nullptr;

    if (target_zone)
    {
      target_pool_zone = (PoolZone *) target_zone;
      source_pool_zone = (PoolZone *) source_zone;

      if (floating_object->Is ("Fencer"))
      {
        if (source_pool_zone)
        {
          Pool *source_pool = source_pool_zone->GetPool ();

          source_pool->RemoveFencer (floating_object);
        }

        {
          Pool *target_pool = target_pool_zone->GetPool ();

          target_pool->AddFencer (floating_object);
        }

        {
          _swapper->MoveFencer ((Player *) floating_object,
                                source_pool_zone,
                                target_pool_zone);
          DisplaySwapperError ();
        }

        _fencer_list->Update (floating_object);
        RecallJobs ();
        SpreadJobs ();
      }
    }

    if (source_pool_zone || target_pool_zone)
    {
      FillPoolTable (source_pool_zone);
      FillPoolTable (target_pool_zone);
      FixUpTablesBounds ();
      SignalStatusUpdate ();
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Allocator::DragingIsForbidden (Object *object)
  {
    Player *player = (Player *) object;

    if (player)
    {
      if (player->Is ("Fencer"))
      {
        return Locked ();
      }
    }

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  GString *Allocator::GetFloatingImage (Object *floating_object)
  {
    GString *string = g_string_new ("");

    if (floating_object)
    {
      Player *player        = (Player *) floating_object;
      GList  *layout_list = nullptr;

      if (_filter)
      {
        layout_list = _filter->GetLayoutList ();
      }

      while (layout_list)
      {
        Filter::Layout       *attr_layout = (Filter::Layout *) layout_list->data;
        Attribute            *attr;
        Player::AttributeId  *attr_id;

        attr_id = Player::AttributeId::Create (attr_layout->_desc, this);
        attr = player->GetAttribute (attr_id);
        attr_id->Release ();

        if (attr)
        {
          gchar *image = attr->GetUserImage (attr_layout->_look);

          string = g_string_append (string,
                                    image);
          string = g_string_append (string,
                                    "  ");
          g_free (image);
        }
        layout_list = g_list_next (layout_list);
      }
    }

    return string;
  }

  // --------------------------------------------------------------------------------
  void Allocator::RefreshDisplay ()
  {
    if (_main_table && (Locked () == FALSE))
    {
      SetCursor (GDK_WATCH);

      DeletePools ();
      CreatePools ();
      Display ();
      SignalStatusUpdate ();
      MakeDirty ();

      ResetCursor ();
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::Display ()
  {
    Stage::Display ();

    SetUpCombobox ();

    {
      gboolean adjustement_allowed = (_contest->IsTeamEvent () == FALSE) && (GetId () == 1);

      gtk_widget_set_visible (_glade->GetWidget ("latecomer_toolbutton"), adjustement_allowed);
      gtk_widget_set_visible (_glade->GetWidget ("absent_toolbutton"),    adjustement_allowed);
    }

    {
      OnFencerListToggled (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("fencer_list"))));

      PopulateFencerList ();
    }

    if (_main_table)
    {
      goo_canvas_item_remove (_main_table);
    }

    _max_w = 0;
    _max_h = 0;

    {
      GooCanvasItem *root = GetRootItem ();

      if (root)
      {
        _main_table = goo_canvas_group_new (root,
                                            NULL);

        for (GSList *current = _drop_zones; current; current = g_slist_next (current))
        {
          FillPoolTable ((PoolZone *) current->data);

        }
      }
    }
    DisplaySwapperError ();
    FixUpTablesBounds ();
    SignalStatusUpdate ();
    SpreadJobs ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::Garnish ()
  {
    if (_drop_zones == nullptr)
    {
      Setup ();

      if (_best_config)
      {
        _selected_config = _best_config;

        CreatePools ();
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::SelectConfig (guint nb_pool)
  {
    for (GSList *c = _config_list; c; c = g_slist_next (c))
    {
      Configuration *config = (Configuration *) c->data;

      if (config->_nb_pool == nb_pool)
      {
        _selected_config = config;
        break;
      }
    }

    SetUpCombobox ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::ApplyConfig ()
  {
    Module *next_stage = dynamic_cast <Module *> (GetNextStage ());

    if (next_stage)
    {
      GtkToggleButton *toggle = GTK_TOGGLE_BUTTON (next_stage->GetGObject ("balanced_radiobutton"));

      if (toggle)
      {
        guint seeding_is_balanced = gtk_toggle_button_get_active (toggle);

        if (_seeding_balanced->_value != seeding_is_balanced)
        {
          _seeding_balanced->_value = seeding_is_balanced;

          if (seeding_is_balanced)
          {
            _swapping_sensitivity_trigger->SwitchOn ();
          }
          else
          {
            _swapping_sensitivity_trigger->SwitchOff ();

            g_slist_free (_swapping_criteria_list);
            _swapping_criteria_list = nullptr;

            {
              GtkContainer *swapping_hbox = GTK_CONTAINER (_glade->GetGObject ("regular_swapping_hbox"));
              GList        *siblings      = gtk_container_get_children (swapping_hbox);

              while (siblings)
              {
                GtkToggleButton *togglebutton = GTK_TOGGLE_BUTTON (siblings->data);

                __gcc_extension__ g_signal_handlers_disconnect_by_func (G_OBJECT (togglebutton),
                                                                        (void *) OnSwappingToggled,
                                                                        (Object *) this);
                gtk_toggle_button_set_active (togglebutton,
                                              FALSE);
                g_signal_connect (G_OBJECT (togglebutton), "toggled",
                                  G_CALLBACK (OnSwappingToggled), this);

                siblings = g_list_next (siblings);
              }

              {
                GtkToggleButton *togglebutton = GTK_TOGGLE_BUTTON (_glade->GetWidget ("extra_toggle"));

                __gcc_extension__ g_signal_handlers_disconnect_by_func (G_OBJECT (togglebutton),
                                                                        (void *) OnSwappingToggled,
                                                                        (Object *) this);
                gtk_toggle_button_set_active (togglebutton,
                                              FALSE);
                g_signal_connect (G_OBJECT (togglebutton), "toggled",
                                  G_CALLBACK (OnSwappingToggled), this);

              }
            }
          }
          RecallJobs ();
          RefreshDisplay ();
          SpreadJobs ();
          return;
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::Recall ()
  {
    RecallJobs ();
    Stage::Recall ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::FillInConfig ()
  {
    Module *next_stage = dynamic_cast <Module *> (GetNextStage ());

    if (next_stage)
    {
      GtkToggleButton *w;

      if (_seeding_balanced->_value)
      {
        w = GTK_TOGGLE_BUTTON (next_stage->GetGObject ("balanced_radiobutton"));
      }
      else
      {
        w = GTK_TOGGLE_BUTTON (next_stage->GetGObject ("strength_radiobutton"));
      }

      if (w)
      {
        gtk_toggle_button_set_active (w,
                                      TRUE);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::LoadConfiguration (xmlNode *xml_node)
  {
    Stage::LoadConfiguration (xml_node);

    {
      Filter *filter = _fencer_list->GetFilter ();

      filter->Load (xml_node,
                    "Liste");
      filter->Release ();
    }

    if (_seeding_balanced)
    {
      _seeding_balanced->Load (xml_node);

      if (_seeding_balanced->_value == FALSE)
      {
        _swapping_sensitivity_trigger->SwitchOff ();
      }
    }

    if (_swapping)
    {
      _swapping->Load (xml_node);

      if (_swapping->GetString ())
      {
        gchar **tokens = g_strsplit_set (_swapping->GetString (),
                                         "/",
                                         0);

        if (tokens)
        {
          GtkContainer *swapping_hbox = GTK_CONTAINER (_glade->GetGObject ("regular_swapping_hbox"));
          GList        *siblings      = gtk_container_get_children (swapping_hbox);

          for (guint i = 0; tokens[i] != nullptr; i++)
          {
            if (*tokens[i] != '\0')
            {
              gboolean       is_regular = FALSE;
              AttributeDesc *attr_desc;

              for (GList *current = siblings; current; current = g_list_next (current))
              {
                GtkToggleButton *togglebutton = GTK_TOGGLE_BUTTON (current->data);

                attr_desc = (AttributeDesc *) g_object_get_data (G_OBJECT (togglebutton), "criteria_attribute");

                if (g_strcmp0 (tokens[i], attr_desc->_code_name) == 0)
                {
                  _swapping_criteria_list = g_slist_append (_swapping_criteria_list,
                                                            attr_desc);

                  gtk_toggle_button_set_active (togglebutton,
                                                TRUE);
                  is_regular = TRUE;
                  break;
                }
              }

              if (is_regular == FALSE)
              {
                GtkTreeModel *store         = GTK_TREE_MODEL (_glade->GetGObject ("swapping_liststore"));
                GtkTreeIter   iter;
                gboolean      iter_is_valid;

                iter_is_valid = gtk_tree_model_get_iter_first (store,
                                                               &iter);
                while (iter_is_valid)
                {
                  gtk_tree_model_get (store, &iter,
                                      SwappingColumn::CRITERIA, &attr_desc,
                                      -1);

                  if (g_strcmp0 (tokens[i], attr_desc->_code_name) == 0)
                  {
                    _swapping_criteria_list = g_slist_prepend (_swapping_criteria_list,
                                                               attr_desc);
                    gtk_combo_box_set_active_iter (GTK_COMBO_BOX (_glade->GetWidget ("extra_combobox")),
                                                   &iter);
                    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_glade->GetWidget ("extra_toggle")),
                                                  TRUE);
                    break;
                  }

                  iter_is_valid = gtk_tree_model_iter_next (store,
                                                            &iter);
                }
              }
            }
          }

          g_strfreev (tokens);
        }
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::SaveConfiguration (XmlScheme *xml_scheme)
  {
    Stage::SaveConfiguration (xml_scheme);

    {
      Filter *filter = _fencer_list->GetFilter ();

      filter->Save (xml_scheme,
                    "Liste");
      filter->Release ();
    }

    if (_seeding_balanced)
    {
      _seeding_balanced->Save (xml_scheme);
    }

    if (_swapping)
    {
      _swapping->Save (xml_scheme);
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::Load (xmlNode *xml_node)
  {
    static Pool     *current_pool     = nullptr;
    static PoolZone *current_zone     = nullptr;
    static guint     nb_pool          = 0;
    Stage           *next_stage       = GetNextStage ();
    StageClass      *next_stage_class = next_stage->GetClass ();

    for (xmlNode *n = xml_node; n != nullptr; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE)
      {
        if (   (_loaded == FALSE)
            && g_strcmp0 ((char *) n->name, next_stage_class->_xml_name) == 0)
        {
          LoadConfiguration (xml_node);

          {
            gchar *string = (gchar *) xmlGetProp (xml_node,
                                                  BAD_CAST "NbDePoules");
            if (string)
            {
              nb_pool = (guint) atoi (string);
              xmlFree (string);
            }
          }
          _loaded = TRUE;
        }
        else if (g_strcmp0 ((char *) n->name, "Poule") == 0)
        {
          if (_config_list == nullptr)
          {
            // Get the configuration
            {
              Setup ();
              SelectConfig (nb_pool);
            }

            // Assign players an original pool
            {
              CreatePools ();
              DeletePools ();
            }
          }

          {
            guint number = g_slist_length (_drop_zones);

            current_pool = new Pool (_max_score,
                                     number+1,
                                     GetXmlPlayerTag (),
                                     _rand_seed,
                                     GetNetID (),
                                     this,
                                     "competition", _contest->GetNetID (),
                                     "stage",       _parcel->GetNetID (),
                                     "batch",       _parcel->GetNetID (),
                                     NULL);
            current_pool->SetStrengthContributors (_selected_config->_size - _selected_config->_size%2);

            {
              current_zone = new PoolZone (this,
                                           current_pool);

              _drop_zones = g_slist_append (_drop_zones,
                                            current_zone);
            }
          }

          current_pool->Load (n);
        }
        else if (g_strcmp0 ((char *) n->name, GetXmlPlayerTag ()) == 0)
        {
          if (current_pool == nullptr)
          {
            LoadAttendees (n);
          }
          else
          {
            gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");

            if (attr)
            {
              Player *player = GetFencerFromRef (atoi (attr));

              if (player)
              {
                gchar *position_attr = (gchar *) xmlGetProp (n, BAD_CAST "NoDansLaPoule");
                gint  position       = -1;

                if (position_attr)
                {
                  position = g_ascii_strtoull (position_attr,
                                               nullptr,
                                               10);
                }
                current_pool->AddFencer (player,
                                         position);

                xmlFree (position_attr);
              }
              xmlFree (attr);
            }
          }
        }
        else if (g_strcmp0 ((char *) n->name, "Arbitre") == 0)
        {
          gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");

          if (attr)
          {
            Player *referee = _contest->GetRefereeFromRef (atoi (attr));

            if (referee)
            {
              Pool *pool = current_zone->GetPool ();

              pool->AddReferee (referee);
            }
            xmlFree (attr);
          }
        }
        else if (g_strcmp0 ((char *) n->name, "Match") == 0)
        {
          current_pool->CreateMatchs (_swapping_criteria_list);
          current_pool->Load (n,
                              GetShortList ());
          current_pool = nullptr;
          return;
        }
        else
        {
          current_pool = nullptr;

          _swapper->Configure (_drop_zones,
                               _swapping_criteria_list);
          _swapper->CheckCurrentDistribution ();
          return;
        }
      }

      Load (n->children);
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::SaveHeader (XmlScheme *xml_scheme)
  {
    Stage      *next_stage       = GetNextStage ();
    StageClass *next_stage_class = next_stage->GetClass ();

    xml_scheme->StartElement (next_stage_class->_xml_name);

    SaveConfiguration (xml_scheme);

    if (_drop_zones)
    {
      xml_scheme->WriteFormatAttribute ("NbDePoules",
                                        "%d", g_slist_length (_drop_zones));
    }
    xml_scheme->WriteFormatAttribute ("PhaseSuivanteDesQualifies",
                                      "%d", GetId ()+2);
  }

  // --------------------------------------------------------------------------------
  void Allocator::Save (XmlScheme *xml_scheme)
  {
    SaveHeader (xml_scheme);

    Stage::SaveAttendees (xml_scheme);

    for (GSList *current = _drop_zones; current; current = g_slist_next (current))
    {
      Pool *pool = GetPoolOf (current);

      pool->Save (xml_scheme);
    }

    xml_scheme->EndElement ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::RegisterConfig (Configuration *config)
  {
    if (config)
    {
      GtkTreeIter iter;

      _config_list = g_slist_append (_config_list,
                                     config);
      gtk_list_store_append (_combobox_store, &iter);

      {
        gchar *nb_pool_text = g_strdup_printf ("%d", config->_nb_pool);

        gtk_list_store_set (_combobox_store, &iter,
                            ComboboxColumn::NB_POOLS, nb_pool_text,
                            -1);
        g_free (nb_pool_text);
      }

      {
        gchar *pool_size_text;

        if (config->_nb_overloaded)
        {
          pool_size_text = g_strdup_printf ("%d %s %d", config->_size, gettext ("or"), config->_size+1);
        }
        else
        {
          pool_size_text = g_strdup_printf ("%d", config->_size);
        }

        gtk_list_store_set (_combobox_store, &iter,
                            ComboboxColumn::POOL_SIZE, pool_size_text,
                            -1);
        g_free (pool_size_text);
      }

      if (   (config->_size < 7)
          || ((config->_size == 7) && (config->_nb_overloaded == 0)))
      {
        _best_config = config;
      }

      config = nullptr;
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::Setup ()
  {
    ClearConfigurations ();

    {
      guint           nb_players    = g_slist_length (GetShortList ());
      Configuration  *config        = nullptr;
      guint           max_pool_size = Dispatcher::_MAX_POOL_SIZE;

      if (nb_players % Dispatcher::_MAX_POOL_SIZE)
      {
        max_pool_size = Dispatcher::_MAX_POOL_SIZE -1;
      }

      _best_config = nullptr;
      for (guint size = 2; size <= MIN (max_pool_size, nb_players); size++)
      {
        if (nb_players%size == 0)
        {
          config = (Configuration *) g_malloc (sizeof (Configuration));

          config->_nb_pool       = nb_players/size;
          config->_size          = size;
          config->_nb_overloaded = 0;

          RegisterConfig (config);
        }

        for (guint p = 1; p < nb_players/size; p++)
        {
          if ((nb_players - size*p) % (size + 1) == 0)
          {
            config = (Configuration *) g_malloc (sizeof (Configuration));

            config->_nb_pool       = p + (nb_players - size*p) / (size + 1);
            config->_size          = size;
            config->_nb_overloaded = (nb_players - size*p) / (size+1);

            RegisterConfig (config);
            break;
          }
        }
      }
    }

    if (_best_config)
    {
      GtkTreeIter iter;
      gint        best_index;

      best_index = g_slist_index (_config_list,
                                  _best_config);

      if (gtk_tree_model_iter_nth_child (GTK_TREE_MODEL (_combobox_store),
                                         &iter,
                                         nullptr,
                                         best_index))
      {
        gtk_list_store_set (_combobox_store, &iter,
                            ComboboxColumn::BEST_PIXMAP, GTK_STOCK_ABOUT,
                            -1);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::PopulateFencerList ()
  {
    GSList *current_player = GetShortList ();

    _fencer_list->Wipe ();

    while (current_player)
    {
      _fencer_list->Add ((Player *) current_player->data);
      current_player = g_slist_next (current_player);
    }
    _fencer_list->OnAttrListUpdated ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::CreatePools ()
  {
    GtkWidget *swapping_hbox = _glade->GetWidget ("swapping_hbox");

    gtk_widget_set_tooltip_text (swapping_hbox,
                                 "");

    if (_selected_config)
    {
      Pool   **pool_table;
      GSList  *short_list = GetShortList ();
      guint    nb_fencer  = g_slist_length (short_list);
      guint    nb_pool    = _selected_config->_nb_pool;

      pool_table = g_new (Pool *, nb_pool);
      for (guint i = 0; i < nb_pool; i++)
      {
        pool_table[i] = new Pool (_max_score,
                                  i+1,
                                  GetXmlPlayerTag (),
                                  _rand_seed,
                                  GetNetID (),
                                  this,
                                  "competition", _contest->GetNetID (),
                                  "stage",       _parcel->GetNetID (),
                                  "batch",       _parcel->GetNetID (),
                                  NULL);
        pool_table[i]->SetStrengthContributors (_selected_config->_size - _selected_config->_size%2);

        {
          PoolZone *zone = new PoolZone (this,
                                         pool_table[i]);

          _drop_zones = g_slist_append (_drop_zones,
                                        zone);
        }
      }

      _swapper->Configure (_drop_zones,
                           _swapping_criteria_list);
      if (_seeding_balanced->_value)
      {
        _swapper->Swap (short_list);
      }
      else
      {
        Pool *pool = pool_table[0];

        for (guint i = 0; i < nb_fencer; i++)
        {
          Player *player = (Player *) g_slist_nth_data (short_list,
                                                        i);

          if (_selected_config->_nb_overloaded)
          {
            if (pool->GetNumber () <= _selected_config->_nb_overloaded)
            {
              if (pool->GetNbPlayers () >= _selected_config->_size+1)
              {
                pool = pool_table[pool->GetNumber ()];
              }
            }
            else if (pool->GetNbPlayers () >= _selected_config->_size)
            {
              pool = pool_table[pool->GetNumber ()];
            }
          }
          else if (pool->GetNbPlayers () >= _selected_config->_size)
          {
            pool = pool_table[pool->GetNumber ()];
          }

          player->SetData (this,
                           "original_pool",
                           (void *) pool->GetNumber ());
          player->RemoveData (this,
                              "status_item");
#ifdef DEBUG
          player->RemoveData (this,
                              "swapped_from");
#endif
          pool->AddFencer (player);
        }
      }
      g_free (pool_table);
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnPartnerJoined (Net::Partner *partner,
                                   gboolean      joined)
  {
    _has_marshaller = joined;
    SignalStatusUpdate ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::DisplaySwapperError ()
  {
    if (_swapper->HasErrors ())
    {
      GdkColor *gdk_color = g_new (GdkColor, 1);

      gdk_color_parse ("#ffb245", gdk_color);

      gtk_widget_modify_bg (_glade->GetWidget ("swapping_box"),
                            GTK_STATE_NORMAL,
                            gdk_color);
      g_free (gdk_color);
    }
    else
    {
      gtk_widget_modify_bg (_glade->GetWidget ("swapping_box"),
                            GTK_STATE_NORMAL,
                            nullptr);
    }


    {
      gchar *tooltip = g_strdup_printf ("%d error(s)",
                                        _swapper->GetMoved ());

      gtk_widget_set_tooltip_text (_glade->GetWidget ("swapping_box"),
                                   tooltip);
      gtk_widget_set_tooltip_text (_glade->GetWidget ("swapping_criteria_hbox"),
                                   tooltip);
      g_free (tooltip);
    }
  }

  // --------------------------------------------------------------------------------
  guint Allocator::GetBiggestPoolSize ()
  {
    if (_selected_config)
    {
      if (_selected_config->_nb_overloaded)
      {
        return _selected_config->_size + 1;
      }
      return _selected_config->_size;
    }

    return 0;
  }

  // --------------------------------------------------------------------------------
  gint Allocator::GetNbMatchs ()
  {
    gint nb_matchs = 0;

    if (_selected_config)
    {
      nb_matchs  = _selected_config->_size * (_selected_config->_size - 1) / 2;
      nb_matchs *= _selected_config->_nb_pool - _selected_config->_nb_overloaded;
      nb_matchs += _selected_config->_nb_overloaded * _selected_config->_size * (_selected_config->_size + 1) / 2;
    }

    return nb_matchs;
  }

  // --------------------------------------------------------------------------------
  void Allocator::SetUpCombobox ()
  {
    if (_selected_config)
    {
      GtkWidget *w;
      gint       config_index;

      config_index = g_slist_index (_config_list,
                                    _selected_config);

      w = _glade->GetWidget ("nb_pools_combobox");
      __gcc_extension__ g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                                              (void *) on_nb_pools_combobox_changed,
                                                              (Object *) this);
      gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                                config_index);
      g_signal_connect (G_OBJECT (w), "changed",
                        G_CALLBACK (on_nb_pools_combobox_changed),
                        (Object *) this);

      w = _glade->GetWidget ("pool_size_combobox");
      __gcc_extension__ g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                                              (void *) on_pool_size_combobox_changed,
                                                              (Object *) this);
      gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                                config_index);
      g_signal_connect (G_OBJECT (w), "changed",
                        G_CALLBACK (on_pool_size_combobox_changed),
                        (Object *) this);
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::FixUpTablesBounds ()
  {
    {
      for (GSList *current_zone = _drop_zones; current_zone; current_zone = g_slist_next (current_zone))
      {
        Pool *pool = GetPoolOf (current_zone);

        for (GSList *current_fencer = pool->GetFencerList (); current_fencer; current_fencer = g_slist_next (current_fencer))
        {
          Player        *fencer      = (Player *) current_fencer->data;
          GooCanvasItem *status_item = (GooCanvasItem *) fencer->GetPtrData (this, "status_item");

          if (_swapper->IsAnError (fencer))
          {
            g_object_set (G_OBJECT (status_item),
                          "pixbuf", _warning_pixbuf,
                          NULL);
          }
          else if (fencer->GetUIntData (this, "original_pool") != pool->GetNumber ())
          {
            g_object_set (G_OBJECT (status_item),
                          "pixbuf", _moved_pixbuf,
                          NULL);
          }
          else
          {
            g_object_set (G_OBJECT (status_item),
                          "pixbuf", NULL,
                          NULL);
          }
        }
      }
    }
    if (_selected_config)
    {
      for (GSList *current = _drop_zones; current; current = g_slist_next (current))
      {
        PoolZone        *zone  = (PoolZone *) current->data;
        Pool            *pool  = GetPoolOf (current);
        GooCanvasItem   *table = (GooCanvasItem *) zone->GetPtrData (this, "table");
        GooCanvasBounds  bounds;

        {
          guint nb_columns = 2;
          guint nb_rows    = _selected_config->_nb_pool / nb_columns;

          if (_selected_config->_nb_pool % nb_columns != 0)
          {
            nb_rows++;
          }

          {
            guint row    = (pool->GetNumber () - 1) % nb_rows;
            guint column = (pool->GetNumber () - 1) / nb_rows;

            goo_canvas_item_get_bounds (table,
                                        &bounds);
            goo_canvas_item_translate (table,
                                       -bounds.x1,
                                       -bounds.y1);
            goo_canvas_item_translate (table,
                                       column * (_max_w + 40.0),
                                       row * (_max_h + 20.0));
          }
        }

        goo_canvas_item_get_bounds (table,
                                    &bounds);

        zone->Redraw (bounds.x1,
                      bounds.y1,
                      bounds.x2 - bounds.x1,
                      bounds.y2 - bounds.y1);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::FillPoolTable (PoolZone *zone)
  {
    if ((zone == nullptr) || (_main_table == nullptr))
    {
      return;
    }

    GooCanvasItem *item;
    GooCanvasItem *table       = (GooCanvasItem *) zone->GetPtrData (this, "table");
    GList         *layout_list = nullptr;
    Pool          *pool        = zone->GetPool ();

    if (_filter)
    {
      layout_list = _filter->GetLayoutList ();
    }

    if (table)
    {
      goo_canvas_item_remove (table);
    }

    {
      table = goo_canvas_table_new (_main_table,
                                    "row-spacing",    2.0,
                                    "column-spacing", 4.0,
                                    NULL);
      zone->SetData (this, "table",
                     table);
    }

    {
      guint          pool_size    = pool->GetNbPlayers ();
      guint          column_count = 0;
      GooCanvasItem *header_table = goo_canvas_table_new (table,
                                                          "column-spacing", 8.0,
                                                          NULL);

      // Status icon
      if (_selected_config)
      {
        gchar *icon_name;

        if (   (pool_size == _selected_config->_size)
            || (   _selected_config->_nb_overloaded
                && ((pool_size == _selected_config->_size) || (pool_size == _selected_config->_size + 1))))
        {
          icon_name = g_strdup (GTK_STOCK_APPLY);
          pool->SetData (this, "is_balanced", (void *) 1);
        }
        else
        {
          icon_name = g_strdup (GTK_STOCK_DIALOG_ERROR);
          pool->SetData (this, "is_balanced", nullptr);
        }

        Canvas::PutStockIconInTable (header_table,
                                     icon_name,
                                     0, column_count++);
        g_free (icon_name);
      }

      // Name
      {
        item = Canvas::PutTextInTable (header_table,
                                       pool->GetName (),
                                       0, column_count++);
        g_object_set (G_OBJECT (item),
                      "font", BP_FONT "bold 18px",
                      NULL);
        Canvas::SetTableItemAttribute (item, "y-align", 1.0);
      }

      // Strength
#ifdef DEBUG_SWAPPING
      {
        gchar *strength = g_strdup_printf ("%d", pool->GetStrength ());

        item = Canvas::PutTextInTable (header_table,
                                       strength,
                                       0, column_count++);
        g_object_set (G_OBJECT (item),
                      "font", BP_FONT "Bold Italic 14px",
                      "fill_color", "blue",
                      NULL);
        g_free (strength);
      }
#endif

      // Piste
      {
        guint piste = pool->GetPiste ();

        if (piste)
        {
          gchar *label = g_strdup_printf ("%s%c%c%d",
                                          gettext ("Piste"),
                                          0xC2, 0xA0, // non breaking space
                                          piste);

          item = Canvas::PutTextInTable (header_table,
                                         label,
                                         0, column_count++);
          g_object_set (G_OBJECT (item),
                        "font",       BP_FONT "bold italic 14px",
                        "fill-color", "DarkGreen",
                        NULL);
          Canvas::SetTableItemAttribute (item, "y-align", 1.0);
          g_free (label);
        }
      }

      // Time
      {
        FieTime *start_time = pool->GetStartTime ();

        if (start_time)
        {
          item = Canvas::PutTextInTable (header_table,
                                         start_time->GetImage (),
                                         0, column_count++);
          g_object_set (G_OBJECT (item),
                        "font",       BP_FONT "bold italic 14px",
                        "fill-color", "DarkGreen",
                        NULL);
          Canvas::SetTableItemAttribute (item, "y-align", 1.0);
        }
      }

      Canvas::PutInTable (table,
                          header_table,
                          0, 0);
      Canvas::SetTableItemAttribute (header_table,
                                     "columns", g_list_length (layout_list) + column_count);

      //g_object_set (G_OBJECT (header_table),
                    //"horz-grid-line-width", 2.0,
                    //"vert-grid-line-width", 2.0,
                    //NULL);
      //g_object_set (G_OBJECT (table),
                    //"horz-grid-line-width", 2.0,
                    //"vert-grid-line-width", 2.0,
                    //NULL);
    }

    if (layout_list)
    {
      guint p = 0;

      // Referees
      for (GSList *current = pool->GetRefereeList (); current; current = g_slist_next (current))
      {
        Player *player = (Player *) current->data;

        DisplayPlayer (player,
                       p,
                       table,
                       zone,
                       layout_list);
        p++;
      }

      // Fencers
      for (GSList *current = pool->GetFencerList (); current; current = g_slist_next (current))
      {
        Player *player = (Player *) current->data;

        DisplayPlayer (player,
                       p,
                       table,
                       zone,
                       layout_list);
        p++;
      }
    }

    {
      GooCanvasBounds bounds;

      goo_canvas_item_get_bounds (table,
                                  &bounds);

      if ((bounds.x2 - bounds.x1) > _max_w)
      {
        _max_w = bounds.x2 - bounds.x1;
      }
      if ((bounds.y2 - bounds.y1) > _max_h)
      {
        _max_h = bounds.y2 - bounds.y1;
      }
    }

    zone->Draw (GetRootItem ());
  }

  // --------------------------------------------------------------------------------
  void Allocator::DisplayPlayer (Player        *player,
                                 guint          indice,
                                 GooCanvasItem *table,
                                 PoolZone      *zone,
                                 GList         *layout_list)
  {
    if (player)
    {
      if (player->Is ("Referee"))
      {
        static gchar  *referee_icon = nullptr;

        if (referee_icon == nullptr)
        {
          referee_icon = g_build_filename (Global::_share_dir, "resources/glade/images/referee.png", NULL);
        }
        Canvas::PutIconInTable (table,
                                referee_icon,
                                indice+1, 0);
      }
      else
      {
#ifdef DEBUG
        {
          guint swapped_from = player->GetUIntData (this, "swapped_from");

          if (swapped_from)
          {
            GooCanvasItem *item;
            gchar         *swapped_from_text = g_strdup_printf ("%02d", swapped_from);

            item = Canvas::PutTextInTable (table,
                                           swapped_from_text,
                                           indice+1, 0);
            g_object_set (G_OBJECT (item),
                          "font", BP_FONT "Bold Italic 14px",
                          "fill_color", "blue",
                          NULL);
            g_free (swapped_from_text);
          }
        }
#endif

        {
          GooCanvasItem *status_item = Canvas::PutPixbufInTable (table,
                                                                 _moved_pixbuf,
                                                                 indice+1, 1);
          player->SetData (this,
                           "status_item",
                           status_item);
        }

        if (player->GetUIntData (this, "injected"))
        {
          static gchar  *injected_icon = nullptr;

          if (injected_icon == nullptr)
          {
            injected_icon = g_build_filename (Global::_share_dir, "resources/glade/images/plus.png", NULL);
          }
          Canvas::PutIconInTable (table,
                                  injected_icon,
                                  indice+1, 2);
        }
      }

      for (guint i = 0; layout_list != nullptr; i++)
      {
        GooCanvasItem        *item;
        Filter::Layout       *attr_layout = (Filter::Layout *) layout_list->data;
        Attribute            *attr;
        Player::AttributeId  *attr_id;

        attr_id = Player::AttributeId::Create (attr_layout->_desc, this);
        attr = player->GetAttribute (attr_id);
        attr_id->Release ();

        item = nullptr;
        if (attr)
        {
          GdkPixbuf *pixbuf = nullptr;

          if (attr_layout->_look == AttributeDesc::GRAPHICAL)
          {
            pixbuf = attr->GetPixbuf ();
          }

          if (pixbuf)
          {
            item = Canvas::PutPixbufInTable (table,
                                             pixbuf,
                                             indice+1, i+3);
          }
          else
          {
            gchar *image = attr->GetUserImage (attr_layout->_look);

            if (image)
            {
              item = Canvas::PutTextInTable (table,
                                             image,
                                             indice+1, i+3);
              g_free (image);
            }
          }
        }

        if (item == nullptr)
        {
          item = Canvas::PutTextInTable (table,
                                         " ",
                                         indice+1, i+3);
        }

        if (item)
        {
          if (player->Is ("Referee"))
          {
            g_object_set (G_OBJECT (item),
                          "font", BP_FONT "Bold Italic 14px",
                          "fill_color", "black",
                          NULL);
          }
          else
          {
            g_object_set (G_OBJECT (item),
                          "font", BP_FONT "14px",
                          "fill_color", "black",
                          NULL);

            SetObjectDropZone (player,
                               item,
                               zone);
          }
        }

        layout_list = g_list_next (layout_list);
      }
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::DeletePools ()
  {
    if (_drop_zones)
    {
      for (GSList *current = _drop_zones; current; current = g_slist_next (current))
      {
        Pool *pool = GetPoolOf (current);

        Object::TryToRelease ((PoolZone *) current->data);
        Object::TryToRelease (pool);
      }

      g_slist_free (_drop_zones);
      _drop_zones = nullptr;
    }

    CanvasModule::Wipe ();
    _main_table = nullptr;
  }

  // --------------------------------------------------------------------------------
  void Allocator::Reset ()
  {
    DeletePools ();
    ClearConfigurations ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::ClearConfigurations ()
  {
    {
      __gcc_extension__ g_signal_handlers_disconnect_by_func (_glade->GetWidget ("pool_size_combobox"),
                                                              (void *) on_pool_size_combobox_changed,
                                                              (Object *) this);
      __gcc_extension__ g_signal_handlers_disconnect_by_func (_glade->GetWidget ("nb_pools_combobox"),
                                                              (void *) on_nb_pools_combobox_changed,
                                                              (Object *) this);

      gtk_list_store_clear (_combobox_store);

      g_signal_connect (_glade->GetWidget ("pool_size_combobox"), "changed",
                        G_CALLBACK (on_pool_size_combobox_changed),
                        (Object *) this);
      g_signal_connect (_glade->GetWidget ("nb_pools_combobox"), "changed",
                        G_CALLBACK (on_nb_pools_combobox_changed),
                        (Object *) this);
    }

    if (_config_list)
    {
      g_slist_free_full (_config_list,
                         (GDestroyNotify) g_free);
      _config_list     = nullptr;
      _selected_config = nullptr;
    }
  }

  // --------------------------------------------------------------------------------
  PoolZone *Allocator::GetPoolOf (Player *of)
  {
    for (GSList *current = _drop_zones; current; current = g_slist_next (current))
    {
      Pool   *pool    = GetPoolOf (current);
      GSList *fencers = pool->GetFencerList ();

      if (g_slist_find (fencers, of))
      {
        return (PoolZone *) current->data;
      }
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  gboolean Allocator::IsOver ()
  {
    for (GSList *current = _drop_zones; current; current = g_slist_next (current))
    {
      Pool *pool = GetPoolOf (current);

      if (pool->GetUIntData (this, "is_balanced") == 0)
      {
        return FALSE;
      }
    }

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  Error *Allocator::GetError ()
  {
    for (GSList *current = _drop_zones; current; current = g_slist_next (current))
    {
      Pool *pool = GetPoolOf (current);

      if (pool->GetUIntData (this, "is_balanced") == 0)
      {
        return new Error (Error::Level::MAJOR,
                          pool->GetName (),
                          gettext ("Wrong fencers count!"));
      }
    }

    for (GSList *current = _drop_zones; current; current = g_slist_next (current))
    {
      Pool *pool = GetPoolOf (current);

      if (_has_marshaller)
      {
        if (   (pool->GetPiste ()       == 0)
            || (pool->GetRefereeList () == nullptr))
        {
          return new Error (Error::Level::WARNING,
                            nullptr,
                            gettext ("Referees allocation \n ongoing"));
        }
      }
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  gchar *Allocator::GetPrintName ()
  {
    return g_strdup_printf ("%s %s", gettext ("Allocation"), GetName ());
  }

  // --------------------------------------------------------------------------------
  guint Allocator::PreparePrint (GtkPrintOperation *operation,
                                 GtkPrintContext   *context)
  {
    if (GetStageView (operation) == StageView::CLASSIFICATION)
    {
      return 0;
    }
    else
    {
      gdouble canvas_w;
      gdouble paper_w  = gtk_print_context_get_width (context);
      gdouble paper_h  = gtk_print_context_get_height (context);
      gdouble header_h = GetPrintHeaderSize (context, SizeReferential::ON_SHEET);

      {
        GooCanvasBounds bounds;

        goo_canvas_item_get_bounds (GetRootItem (),
                                    &bounds);
        canvas_w = bounds.x2 - bounds.x1;
      }

      {
        gdouble canvas_dpi;
        gdouble printer_dpi;

        g_object_get (G_OBJECT (GetCanvas ()),
                      "resolution-x", &canvas_dpi,
                      NULL);
        printer_dpi = gtk_print_context_get_dpi_x (context);

        _print_scale = printer_dpi/canvas_dpi;
      }

      if (canvas_w * _print_scale > paper_w)
      {
        _print_scale = paper_w / canvas_w;
      }

      {
        guint nb_row_by_page = (guint) ((paper_h - header_h) / ((_max_h+20)*_print_scale));
        guint nb_row         = g_slist_length (_drop_zones)/2;

        if (g_slist_length (_drop_zones) % 2 > 0)
        {
          nb_row++;
        }

        _nb_page = nb_row/nb_row_by_page;
        _page_h  = nb_row_by_page * (_max_h+20)*_print_scale;

        if (nb_row % nb_row_by_page != 0)
        {
          _nb_page++;
        }
      }
    }

    return _nb_page;
  }

  // --------------------------------------------------------------------------------
  void Allocator::DrawPage (GtkPrintOperation *operation,
                            GtkPrintContext   *context,
                            gint               page_nr)
  {
    gdouble          paper_w = gtk_print_context_get_width  (context);
    cairo_t         *cr      = gtk_print_context_get_cairo_context (context);
    GooCanvasBounds  bounds;

    DrawContainerPage (operation,
                       context,
                       page_nr);

    {
      GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);
      char      *text   = g_strdup_printf ("Page %d/%d", page_nr+1, _nb_page);

      goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                           text,
                           98.0, -2.0,
                           -1.0,
                           GTK_ANCHOR_SE,
                           "fill-color", "black",
                           "font", BP_FONT "Bold 2px", NULL);
      g_free (text);

      goo_canvas_render (canvas,
                         gtk_print_context_get_cairo_context (context),
                         nullptr,
                         1.0);

      gtk_widget_destroy (GTK_WIDGET (canvas));
    }

    cairo_save (cr);

    cairo_scale (cr,
                 _print_scale,
                 _print_scale);

    bounds.x1 = 0.0;
    bounds.y1 = (page_nr*_page_h)/_print_scale;
    bounds.x2 = paper_w/_print_scale;
    bounds.y2 = ((page_nr+1)*_page_h)/_print_scale;

    cairo_translate (cr,
                     0.0,
                     -(_page_h*page_nr)/_print_scale);

    goo_canvas_render (GetCanvas (),
                       cr,
                       &bounds,
                       1.0);

    cairo_restore (cr);
  }

  // --------------------------------------------------------------------------------
  void Allocator::DrawConfig (GtkPrintOperation *operation,
                              GtkPrintContext   *context,
                              gint               page_nr)
  {
    {
      gchar *text;

      if (_seeding_balanced->_value)
      {
        text = g_strdup_printf ("%s : %s",
                                gettext ("Seeding"),
                                gettext ("Balanced"));
      }
      else
      {
        text = g_strdup_printf ("%s : %s",
                                gettext ("Seeding"),
                                gettext ("Grouped by strength"));
      }

      DrawConfigLine (operation,
                      context,
                      text);

      g_free (text);
    }

    {
      gchar *text = g_strdup_printf ("%s : %d",
                                     gettext ("Max score"),
                                     _max_score->_value);

      DrawConfigLine (operation,
                      context,
                      text);

      g_free (text);
    }

    {
      gchar *text;

      if (_nb_qualified->IsValid ())
      {
        text = g_strdup_printf ("%s : %d",
                                gettext ("Qualified"),
                                _nb_qualified->_value);
      }
      else
      {
        text = g_strdup_printf ("%s : %s",
                                gettext ("Qualified"),
                                "100%");
      }

      DrawConfigLine (operation,
                      context,
                      text);

      g_free (text);
    }
  }

  // --------------------------------------------------------------------------------
  Data *Allocator::GetMaxScore ()
  {
    return _max_score;
  }

  // --------------------------------------------------------------------------------
  gboolean Allocator::SeedingIsBalanced ()
  {
    return (_seeding_balanced->_value == 1);
  }

  // --------------------------------------------------------------------------------
  guint Allocator::GetNbPools ()
  {
    return g_slist_length (_drop_zones);
  }

  // --------------------------------------------------------------------------------
  Pool *Allocator::GetPool (guint index)
  {
    return GetPoolOf (g_slist_nth (_drop_zones,
                                   index));
  }

  // --------------------------------------------------------------------------------
  Pool *Allocator::GetPoolOf (GSList *drop_zone)
  {
    if (drop_zone)
    {
      PoolZone *zone = (PoolZone *) drop_zone->data;

      return zone->GetPool ();
    }

    return nullptr;
  }

  // --------------------------------------------------------------------------------
  PoolZone *Allocator::GetZone (guint index)
  {
    GSList *zone = g_slist_nth (_drop_zones,
                                index);

    return (PoolZone *) zone->data;
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_pool_size_combobox_changed (GtkWidget *widget,
                                                                 Object    *owner)
  {
    Allocator *pl = dynamic_cast <Allocator *> (owner);

    pl->OnComboboxChanged (GTK_COMBO_BOX (widget));
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnComboboxChanged (GtkComboBox *cb)
  {
    gint config_index     = gtk_combo_box_get_active (cb);
    Configuration *config = (Configuration *) g_slist_nth_data (_config_list,
                                                                config_index);

    if (config)
    {
      {
        GtkWidget *w;

        w = _glade->GetWidget ("nb_pools_combobox");
        __gcc_extension__ g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                                                (void *) on_nb_pools_combobox_changed,
                                                                (Object *) this);
        gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                                  config_index);
        g_signal_connect (G_OBJECT (w), "changed",
                          G_CALLBACK (on_nb_pools_combobox_changed),
                          (Object *) this);

        w = _glade->GetWidget ("pool_size_combobox");
        __gcc_extension__ g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                                                (void *) on_pool_size_combobox_changed,
                                                                (Object *) this);
        gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                                  config_index);
        g_signal_connect (G_OBJECT (w), "changed",
                          G_CALLBACK (on_pool_size_combobox_changed),
                          (Object *) this);
      }

      _selected_config = config;

      RecallJobs ();
      RefreshDisplay ();
      SpreadJobs ();
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnPoolRoadmap (Pool         *pool,
                                 Net::Message *message)
  {
    PoolZone *zone = (PoolZone *) g_slist_nth_data (_drop_zones,
                                                    pool->GetNumber () - 1);

    {
      pool->RemoveAllReferee ();

      if (message->GetFitness () > 0)
      {
        gsize   length;
        guint  *ref     = message->GetIntegerList ("referees", &length);

        for (guint i = 0; i < length; i++)
        {
          Player *referee = (Player *) _contest->GetRefereeFromRef (ref[i]);

          if (referee)
          {
            pool->AddReferee (referee);
          }
        }
      }

      for (GSList *current = pool->GetFencerList (); current; current = g_slist_next (current))
      {
        Player *fencer = (Player *) current->data;

        _fencer_list->Update (fencer);
      }
    }

    MakeDirty ();
    SignalStatusUpdate ();

    FillPoolTable (zone);
    FixUpTablesBounds ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnPrintClicked ()
  {
    gchar *title = GetPrintName ();

    if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("fencer_list"))))
    {
      _fencer_list->Print (title);
    }
    else
    {
      Print (title);
    }
    g_free (title);
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnNewLatecomerClicked ()
  {
    _latecomer_dialog->DisplayForm ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnLatecomerClicked ()
  {
    GtkWidget *dialog = _glade->GetWidget ("latecomer_dialog");

    _latecomer_dialog->Populate (_attendees->GetAbsents (),
                                 _selected_config->_nb_pool);

    _latecomer_dialog->SetParentWindow (GTK_WINDOW (dialog));

    if (RunDialog (GTK_DIALOG (dialog)) == GTK_RESPONSE_CANCEL)
    {
      _latecomer_dialog->RevertAll ();
    }
    gtk_widget_hide (dialog);

    _latecomer_dialog->Clear ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnAbsentClicked ()
  {
    GtkWidget *dialog = _glade->GetWidget ("absent_dialog");

    _absent_dialog->Populate (_attendees->GetPresents (),
                              _selected_config->_nb_pool);

    if (RunDialog (GTK_DIALOG (dialog)) == GTK_RESPONSE_CANCEL)
    {
      _absent_dialog->RevertAll ();
    }
    gtk_widget_hide (dialog);

    _absent_dialog->Clear ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_nb_pools_combobox_changed (GtkWidget *widget,
                                                                Object    *owner)
  {
    Allocator *c = dynamic_cast <Allocator *> (owner);

    c->OnComboboxChanged (GTK_COMBO_BOX (widget));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_print_toolbutton_clicked (GtkWidget *widget,
                                                               Object    *owner)
  {
    Allocator *pa = dynamic_cast <Allocator *> (owner);

    pa->OnPrintClicked ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnLocked ()
  {
    DisableSensitiveWidgets ();

    for (GSList *current = _drop_zones; current; current = g_slist_next (current))
    {
      Pool *pool = GetPoolOf (current);

      pool->CreateMatchs (_swapping_criteria_list);
    }
  }

  // --------------------------------------------------------------------------------
  GSList *Allocator::GetCurrentClassification ()
  {
    return g_slist_copy (GetShortList ());
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnUnLocked ()
  {
    EnableSensitiveWidgets ();
    SpreadJobs ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnAttrListUpdated ()
  {
    _max_w = 0;
    _max_h = 0;

    for (GSList *current = _drop_zones; current; current = g_slist_next (current))
    {
      FillPoolTable ((PoolZone *) current->data);
    }
    FixUpTablesBounds ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnFilterClicked ()
  {
    if (gtk_toggle_tool_button_get_active (GTK_TOGGLE_TOOL_BUTTON (_glade->GetWidget ("fencer_list"))))
    {
      _fencer_list->SelectAttributes ();
    }
    else
    {
      SelectAttributes ();
    }
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnSwappingToggled (GtkToggleButton *togglebutton,
                                     Allocator       *allocator)
  {
    if (allocator->_contest->GetState () == State::OPERATIONAL)
    {
      GtkWidget *parent   = allocator->_glade->GetWidget ("regular_swapping_hbox");
      GList     *siblings = gtk_container_get_children (GTK_CONTAINER (parent));
      GString   *string   = g_string_new ("/");

      g_slist_free (allocator->_swapping_criteria_list);
      allocator->_swapping_criteria_list = nullptr;

      // Regular swapping
      while (siblings)
      {
        GtkToggleButton *sibling_togglebutton = GTK_TOGGLE_BUTTON (siblings->data);

        if (gtk_toggle_button_get_active (sibling_togglebutton))
        {
          AttributeDesc *desc;

          desc = (AttributeDesc *) g_object_get_data (G_OBJECT (sibling_togglebutton), "criteria_attribute");

          allocator->_swapping_criteria_list = g_slist_append (allocator->_swapping_criteria_list,
                                                               desc);

          g_string_append_printf (string,
                                  "%s/",
                                  desc->_code_name);
        }

        siblings = g_list_next (siblings);
      }

      // Extra swapping
      {
        GtkToggleButton *extra_toggle = GTK_TOGGLE_BUTTON (allocator->_glade->GetWidget ("extra_toggle"));

        if (gtk_toggle_button_get_active (extra_toggle))
        {
          GtkComboBox *extra_combo  = GTK_COMBO_BOX (allocator->_glade->GetWidget ("extra_combobox"));
          GtkTreeIter  iter;

          if (gtk_combo_box_get_active_iter (extra_combo,
                                             &iter))
          {
            GtkTreeModel  *store = GTK_TREE_MODEL (allocator->_glade->GetGObject ("swapping_liststore"));
            AttributeDesc *desc;

            gtk_tree_model_get (store, &iter,
                                SwappingColumn::CRITERIA, &desc,
                                -1);

            allocator->_swapping_criteria_list = g_slist_prepend (allocator->_swapping_criteria_list,
                                                                  desc);

            g_string_append_printf (string,
                                    "%s/",
                                    desc->_code_name);
          }
        }
      }

      allocator->_swapping->SetString (string->str);
      g_string_free (string,
                     TRUE);

      allocator->RecallJobs ();
      allocator->RefreshDisplay ();
      allocator->SpreadJobs ();
    }
  }

  // --------------------------------------------------------------------------------
  gboolean Allocator::OnAttendeeToggled (PillowDialog *from,
                                         Player       *attendee,
                                         gboolean      attending)
  {
    PoolZone *zone;

    if (attending == FALSE)
    {
      zone = GetPoolOf (attendee);

      if (zone)
      {
        Pool *pool = zone->GetPool ();

        if (pool)
        {
          _attendees->Toggle (attendee);

          _fencer_list->Remove (attendee);

          pool->RemoveFencer (attendee);

          attendee->RemoveData (this,
                                "injected");

          if (from == _absent_dialog)
          {
            attendee->SetData (from,
                               from->GetRevertContext (),
                               (void *) pool->GetNumber ());
          }
        }
      }
      else
      {
        return FALSE;
      }
    }
    else
    {
      _attendees->Toggle (attendee);
      RetrieveAttendees ();

      _fencer_list->Add (attendee);

      if (from == _latecomer_dialog)
      {
        _swapper->InjectFencer (attendee);
        attendee->SetData (this,
                           "injected",
                           (void *) TRUE);
      }
      else
      {
        _swapper->InjectFencer (attendee,
                                attendee->GetUIntData (from,
                                                       from->GetRevertContext ()));
      }

      zone = GetPoolOf (attendee);
    }

    {
      _swapper->Configure (_drop_zones,
                           _swapping_criteria_list);
      _swapper->CheckCurrentDistribution ();

      DisplaySwapperError ();
    }

    {
      Setup ();
      SelectConfig (from->GetNbPools ());

      if (_selected_config == nullptr)
      {
        return FALSE;
      }
    }

    RecallJobs ();
    SpreadJobs ();

    {
      FillPoolTable (zone);
      FixUpTablesBounds ();
      SignalStatusUpdate ();
      MakeDirty ();
    }

    _fencer_list->Update (attendee);

    return TRUE;
  }

  // --------------------------------------------------------------------------------
  void Allocator::DumpToHTML (FILE *file)
  {
    _fencer_list->DumpToHTML (file);
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_filter_button_clicked (GtkWidget *widget,
                                                            Object    *owner)
  {
    Allocator *p = dynamic_cast <Allocator *> (owner);

    p->OnFilterClicked ();
  }

  // --------------------------------------------------------------------------------
  void Allocator::OnFencerListToggled (gboolean toggled)
  {
    GtkWidget *main_w        = _glade->GetWidget ("main_hook");
    GtkWidget *fencer_list_w = _glade->GetWidget ("fencer_list_hook");

    if (toggled)
    {
      if (main_w)
      {
        gtk_widget_hide (main_w);
      }
      gtk_widget_show (fencer_list_w);
    }
    else
    {
      gtk_widget_hide (fencer_list_w);
      if (main_w)
      {
        gtk_widget_show_all (main_w);
      }
    }
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_fencer_list_toggled (GtkToggleToolButton *widget,
                                                          Object              *owner)
  {
    Allocator *p = dynamic_cast <Allocator *> (owner);

    p->OnFencerListToggled (gtk_toggle_tool_button_get_active (widget));
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_latecomer_toolbutton_clicked (GtkToolButton *widget,
                                                                   Object        *owner)
  {
    Allocator *pa = dynamic_cast <Allocator *> (owner);

    pa->OnLatecomerClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_absent_toolbutton_clicked (GtkToolButton *widget,
                                                                Object        *owner)
  {
    Allocator *pa = dynamic_cast <Allocator *> (owner);

    pa->OnAbsentClicked ();
  }

  // --------------------------------------------------------------------------------
  extern "C" G_MODULE_EXPORT void on_new_latecomer_clicked (GtkToolButton *widget,
                                                            Object        *owner)
  {
    Allocator *pa = dynamic_cast <Allocator *> (owner);

    pa->OnNewLatecomerClicked ();
  }
}
