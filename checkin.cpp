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
#include <string.h>

#include "attribute.hpp"
#include "schedule.hpp"
#include "player.hpp"
#include "filter.hpp"

#include "checkin.hpp"

const gchar *Checkin::_class_name     = "checkin";
const gchar *Checkin::_xml_class_name = "checkin_stage";

// --------------------------------------------------------------------------------
Checkin::Checkin (StageClass *stage_class)
: Stage_c (stage_class),
  PlayersList ("checkin.glade")
{
  // Sensitive widgets
  {
    AddSensitiveWidget (_glade->GetWidget ("add_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("remove_player_button"));
    AddSensitiveWidget (_glade->GetWidget ("import_toolbutton"));
  }

  // Player attributes to display
  {
    GSList *attr_list;
    Filter *filter;

    AttributeDesc::CreateList (&attr_list,
                               "ref",
                               "exported",
                               NULL);
    filter = new Filter (attr_list,
                         this);

    filter->ShowAttribute ("attending");
    filter->ShowAttribute ("rating");
    filter->ShowAttribute ("name");
    filter->ShowAttribute ("first_name");
    filter->ShowAttribute ("birth_year");
    filter->ShowAttribute ("gender");
    filter->ShowAttribute ("club");
    filter->ShowAttribute ("country");
    filter->ShowAttribute ("licence");

    SetFilter (filter);
    filter->Release ();
  }

  {
    GSList *filter_list = _filter->GetAttrList ();

    for (guint i = 0; i < g_slist_length (filter_list); i++)
    {
      AttributeDesc *attr_desc;

      attr_desc = (AttributeDesc *) g_slist_nth_data (filter_list,
                                                      i);

      if (attr_desc->_rights == AttributeDesc::PUBLIC)
      {
        {
          GtkWidget *w   = gtk_label_new (attr_desc->_name);
          GtkWidget *box = _glade->GetWidget ("title_vbox");

          gtk_box_pack_start (GTK_BOX (box),
                              w,
                              TRUE,
                              TRUE,
                              0);
          g_object_set (G_OBJECT (w),
                        "xalign", 0.0,
                        NULL);
        }

        {
          GtkWidget *value_w = gtk_label_new (attr_desc->_name);

          {
            GtkWidget *box = GetWidget ("value_vbox");

            if (attr_desc->_type == G_TYPE_BOOLEAN)
            {
              value_w = gtk_check_button_new ();
            }
            else
            {
              value_w = gtk_entry_new ();
            }

            gtk_box_pack_start (GTK_BOX (box),
                                value_w,
                                TRUE,
                                TRUE,
                                0);
            g_object_set (G_OBJECT (value_w),
                          "xalign", 0.0,
                          NULL);
            g_object_set_data (G_OBJECT (value_w), "attribute_name", attr_desc->_name);
          }

          {
            GtkWidget *w   = gtk_check_button_new ();
            GtkWidget *box = _glade->GetWidget ("check_vbox");

            gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                          TRUE);
            g_signal_connect (w, "toggled",
                              G_CALLBACK (on_sensitive_state_toggled), value_w);

            gtk_box_pack_start (GTK_BOX (box),
                                w,
                                TRUE,
                                TRUE,
                                0);
            g_object_set (G_OBJECT (w),
                          "xalign", 0.0,
                          NULL);
          }
        }
      }
    }
  }
}

// --------------------------------------------------------------------------------
Checkin::~Checkin ()
{
}

// --------------------------------------------------------------------------------
void Checkin::Init ()
{
  RegisterStageClass (_class_name,
                      _xml_class_name,
                      CreateInstance,
                      0);
}

// --------------------------------------------------------------------------------
Stage_c *Checkin::CreateInstance (StageClass *stage_class)
{
  return new Checkin (stage_class);
}

// --------------------------------------------------------------------------------
void Checkin::OnPlugged ()
{
}

// --------------------------------------------------------------------------------
void Checkin::Load (xmlNode *xml_node)
{
  for (xmlNode *n = xml_node; n != NULL; n = n->next)
  {
    if (n->type == XML_ELEMENT_NODE)
    {
      if (strcmp ((char *) n->name, "player") == 0)
      {
        Player_c *player = new Player_c;

        player->Load (n);
        Add (player);

        player->Release ();
      }
      else if (strcmp ((char *) n->name, _xml_class_name) != 0)
      {
        UpdateRanking ();
        return;
      }
    }
    Load (n->children);
  }
}

// --------------------------------------------------------------------------------
void Checkin::Save (xmlTextWriter *xml_writer)
{
  int result;

  result = xmlTextWriterStartElement (xml_writer,
                                      BAD_CAST _xml_class_name);

  for (guint i = 0; i < g_slist_length (_player_list); i++)
  {
    Player_c *p;

    p = (Player_c *) g_slist_nth_data (_player_list, i);

    if (p)
    {
      p->Save (xml_writer);
    }
  }

  result = xmlTextWriterEndElement (xml_writer);
}

// --------------------------------------------------------------------------------
void Checkin::Display ()
{
  OnAttrListUpdated ();
}

// --------------------------------------------------------------------------------
void Checkin::UpdateRanking ()
{
  _player_list = g_slist_sort_with_data (_player_list,
                                         (GCompareDataFunc) Player_c::Compare,
                                         (void *) "rating");

  for (guint i = 0; i <  g_slist_length (_player_list); i++)
  {
    Player_c *p;

    p = (Player_c *) g_slist_nth_data (_player_list, i);
    p->SetAttributeValue ("rank",
                          i + 1);
    Update (p);
  }
}

// --------------------------------------------------------------------------------
void Checkin::OnLocked ()
{
  DisableSensitiveWidgets ();
  SetSensitiveState (FALSE);

  {
    GSList *result = CreateCustomList (PresentPlayerFilter);

    if (result)
    {
      result = g_slist_sort_with_data (result,
                                       (GCompareDataFunc) Player_c::Compare,
                                       (void *) "rank");
      SetResult (result);
    }
  }
}

// --------------------------------------------------------------------------------
gboolean Checkin::IsOver ()
{
  return TRUE;
}

// --------------------------------------------------------------------------------
gboolean Checkin::PresentPlayerFilter (Player_c *player)
{
  Attribute_c *attr = player->GetAttribute ("attending");

  return ((gboolean) attr->GetValue () == TRUE);
}

// --------------------------------------------------------------------------------
void Checkin::OnUnLocked ()
{
  EnableSensitiveWidgets ();
  SetSensitiveState (TRUE);

  UpdateRanking ();
}

// --------------------------------------------------------------------------------
void Checkin::Wipe ()
{
}

// --------------------------------------------------------------------------------
void Checkin::Import ()
{
  GtkWidget *chooser = gtk_file_chooser_dialog_new ("Choose a fencers file to import ...",
                                                    NULL,
                                                    GTK_FILE_CHOOSER_ACTION_OPEN,
                                                    GTK_STOCK_CANCEL,
                                                    GTK_RESPONSE_CANCEL,
                                                    GTK_STOCK_OPEN,
                                                    GTK_RESPONSE_ACCEPT,
                                                    NULL);

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              "All FFE files (.FFF)");
    gtk_file_filter_add_pattern (filter,
                                 "*.FFF");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              "All Excell files (.CSV)");
    gtk_file_filter_add_pattern (filter,
                                 "*.CSV");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  {
    GtkFileFilter *filter = gtk_file_filter_new ();

    gtk_file_filter_set_name (filter,
                              "All files");
    gtk_file_filter_add_pattern (filter,
                                 "*");
    gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (chooser),
                                 filter);
  }

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    gchar *filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
    gchar *file;

    {
      gchar *raw_file;
      gsize  length;
      guint  j;

      g_file_get_contents ((const gchar *) filename,
                           &raw_file,
                           &length,
                           NULL);
      length++;

      file = (gchar *) g_malloc (length);

      j = 0;
      for (guint i = 0; i < length; i++)
      {
        if (raw_file[i] != '\r')
        {
          file[j] = raw_file[i];
          j++;
        }
      }
    }

    if (strstr (filename, ".FFF"))
    {
      ImportFFF (file);
    }
    else
    {
      ImportCSV (file);
    }

    g_free (file);
  }

  gtk_widget_destroy (chooser);

  UpdateRanking ();
}

