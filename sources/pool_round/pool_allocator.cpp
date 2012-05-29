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

#include <string.h>
#include <libxml/encoding.h>
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "green_swapper.hpp"
#include "contest.hpp"
#include "pool_match_order.hpp"

#include "pool_allocator.hpp"

#define VALUE_INIT {0,{{0}}}

typedef enum
{
  POOL_SIZE_COL,
  NB_POOLS_COL,
  BEST_PIXMAP_COL
} ComboboxColumn;

typedef enum
{
  SWAPPING_IMAGE,
  SWAPPING_CRITERIA
} SwappingColumn;

extern "C" G_MODULE_EXPORT void on_nb_pools_combobox_changed (GtkWidget *widget,
                                                              Object    *owner);
extern "C" G_MODULE_EXPORT void on_pool_size_combobox_changed (GtkWidget *widget,
                                                               Object    *owner);
extern "C" G_MODULE_EXPORT void on_swapping_combobox_changed (GtkWidget *widget,
                                                              Object    *owner);

const gchar *PoolAllocator::_class_name     = N_ ("Pools arrangement");
const gchar *PoolAllocator::_xml_class_name = "TourDePoules";

// --------------------------------------------------------------------------------
PoolAllocator::PoolAllocator (StageClass *stage_class)
  : Stage (stage_class),
  CanvasModule ("pool_allocator.glade")
{
  _drop_zones        = NULL;
  _config_list       = NULL;
  _selected_config   = NULL;
  _main_table        = NULL;
  _swapping_criteria = NULL;
  _loaded            = FALSE;
  _nb_matchs         = 0;

  _max_score = new Data ("ScoreMax",
                         5);

  _seeding_balanced = new Data ("RepartitionEquilibre",
                                TRUE);

  _swapping = new Data ("Decalage",
                        (gchar *) NULL);

  _combobox_store = GTK_LIST_STORE (_glade->GetObject ("combo_liststore"));

  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("nb_pools_combobox"));
    AddSensitiveWidget (_glade->GetWidget ("pool_size_combobox"));
    AddSensitiveWidget (_glade->GetWidget ("swapping_combobox"));

    _swapping_sensitivity_trigger.AddWidget (_glade->GetWidget ("swapping_combobox"));
  }

  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
#ifndef DEBUG
                               "ref",
#endif
                               "availability",
                               "participation_rate",
                               "level",
                               "status",
                               "global_status",
                               "start_rank",
                               "final_rank",
                               "attending",
                               "exported",
                               "victories_ratio",
                               "indice",
                               "pool_nr",
                               "HS",
                               "rank",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("previous_stage_rank");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("club");

    SetFilter (filter);
    filter->Release ();
  }

  {
    GtkListStore *swapping_store = GTK_LIST_STORE (_glade->GetObject ("swapping_liststore"));
    GtkTreeIter   iter;
    GSList       *attr           = _filter->GetAttrList ();

    gtk_list_store_append (swapping_store, &iter);
    gtk_list_store_set (swapping_store, &iter,
                        SWAPPING_IMAGE, "Aucun",
                        SWAPPING_CRITERIA, NULL,
                        -1);

    while (attr)
    {
      AttributeDesc *attr_desc;

      attr_desc = (AttributeDesc *) attr->data;
      if (attr_desc->_uniqueness == AttributeDesc::NOT_SINGULAR)
      {
        gtk_list_store_append (swapping_store, &iter);

        gtk_list_store_set (swapping_store, &iter,
                            SWAPPING_IMAGE, attr_desc->_user_name,
                            SWAPPING_CRITERIA, attr_desc,
                            -1);
      }

      attr = g_slist_next (attr);
    }

    gtk_combo_box_set_active (GTK_COMBO_BOX (_glade->GetObject ("swapping_combobox")),
                              0);
  }

  {
    _fencer_list = new PlayersList ("classification.glade",
                                    PlayersList::SORTABLE);
    {
      GSList *attr_list;
      Filter *filter;

      AttributeDesc::CreateList (&attr_list,
#ifndef DEBUG
                                 "ref",
#endif
                                 "availability",
                                 "participation_rate",
                                 "level",
                                 "status",
                                 "global_status",
                                 "start_rank",
                                 "final_rank",
                                 "attending",
                                 "exported",
                                 "victories_ratio",
                                 "indice",
                                 "HS",
                                 "rank",
                                 NULL);
      filter = new Filter (attr_list,
                           this);

      filter->ShowAttribute ("name");
      filter->ShowAttribute ("first_name");
      filter->ShowAttribute ("club");
      filter->ShowAttribute ("pool_nr");

      _fencer_list->SetFilter    (filter);
      _fencer_list->SetDataOwner (this);
      filter->Release ();
    }

    Plug (_fencer_list,
          _glade->GetWidget ("fencer_list_hook"));
  }
}