// --------------------------------------------------------------------------------
void Checkin::ImportFFF (gchar *file)
{
  gchar **lines = g_strsplit_set (file,
                                  "\n",
                                  0);

  if (lines == NULL)
  {
    return;
  }
  else
  {
    // Header
    // FFF;WIN;CLASSEMENT;FFE
    // 08/11/2009;;;;

    // Fencers
    for (guint l = 2; lines[l] != NULL; l++)
    {
      gchar **tokens = g_strsplit_set (lines[l],
                                       ",;",
                                       0);

      if (tokens && tokens[0])
      {
        Player_c *player = new Player_c;

        player->SetAttributeValue ("attending", (guint) FALSE);
        player->SetAttributeValue ("name",       tokens[0]);
        player->SetAttributeValue ("first_name", tokens[1]);
        player->SetAttributeValue ("birth_year", tokens[2]);
        player->SetAttributeValue ("gender",     tokens[3]);
        player->SetAttributeValue ("country",    tokens[4]);
        player->SetAttributeValue ("licence",    tokens[8]);
        player->SetAttributeValue ("club",       tokens[10]);
        player->SetAttributeValue ("exported",   (guint) FALSE);
        if (*tokens[11])
        {
          player->SetAttributeValue ("rating", tokens[11]);
        }
        else
        {
          player->SetAttributeValue ("rating", (guint) 0);
        }
        Add (player);
        player->Release ();

        g_strfreev (tokens);
      }
    }

    g_strfreev (lines);
  }
}