// --------------------------------------------------------------------------------
PoolAllocator::~PoolAllocator ()
{
  _max_score->Release        ();
  _swapping->Release         ();
  _seeding_balanced->Release ();
  _fencer_list->Release      ();
}

// --------------------------------------------------------------------------------
void PoolAllocator::Init ()
{
  RegisterStageClass (gettext (_class_name),
                      _xml_class_name,
                      CreateInstance);
}

// --------------------------------------------------------------------------------
Stage *PoolAllocator::CreateInstance (StageClass *stage_class)
{
  return new PoolAllocator (stage_class);
}

// --------------------------------------------------------------------------------
const gchar *PoolAllocator::GetInputProviderClient ()
{
  return "pool_stage";
}

// --------------------------------------------------------------------------------
void PoolAllocator::OnPlugged ()
{
  CanvasModule::OnPlugged ();

  SetDndDest (GTK_WIDGET (GetCanvas ()));
  EnableDragAndDrop ();
}

// --------------------------------------------------------------------------------
Object *PoolAllocator::GetDropObjectFromRef (guint32 ref)
{
  return _contest->GetRefereeFromRef (ref);
}

// --------------------------------------------------------------------------------
void PoolAllocator::DragObject (Object   *object,
                                DropZone *from_zone)
{
  Player   *floating_object = (Player *) object;
  PoolZone *pool_zone       = (PoolZone *) from_zone;

  if (floating_object->IsFencer ())
  {
    Pool *pool = pool_zone->GetPool ();

    pool->RemoveFencer (floating_object);
  }
  else
  {
    pool_zone->RemoveObject (floating_object);
  }

  FillPoolTable (pool_zone);
  SignalStatusUpdate ();
  FixUpTablesBounds ();
}

// --------------------------------------------------------------------------------
void PoolAllocator::DropObject (Object   *object,
                                DropZone *source_zone,
                                DropZone *target_zone)
{
  Player   *floating_object = (Player *) object;
  PoolZone *pool_zone       = NULL;

  if (floating_object->IsFencer ())
  {
    Pool *target_pool;

    if (target_zone)
    {
      pool_zone = (PoolZone *) target_zone;
    }
    else
    {
      pool_zone = (PoolZone *) source_zone;
    }

    target_pool = pool_zone->GetPool ();
    target_pool->AddFencer (floating_object,
                            this);
    _fencer_list->Update (floating_object);
  }
  else if (target_zone)
  {
    pool_zone = (PoolZone *) target_zone;

    pool_zone->AddObject (floating_object);
  }

  if (pool_zone)
  {
    FillPoolTable (pool_zone);
    FixUpTablesBounds ();
  }
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator::DroppingIsForbidden ()
{
  return Locked ();
}

// --------------------------------------------------------------------------------
GString *PoolAllocator::GetFloatingImage (Object *floating_object)
{
  GString *string = g_string_new ("");

  if (floating_object)
  {
    Player *player        = (Player *) floating_object;
    GSList *selected_attr = NULL;

    if (_filter)
    {
      selected_attr = _filter->GetSelectedAttrList ();
    }

    while (selected_attr)
    {
      AttributeDesc       *attr_desc = (AttributeDesc *) selected_attr->data;
      Attribute           *attr;
      Player::AttributeId *attr_id;

      attr_id = Player::AttributeId::CreateAttributeId (attr_desc, this);
      attr = player->GetAttribute (attr_id);
      attr_id->Release ();

      if (attr)
      {
        gchar *image = attr->GetUserImage ();

        string = g_string_append (string,
                                  image);
        string = g_string_append (string,
                                  "  ");
        g_free (image);
      }
      selected_attr = g_slist_next (selected_attr);
    }
  }

  return string;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator::ObjectIsDropable (Object   *floating_object,
                                          DropZone *in_zone)
{
  if (floating_object && in_zone)
  {
    Player::AttributeId  attr_id  ("availability");
    Player              *player = (Player *) floating_object;
    Attribute           *attr = player->GetAttribute (&attr_id);

    if (attr && (strcmp (attr->GetStrValue (), "Free") == 0))
    {
      return TRUE;
    }
  }

  return FALSE;
}

// --------------------------------------------------------------------------------
void PoolAllocator::Display ()
{
  SetUpCombobox ();

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

      {
        GSList *current = _drop_zones;

        while (current)
        {
          FillPoolTable ((PoolZone *) current->data);

          current = g_slist_next (current);
        }
      }
    }
  }
  FixUpTablesBounds ();

  SignalStatusUpdate ();
}