// --------------------------------------------------------------------------------
void Checkin::ImportCSV (gchar *file)
{
  guint nb_attr = 0;

  gchar **header_line = g_strsplit_set (file,
                                        "\n",
                                        0);

  if (header_line == NULL)
  {
    return;
  }
  else
  {
    // Header
    {
      gchar **header_attr = g_strsplit_set (header_line[0],
                                            ";",
                                            0);

      if (header_attr)
      {
        for (guint i = 0; header_attr[i] != NULL; i++)
        {
          nb_attr++;
        }
        g_strfreev (header_attr);
      }

      g_strfreev (header_line);
    }

    // Fencers
    {
      gchar **tokens = g_strsplit_set (file,
                                       ";\n",
                                       0);

      if (tokens)
      {
        for (guint i = nb_attr; tokens[i] != NULL; i += nb_attr)
        {
          Player_c *player = new Player_c;

          player->SetAttributeValue ("name",       tokens[i]);
          player->SetAttributeValue ("first_name", tokens[i+1]);
          player->SetAttributeValue ("exported",   (guint) FALSE);

          Add (player);
          player->Release ();
        }
        g_strfreev (tokens);
      }
    }
  }
}

// --------------------------------------------------------------------------------
void Checkin::on_add_button_clicked ()
{
  Player_c *player   = new Player_c;
  GList    *children = gtk_container_get_children (GTK_CONTAINER (GetWidget ("value_vbox")));

  {
    gchar *str = g_strdup_printf ("%d\n", player->GetRef ());
    player->SetAttributeValue ("ref", str);
    g_free (str);
  }

  player->SetAttributeValue ("exported", (guint) FALSE);

  for (guint i = 0; i < g_list_length (children); i ++)
  {
    GtkWidget       *w;
    gchar           *attr_name;
    AttributeDesc   *attr_desc;

    w = (GtkWidget *) g_list_nth_data (children,
                                       i);
    attr_name = (gchar *) g_object_get_data (G_OBJECT (w), "attribute_name");
    attr_desc = AttributeDesc::GetDesc (attr_name);

    if (attr_desc->_type == G_TYPE_BOOLEAN)
    {
      player->SetAttributeValue (attr_name,
                                 gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (w)));
      if (attr_desc->_uniqueness == AttributeDesc::SINGULAR)
      {
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w),
                                      FALSE);
      }
    }
    else
    {
      player->SetAttributeValue (attr_name,
                                 (gchar *) gtk_entry_get_text (GTK_ENTRY (w)));

      if (attr_desc->_uniqueness == AttributeDesc::SINGULAR)
      {
        gtk_entry_set_text (GTK_ENTRY (w), "");
      }
    }
  }

  gtk_widget_grab_focus ((GtkWidget *) g_list_nth_data (children,
                                                        0));

  Add (player);
  player->Release ();
  UpdateRanking ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_add_player_button_clicked (GtkWidget *widget,
                                                              Object_c  *owner)
{
  Checkin *p = dynamic_cast <Checkin *> (owner);

  p->on_add_player_button_clicked ();
}

// --------------------------------------------------------------------------------
void Checkin::on_add_player_button_clicked ()
{
  GtkWidget *w = _glade->GetWidget ("FillInForm");

  gtk_widget_show_all (w);

  {
    GList *children = gtk_container_get_children (GTK_CONTAINER (GetWidget ("value_vbox")));

    gtk_widget_grab_focus ((GtkWidget *) g_list_nth_data (children,
                                                          0));
  }
}

// --------------------------------------------------------------------------------
void Checkin::on_sensitive_state_toggled (GtkToggleButton *togglebutton,
                                          GtkWidget       *w)
{
  if (gtk_toggle_button_get_active (togglebutton))
  {
    gtk_widget_set_sensitive (w,
                              TRUE);
  }
  else
  {
    gtk_widget_set_sensitive (w,
                              FALSE);
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_remove_player_button_clicked (GtkWidget *widget,
                                                                 Object_c  *owner)
{
  Checkin *p = dynamic_cast <Checkin *> (owner);

  p->on_remove_player_button_clicked ();
}

// --------------------------------------------------------------------------------
void Checkin::on_remove_player_button_clicked ()
{
  RemoveSelection ();
}

// --------------------------------------------------------------------------------
void Checkin::on_close_button_clicked ()
{
  GtkWidget *w = _glade->GetWidget ("FillInForm");

  gtk_widget_hide (w);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_players_list_filter_button_clicked (GtkWidget *widget,
                                                                       Object_c  *owner)
{
  Checkin *p = dynamic_cast <Checkin *> (owner);

  p->SelectAttributes ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_import_toolbutton_clicked (GtkWidget *widget,
                                                              Object_c  *owner)
{
  Checkin *p = dynamic_cast <Checkin *> (owner);

  p->Import ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_add_button_clicked (GtkWidget *widget,
                                                       Object_c  *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  c->on_add_button_clicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_close_button_clicked (GtkWidget *widget,
                                                         Object_c  *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  c->on_close_button_clicked ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT gboolean on_FillInForm_key_press_event (GtkWidget   *widget,
                                                                   GdkEventKey *event,
                                                                   Object_c    *owner)
{
  Checkin *c = dynamic_cast <Checkin *> (owner);

  if (event->keyval == GDK_Return)
  {
    c->on_add_button_clicked ();
    return TRUE;
  }

  return FALSE;
}