// --------------------------------------------------------------------------------
void PoolAllocator::Garnish ()
{
  if (_drop_zones == NULL)
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
void PoolAllocator::ApplyConfig ()
{
  Module *next_stage = dynamic_cast <Module *> (GetNextStage ());

  if (next_stage)
  {
    GtkWidget *w = next_stage->GetWidget ("balanced_radiobutton");

    if (w)
    {
      guint seeding_is_balanced = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w));

      if (_seeding_balanced->_value != seeding_is_balanced)
      {
        _seeding_balanced->_value = seeding_is_balanced;

        if (seeding_is_balanced)
        {
          _swapping_sensitivity_trigger.SwitchOn ();
        }
        else
        {
          _swapping_sensitivity_trigger.SwitchOff ();

          if (_swapping_criteria)
          {
            GtkWidget *w = _glade->GetWidget ("swapping_combobox");

            gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                                      0);
            return;
          }
        }
      }
    }

    if (Locked () == FALSE)
    {
      DeletePools ();
      CreatePools ();
      Display ();
      SignalStatusUpdate ();
      MakeDirty ();
    }
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::FillInConfig ()
{
  Module *next_stage = dynamic_cast <Module *> (GetNextStage ());

  if (next_stage)
  {
    GtkWidget *w;

    if (_seeding_balanced->_value)
    {
      w = next_stage->GetWidget ("balanced_radiobutton");
    }
    else
    {
      w = next_stage->GetWidget ("strength_radiobutton");
    }

    if (w)
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                    TRUE);
    }
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::LoadConfiguration (xmlNode *xml_node)
{
  Stage::LoadConfiguration (xml_node);

  if (_seeding_balanced)
  {
    _seeding_balanced->Load (xml_node);

    if (_seeding_balanced->_value == FALSE)
    {
      _swapping_sensitivity_trigger.SwitchOff ();
    }
  }

  if (_swapping)
  {
    GtkTreeModel *model          = GTK_TREE_MODEL (_glade->GetObject ("swapping_liststore"));
    guint         criteria_index = 0;

    _swapping->Load (xml_node);

    if (_swapping->_string)
    {
      GtkTreeIter iter;
      gboolean    iter_is_valid;

      iter_is_valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (model),
                                                     &iter);
      for (guint i = 0; iter_is_valid; i++)
      {
        AttributeDesc *attr_desc;

        gtk_tree_model_get (model, &iter,
                            SWAPPING_CRITERIA, &attr_desc,
                            -1);
        if (   ((attr_desc == NULL) && strcmp (_swapping->_string, "Aucun") == 0)
            || (attr_desc && (strcmp (attr_desc->_code_name, _swapping->_string) == 0)))
        {
          criteria_index = i;
          break;
        }
        iter_is_valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (model),
                                                  &iter);
      }
    }
    gtk_combo_box_set_active (GTK_COMBO_BOX (_glade->GetObject ("swapping_combobox")),
                              criteria_index);
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::SaveConfiguration (xmlTextWriter *xml_writer)
{
  Stage::SaveConfiguration (xml_writer);

  if (_seeding_balanced)
  {
    _seeding_balanced->Save (xml_writer);
  }

  if (_swapping)
  {
    _swapping->Save (xml_writer);
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::Load (xmlNode *xml_node)
{
  static Pool     *current_pool = NULL;
  static PoolZone *current_zone = NULL;
  static guint     nb_pool      = 0;

  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (   (_loaded == FALSE)
          && strcmp ((char *) n->name, _xml_class_name) == 0)
      {
        LoadConfiguration (xml_node);

        {
          gchar *string = (gchar *) xmlGetProp (xml_node,
                                                BAD_CAST "NbDePoules");
          if (string)
          {
            nb_pool = (guint) atoi (string);
          }
        }
        _loaded = TRUE;
      }
      else if (strcmp ((char *) n->name, "Poule") == 0)
      {
        if (_config_list == NULL)
        {
          Setup ();

          // Get the configuration
          {
            GSList *current = _config_list;

            while (current)
            {
              _selected_config = (Configuration *) current->data;

              if (_selected_config->_nb_pool == nb_pool)
              {
                break;
              }
              current = g_slist_next (current);
            }
          }

          // Assign players an original pool
          {
            CreatePools ();
            DeletePools ();
          }

          {
            _nb_matchs = GetNbMatchs ();
            RefreshMatchRate (_nb_matchs);
          }
        }

        {
          guint           number      = g_slist_length (_drop_zones);
          PoolMatchOrder *match_order = new PoolMatchOrder (_contest->GetWeaponCode ());

          current_pool = new Pool (_max_score,
                                   number+1,
                                   match_order,
                                   _rand_seed);
          match_order->Release ();

          {
            current_zone = new PoolZone (this,
                                         current_pool);

            _drop_zones = g_slist_append (_drop_zones,
                                          current_zone);
          }
        }
      }
      else if (strcmp ((char *) n->name, "Tireur") == 0)
      {
        if (current_pool == NULL)
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
              current_pool->AddFencer (player,
                                       this);
            }
          }
        }
      }
      else if (strcmp ((char *) n->name, "Arbitre") == 0)
      {
        gchar *attr = (gchar *) xmlGetProp (n, BAD_CAST "REF");

        if (attr)
        {
          Player *referee = _contest->GetRefereeFromRef (atoi (attr));

          if (referee)
          {
            current_zone->AddObject (referee);
          }
        }
      }
      else if (strcmp ((char *) n->name, "Match") == 0)
      {
        current_pool->CreateMatchs ();
        current_pool->Load (n,
                            _attendees->GetShortList ());
        current_pool = NULL;
        return;
      }
      else
      {
        current_pool = NULL;
        return;
      }
    }

    Load (n->children);
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::Save (xmlTextWriter *xml_writer)
{
  xmlTextWriterStartElement (xml_writer,
                             BAD_CAST _xml_class_name);

  SaveConfiguration (xml_writer);

  if (_drop_zones)
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "NbDePoules",
                                       "%d", g_slist_length (_drop_zones));
  }
  xmlTextWriterWriteFormatAttribute (xml_writer,
                                     BAD_CAST "PhaseSuivanteDesQualifies",
                                     "%d", GetId ()+2);
  if (_selected_config)
  {
    xmlTextWriterWriteFormatAttribute (xml_writer,
                                       BAD_CAST "NbQualifiesParIndice",
                                       "%d", _selected_config->_size);
  }

  Stage::SaveAttendees (xml_writer);

  {
    GSList *current = _drop_zones;

    while (current)
    {
      Pool *pool = GetPoolOf (current);

      pool->Save (xml_writer);
      current = g_slist_next (current);
    }
  }

  xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void PoolAllocator::RegisterConfig (Configuration *config)
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
                          NB_POOLS_COL, nb_pool_text,
                          -1);
      g_free (nb_pool_text);
    }

    {
      gchar *pool_size_text;

      if (config->_nb_overloaded)
      {
        pool_size_text = g_strdup_printf ("%d ou %d", config->_size, config->_size+1);
      }
      else
      {
        pool_size_text = g_strdup_printf ("%d", config->_size);
      }

      gtk_list_store_set (_combobox_store, &iter,
                          POOL_SIZE_COL, pool_size_text,
                          -1);
      g_free (pool_size_text);
    }

    if (   (config->_size < 7)
        || (config->_size == 7) && (config->_nb_overloaded == 0))
    {
      _best_config = config;
    }

    config = NULL;
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::Setup ()
{
  guint           nb_players    = g_slist_length (_attendees->GetShortList ());
  Configuration  *config        = NULL;
  guint           max_pool_size;

  {
    PoolMatchOrder *pool_match_order = new PoolMatchOrder (_contest->GetWeaponCode ());

    if (nb_players % pool_match_order->GetMaxPoolSize () == 0)
    {
      max_pool_size = pool_match_order->GetMaxPoolSize ();
    }
    else
    {
      max_pool_size = pool_match_order->GetMaxPoolSize () -1;
    }
    pool_match_order->Release ();
  }

  _best_config = NULL;
  for (unsigned int size = 3; size <= MIN (max_pool_size, nb_players); size++)
  {
    if (nb_players%size == 0)
    {
      config = (Configuration *) g_malloc (sizeof (Configuration));

      config->_nb_pool       = nb_players/size;
      config->_size          = size;
      config->_nb_overloaded = 0;

      RegisterConfig (config);
    }

    if (   (nb_players%size != 0)
        || ((size == 6) && (nb_players%7 != 0)))
    {
      for (unsigned int p = 1; p < nb_players/size; p++)
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
                                       NULL,
                                       best_index))
    {
      gtk_list_store_set (_combobox_store, &iter,
                          BEST_PIXMAP_COL, GTK_STOCK_ABOUT,
                          -1);
    }
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::PopulateFencerList ()
{
  if (_attendees)
  {
    GSList *current_player = _attendees->GetShortList ();

    _fencer_list->Wipe ();
    while (current_player)
    {
      _fencer_list->Add ((Player *) current_player->data);
      current_player = g_slist_next (current_player);
    }
    _fencer_list->OnAttrListUpdated ();
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::CreatePools ()
{
  if (_selected_config)
  {
    Pool          **pool_table;
    GSList         *shortlist   = _attendees->GetShortList ();
    guint           nb_pool     = _selected_config->_nb_pool;
    PoolMatchOrder *match_order = new PoolMatchOrder (_contest->GetWeaponCode ());

    pool_table = (Pool **) g_malloc (nb_pool * sizeof (Pool *));
    for (guint i = 0; i < nb_pool; i++)
    {
      pool_table[i] = new Pool (_max_score,
                                i+1,
                                match_order,
                                _rand_seed);

      {
        PoolZone *zone = new PoolZone (this,
                                       pool_table[i]);

        _drop_zones = g_slist_append (_drop_zones,
                                      zone);
      }
    }

    {
      GreenSwapper *swapper;
      guint    nb_fencer = g_slist_length (shortlist);

      if (_swapping_criteria && _seeding_balanced->_value)
      {
        swapper = new GreenSwapper (_drop_zones,
                                    _swapping_criteria->_code_name,
                                    shortlist);
      }
      else
      {
        swapper = new GreenSwapper (_drop_zones,
                                    NULL,
                                    shortlist);
      }

      for (guint i = 0; i < nb_fencer; i++)
      {
        Player *player;
        Pool   *pool;

        if (_seeding_balanced->_value)
        {
          if (((i / nb_pool) % 2) == 0)
          {
            pool = pool_table[i%nb_pool];
          }
          else
          {
            pool = pool_table[nb_pool-1 - i%nb_pool];
          }
        }
        else
        {
          if (i == 0)
          {
            pool = pool_table[0];
          }
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
        }

        player = swapper->GetNextPlayer (pool);
        player->SetData (this,
                         "original_pool",
                         (void *) pool->GetNumber ());
        pool->AddFencer (player,
                         this);
      }

      swapper->Release ();
    }

    match_order->Release ();

    {
      _nb_matchs = GetNbMatchs ();
      RefreshMatchRate (_nb_matchs);
    }
  }
}

// --------------------------------------------------------------------------------
gint PoolAllocator::GetNbMatchs ()
{
  gint nb_matchs;

  nb_matchs  = _selected_config->_size * (_selected_config->_size - 1) / 2;
  nb_matchs *= _selected_config->_nb_pool - _selected_config->_nb_overloaded;
  nb_matchs += _selected_config->_nb_overloaded * _selected_config->_size * (_selected_config->_size + 1) / 2;

  return nb_matchs;
}

// --------------------------------------------------------------------------------
void PoolAllocator::SetUpCombobox ()
{
  if (_best_config)
  {
    GtkWidget *w;
    gint       config_index;

    config_index = g_slist_index (_config_list,
                                  _selected_config);

    w = _glade->GetWidget ("nb_pools_combobox");
    g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                          (void *) on_nb_pools_combobox_changed,
                                          (Object *) this);
    gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                              config_index);
    g_signal_connect (G_OBJECT (w), "changed",
                      G_CALLBACK (on_nb_pools_combobox_changed),
                      (Object *) this);

    w = _glade->GetWidget ("pool_size_combobox");
    g_signal_handlers_disconnect_by_func (G_OBJECT (w),
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
void PoolAllocator::FixUpTablesBounds ()
{
  if (_selected_config)
  {
    GSList *current = _drop_zones;

    while (current)
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

      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::FillPoolTable (PoolZone *zone)
{
  if (_main_table == NULL)
  {
    return;
  }

  GooCanvasItem *item;
  GooCanvasItem *table         = (GooCanvasItem *) zone->GetPtrData (this, "table");
  GSList        *selected_attr = NULL;
  Pool          *pool          = zone->GetPool ();

  if (_filter)
  {
    selected_attr = _filter->GetSelectedAttrList ();
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
    guint pool_size           = pool->GetNbPlayers ();
    GooCanvasItem *name_table = goo_canvas_table_new (table, NULL);

    // Status icon
    if (_selected_config)
    {
      gchar *icon_name;

      if (   (pool_size == _selected_config->_size)
          || (   _selected_config->_nb_overloaded
              && ((pool_size == _selected_config->_size)) || (pool_size == _selected_config->_size + 1)))
      {
        icon_name = g_strdup (GTK_STOCK_APPLY);
        pool->SetData (this, "is_balanced", (void *) 1);
      }
      else
      {
        icon_name = g_strdup (GTK_STOCK_DIALOG_WARNING);
        pool->SetData (this, "is_balanced", 0);
      }

      Canvas::PutStockIconInTable (name_table,
                                   icon_name,
                                   0, 0);
      g_free (icon_name);
    }

    // Name
    {
      item = Canvas::PutTextInTable (name_table,
                                     pool->GetName (),
                                     0, 1);
      g_object_set (G_OBJECT (item),
                    "font", "Sans bold 18px",
                    NULL);
    }

    Canvas::PutInTable (table,
                        name_table,
                        0, 0);
    Canvas::SetTableItemAttribute (name_table, "columns", g_slist_length (selected_attr));
    //Canvas::SetTableItemAttribute (name_table, "x-expand", 1U);
    Canvas::SetTableItemAttribute (name_table, "x-fill", 1U);
  }

  if (selected_attr)
  {
    guint p = 0;

    // Referees
    {
      GSList *current = pool->GetRefereeList ();

      while (current)
      {
        Player *player = (Player *) current->data;

        DisplayPlayer (player,
                       p,
                       table,
                       zone,
                       selected_attr);
        p++;

        current = g_slist_next (current);
      }
    }

    // Fencers
    {
      GSList *current = pool->GetFencerList ();

      while (current)
      {
        Player *player = (Player *) current->data;

        DisplayPlayer (player,
                       p,
                       table,
                       zone,
                       selected_attr);
        p++;

        current = g_slist_next (current);
      }
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
void PoolAllocator::DisplayPlayer (Player        *player,
                                   guint          indice,
                                   GooCanvasItem *table,
                                   PoolZone      *zone,
                                   GSList        *selected_attr)
{
  if (player)
  {
    Pool *pool = zone->GetPool ();

    if (player->IsFencer () == FALSE)
    {
      static gchar  *referee_icon = NULL;

      if (referee_icon == NULL)
      {
        referee_icon = g_build_filename (_program_path, "resources/glade/referee.png", NULL);
      }
      Canvas::PutIconInTable (table,
                              referee_icon,
                              indice+1, 0);
    }
    else if (player->GetUIntData (this, "original_pool") != pool->GetNumber ())
    {
      Canvas::PutStockIconInTable (table,
                                   GTK_STOCK_REFRESH,
                                   indice+1, 0);
    }

    for (guint i = 0; selected_attr != NULL; i++)
    {
      GooCanvasItem       *item;
      AttributeDesc       *attr_desc;
      Attribute           *attr;
      Player::AttributeId *attr_id;

      attr_desc = (AttributeDesc *) selected_attr->data;
      attr_id = Player::AttributeId::CreateAttributeId (attr_desc, this);
      attr = player->GetAttribute (attr_id);
      attr_id->Release ();

      if (attr)
      {
        gchar *image = attr->GetUserImage ();

        item = Canvas::PutTextInTable (table,
                                       image,
                                       indice+1, i+1);
        g_free (image);
      }
      else
      {
        item = Canvas::PutTextInTable (table,
                                       " ",
                                       indice+1, i+1);
      }

      if (player->IsFencer ())
      {
        g_object_set (G_OBJECT (item),
                      "font", "Sans 14px",
                      "fill_color", "black",
                      NULL);
      }
      else
      {
        g_object_set (G_OBJECT (item),
                      "font", "Sans Bold Italic 14px",
                      "fill_color", "black",
                      NULL);
      }

      SetObjectDropZone (player,
                         item,
                         zone);

      selected_attr = g_slist_next (selected_attr);
    }
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::DeletePools ()
{
  if (_drop_zones)
  {
    GSList *current = _drop_zones;

    while (current)
    {
      Pool *pool = GetPoolOf (current);

      Object::TryToRelease ((PoolZone *) current->data);
      Object::TryToRelease (pool);
      current = g_slist_next (current);
    }

    g_slist_free (_drop_zones);
    _drop_zones = NULL;
  }

  CanvasModule::Wipe ();
  _main_table = NULL;

  RefreshMatchRate (-_nb_matchs);
  _nb_matchs = 0;
}

// --------------------------------------------------------------------------------
void PoolAllocator::Wipe ()
{
  DeletePools ();

  {
    g_signal_handlers_disconnect_by_func (_glade->GetWidget ("pool_size_combobox"),
                                          (void *) on_pool_size_combobox_changed,
                                          (Object *) this);
    g_signal_handlers_disconnect_by_func (_glade->GetWidget ("nb_pools_combobox"),
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
    GSList *current = _config_list;

    while (current)
    {
      Configuration *config = (Configuration *) current->data;

      g_free (config);
      current = g_slist_next (current);
    }
    g_slist_free (_config_list);
    _config_list     = NULL;
    _selected_config = NULL;
  }
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator::IsOver ()
{
  GSList *current = _drop_zones;

  while (current)
  {
    Pool *pool = GetPoolOf (current);

    if (pool->GetUIntData (this, "is_balanced") == 0)
    {
      return FALSE;
    }
    current = g_slist_next (current);
  }
  return TRUE;
}

// --------------------------------------------------------------------------------
void PoolAllocator::OnBeginPrint (GtkPrintOperation *operation,
                                  GtkPrintContext   *context)
{
  gdouble canvas_x;
  gdouble canvas_y;
  gdouble canvas_w;
  gdouble canvas_h;
  gdouble paper_w  = gtk_print_context_get_width (context);
  gdouble paper_h  = gtk_print_context_get_height (context);
  gdouble header_h = (PRINT_HEADER_HEIGHT+2) * paper_w  / 100;

  {
    GooCanvasBounds bounds;

    goo_canvas_item_get_bounds (GetRootItem (),
                                &bounds);
    canvas_x = bounds.x1;
    canvas_y = bounds.y1;
    canvas_w = bounds.x2 - bounds.x1;
    canvas_h = bounds.y2 - bounds.y1;
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

    gtk_print_operation_set_n_pages (operation,
                                     _nb_page);
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::OnDrawPage (GtkPrintOperation *operation,
                                GtkPrintContext   *context,
                                gint               page_nr)
{
  gdouble paper_w = gtk_print_context_get_width  (context);
  cairo_t         *cr       = gtk_print_context_get_cairo_context (context);
  gdouble          header_h = (PRINT_HEADER_HEIGHT+2) * paper_w  / 100;
  GooCanvasBounds  bounds;

  DrawContainerPage (operation,
                     context,
                     page_nr);

  {
    GooCanvas *canvas = Canvas::CreatePrinterCanvas (context);
    char      *text   = g_strdup_printf ("Page %d/%d", page_nr+1, _nb_page);

    goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                         text,
                         98.0, 9.0,
                         -1.0,
                         GTK_ANCHOR_SE,
                         "fill-color", "black",
                         "font", "Sans Bold 2px", NULL);
    g_free (text);

    goo_canvas_render (canvas,
                       gtk_print_context_get_cairo_context (context),
                       NULL,
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
Data *PoolAllocator::GetMaxScore ()
{
  return _max_score;
}

// --------------------------------------------------------------------------------
gboolean PoolAllocator::SeedingIsBalanced ()
{
  return (_seeding_balanced->_value == 1);
}

// --------------------------------------------------------------------------------
guint PoolAllocator::GetNbPools ()
{
  return g_slist_length (_drop_zones);
}

// --------------------------------------------------------------------------------
Pool *PoolAllocator::GetPool (guint index)
{
  return GetPoolOf (g_slist_nth (_drop_zones,
                                 index));
}

// --------------------------------------------------------------------------------
Pool *PoolAllocator::GetPoolOf (GSList *drop_zone)
{
  PoolZone *zone = (PoolZone *) drop_zone->data;

  return zone->GetPool ();
}

// --------------------------------------------------------------------------------
PoolZone *PoolAllocator::GetZone (guint index)
{
  GSList *zone = g_slist_nth (_drop_zones,
                              index);

  return (PoolZone *) zone->data;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_swapping_combobox_changed (GtkWidget *widget,
                                                              Object    *owner)
{
  PoolAllocator *pl = dynamic_cast <PoolAllocator *> (owner);

  pl->OnSwappingComboboxChanged (GTK_COMBO_BOX (widget));
}

// --------------------------------------------------------------------------------
void PoolAllocator::OnSwappingComboboxChanged (GtkComboBox *cb)
{
  GtkTreeIter iter;

  gtk_combo_box_get_active_iter (cb,
                                 &iter);
  gtk_tree_model_get (GTK_TREE_MODEL (_glade->GetObject ("swapping_liststore")),
                      &iter,
                      SWAPPING_CRITERIA, &_swapping_criteria,
                      -1);

  if (_swapping_criteria)
  {
    _swapping->_string = _swapping_criteria->_code_name;
  }
  else
  {
    _swapping->_string = "Aucun";
  }

  if (_drop_zones)
  {
    DeletePools ();
    CreatePools ();

    Display ();
    SignalStatusUpdate ();
    MakeDirty ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_pool_size_combobox_changed (GtkWidget *widget,
                                                               Object    *owner)
{
  PoolAllocator *pl = dynamic_cast <PoolAllocator *> (owner);

  pl->OnComboboxChanged (GTK_COMBO_BOX (widget));
}

// --------------------------------------------------------------------------------
void PoolAllocator::OnComboboxChanged (GtkComboBox *cb)
{
  gint config_index     = gtk_combo_box_get_active (cb);
  Configuration *config = (Configuration *) g_slist_nth_data (_config_list,
                                                              config_index);

  if (config)
  {
    {
      GtkWidget *w;

      w = _glade->GetWidget ("nb_pools_combobox");
      g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                            (void *) on_nb_pools_combobox_changed,
                                            (Object *) this);
      gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                                config_index);
      g_signal_connect (G_OBJECT (w), "changed",
                        G_CALLBACK (on_nb_pools_combobox_changed),
                        (Object *) this);

      w = _glade->GetWidget ("pool_size_combobox");
      g_signal_handlers_disconnect_by_func (G_OBJECT (w),
                                            (void *) on_pool_size_combobox_changed,
                                            (Object *) this);
      gtk_combo_box_set_active (GTK_COMBO_BOX (w),
                                config_index);
      g_signal_connect (G_OBJECT (w), "changed",
                        G_CALLBACK (on_pool_size_combobox_changed),
                        (Object *) this);
    }

    _selected_config = config;

    DeletePools ();
    CreatePools ();
    Display ();
    SignalStatusUpdate ();
    MakeDirty ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_nb_pools_combobox_changed (GtkWidget *widget,
                                                              Object    *owner)
{
  PoolAllocator *c = dynamic_cast <PoolAllocator *> (owner);

  c->OnComboboxChanged (GTK_COMBO_BOX (widget));
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_print_toolbutton_clicked (GtkWidget *widget,
                                                             Object    *owner)
{
  PoolAllocator *pa = dynamic_cast <PoolAllocator *> (owner);
  gchar         *title = g_strdup_printf ("%s %s", gettext ("Allocation"), pa->GetName ());

  pa->Print (title);
  g_free (title);
}

// --------------------------------------------------------------------------------
void PoolAllocator::OnLoadingCompleted ()
{
  if (IsPlugged () && (Locked () == FALSE))
  {
    GSList *current = _drop_zones;

    while (current)
    {
      PoolZone *zone = (PoolZone *) current->data;

      zone->BookReferees ();
      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void PoolAllocator::OnLocked ()
{
  DisableSensitiveWidgets ();

  {
    GSList *current = _drop_zones;

    while (current)
    {
      Pool *pool = GetPoolOf (current);

      pool->CreateMatchs ();
      current = g_slist_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
GSList *PoolAllocator::GetCurrentClassification ()
{
  return g_slist_copy (_attendees->GetShortList ());
}

// --------------------------------------------------------------------------------
void PoolAllocator::OnUnLocked ()
{
  EnableSensitiveWidgets ();
}

// --------------------------------------------------------------------------------
void PoolAllocator::OnAttrListUpdated ()
{
  GSList *current = _drop_zones;

  _max_w = 0;
  _max_h = 0;

  while (current)
  {
    FillPoolTable ((PoolZone *) current->data);

    current = g_slist_next (current);
  }
  FixUpTablesBounds ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_filter_button_clicked (GtkWidget *widget,
                                                          Object    *owner)
{
  PoolAllocator *p = dynamic_cast <PoolAllocator *> (owner);

  p->SelectAttributes ();
}

// --------------------------------------------------------------------------------
void PoolAllocator::OnFencerListToggled (gboolean toggled)
{
  GtkWidget *main_w        = GetWidget ("main_hook");
  GtkWidget *fencer_list_w = GetWidget ("fencer_list_hook");

  if (toggled)
  {
    if (main_w)
    {
      gtk_widget_hide_all (main_w);
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
  PoolAllocator *p = dynamic_cast <PoolAllocator *> (owner);

  p->OnFencerListToggled (gtk_toggle_tool_button_get_active (widget));
}
