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
#include <libxml/xmlwriter.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <libxml/xpath.h>

#include "contest.hpp"

gchar *Contest_c::_NEW_CONTEST = NULL;

// --------------------------------------------------------------------------------
Contest_c::Contest_c ()
  : Module_c ("contest.glade")
{
  InitInstance ();

  _schedule->Start ();

  if (gtk_dialog_run (GTK_DIALOG (_properties_dlg)) == GTK_RESPONSE_ACCEPT)
  {
    ReadProperties ();
  }
  gtk_widget_hide (_properties_dlg);

  if (gtk_dialog_run (GTK_DIALOG (_formula_dlg)) == GTK_RESPONSE_ACCEPT)
  {
    ReadProperties ();
  }
  gtk_widget_hide (_formula_dlg);
}

// --------------------------------------------------------------------------------
Contest_c::Contest_c (gchar *filename)
  : Module_c ("contest.glade")
{
  InitInstance ();

  _filename = g_strdup (filename);

  {
    xmlDoc *doc = xmlParseFile (filename);

    xmlXPathInit ();

    // Contest
    {
      xmlXPathContext *xml_context = xmlXPathNewContext (doc);
      xmlXPathObject  *xml_object;
      xmlNodeSet      *xml_nodeset;

      xml_object  = xmlXPathEval (BAD_CAST "/contest", xml_context);
      xml_nodeset = xml_object->nodesetval;

      if (xml_object->nodesetval->nodeNr)
      {
        gchar *attr;

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "title");
        if (attr)
        {
          SetName (g_strdup (attr));
        }

        attr = (gchar *) xmlGetProp (xml_nodeset->nodeTab[0], BAD_CAST "backup");
        if (attr)
        {
          _backup = g_strdup (attr);
        }

        _schedule->Load (xml_nodeset->nodeTab[0]);
      }
      xmlXPathFreeObject  (xml_object);
      xmlXPathFreeContext (xml_context);
    }

    // Players
    _players_base->Load (doc);

    xmlFreeDoc (doc);
  }

  _schedule->Start ();
}

// --------------------------------------------------------------------------------
Contest_c::~Contest_c ()
{
  g_free (_name);
  g_free (_filename);
  g_free (_backup);

  Object_c::Release (_player_list);

  Object_c::Release (_players_base);
  Object_c::Release (_schedule);

  gtk_widget_destroy (_properties_dlg);
  gtk_widget_destroy (_formula_dlg);
}

// --------------------------------------------------------------------------------
Contest_c *Contest_c::Create ()
{
  Contest_c *contest = new Contest_c;

  if (contest->_name == NULL)
  {
    Object_c::Release (contest);
    contest = NULL;
  }

  return contest;
}

// --------------------------------------------------------------------------------
void Contest_c::InitInstance ()
{
  _name            = NULL;
  _filename        = NULL;
  _backup          = NULL;
  _players_base    = new PlayersBase_c ();

  {
    Stage_c *stage;

    _schedule = new Schedule_c (_glade->GetWidget ("stage_entry"),
                                _glade->GetWidget ("previous_stage_toolbutton"),
                                _glade->GetWidget ("next_stage_toolbutton"),
                                _glade->GetWidget ("schedule_notebook"));

    stage = new Stage_c (this,
                         new PlayersList_c (_players_base),
                         "checkin");
    _schedule->AddStage (stage);

    stage = new Stage_c (this,
                         new PoolsList_c (_players_base),
                         "pool allocation");
    _schedule->AddStage (stage);

    stage = new Stage_c (this,
                         new PoolSupervisor_c (),
                         "pool");
    _schedule->AddStage (stage);

    stage = new Stage_c (this,
                         NULL,
                         "table");
    _schedule->AddStage (stage);
  }

  // Properties dialog
  {
    GtkWidget *content_area;

    _properties_dlg = gtk_dialog_new_with_buttons (_NEW_CONTEST,
                                                   NULL,
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_STOCK_OK,
                                                   GTK_RESPONSE_ACCEPT,
                                                   GTK_STOCK_CANCEL,
                                                   GTK_RESPONSE_REJECT,
                                                   NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_properties_dlg));

    gtk_widget_reparent (_glade->GetWidget ("properties_dialog-vbox"),
                         content_area);
  }

  // Formula dialog
  {
    GtkWidget *content_area;

    _formula_dlg = gtk_dialog_new_with_buttons (_NEW_CONTEST,
                                                NULL,
                                                GTK_DIALOG_DESTROY_WITH_PARENT,
                                                GTK_STOCK_OK,
                                                GTK_RESPONSE_ACCEPT,
                                                GTK_STOCK_CANCEL,
                                                GTK_RESPONSE_REJECT,
                                                NULL);

    content_area = gtk_dialog_get_content_area (GTK_DIALOG (_formula_dlg));

    gtk_widget_reparent (_glade->GetWidget ("formula_dialog-vbox"),
                         content_area);
  }

  // Callbacks binding
  {
    _glade->Bind ("save_toolbutton",           this);
    _glade->Bind ("properties_toolbutton",     this);
    _glade->Bind ("formula_toolbutton",        this);
    _glade->Bind ("previous_stage_toolbutton", this);
    _glade->Bind ("next_stage_toolbutton",     this);

    _glade->Bind ("backupfile_button",         this);

    _glade->Bind ("contest_close_button",      this);
  }
}

// --------------------------------------------------------------------------------
void Contest_c::Init ()
{
  _NEW_CONTEST = g_locale_to_utf8 ("Nouvelle compétition",
                                   -1,
                                   NULL,
                                   NULL,
                                   NULL);
}

// --------------------------------------------------------------------------------
void Contest_c::ReadProperties ()
{
  gchar    *str;
  GtkEntry *entry;
  GtkLabel *label;

  entry = GTK_ENTRY (_glade->GetWidget ("title_entry"));
  str = (gchar *) gtk_entry_get_text (entry);
  SetName (g_strdup (str));

  g_free (_backup);
  label = GTK_LABEL (_glade->GetWidget ("backupfile_label"));
  str = (gchar *) gtk_label_get_text (label);
  _backup = NULL;
  if (str[0] != 0)
  {
    _backup = g_strdup (str);
  }
}

// --------------------------------------------------------------------------------
void Contest_c::SetName (gchar *name)
{
  if (_name)
  {
    g_free (_name);
  }

  _name = name;

  {
    GtkWidget *w = _glade->GetWidget ("contest_label");

    if (w)
    {
      gtk_label_set_text (GTK_LABEL (w),
                          _name);
    }
  }
}

// --------------------------------------------------------------------------------
void Contest_c::AttachTo (GtkNotebook *to)
{
  GtkWidget *title = _glade->GetWidget ("notebook_title");

  gtk_notebook_append_page (to,
                            GetRootWidget (),
                            title);
  g_object_unref (title);

  gtk_notebook_set_current_page (GTK_NOTEBOOK (to),
                                 -1);
}

// --------------------------------------------------------------------------------
void Contest_c::Save ()
{
  Save (_filename);
  Save (_backup);
}

// --------------------------------------------------------------------------------
void Contest_c::Save (gchar *filename)
{
  if (filename)
  {
    xmlTextWriter *xml_writer;

    xml_writer = xmlNewTextWriterFilename (filename, 0);
    if (xml_writer)
    {
      xmlTextWriterSetIndent     (xml_writer,
                                  TRUE);
      xmlTextWriterStartDocument (xml_writer,
                                  NULL,
                                  "UTF-8",
                                  NULL);
      xmlTextWriterStartElement (xml_writer,
                                 BAD_CAST "contest");

      {
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "title",
                                     BAD_CAST _name);
        xmlTextWriterWriteAttribute (xml_writer,
                                     BAD_CAST "backup",
                                     BAD_CAST _backup);
        _schedule->Save (xml_writer);
      }

      // Players
      _players_base->Save (xml_writer);

      xmlTextWriterEndElement (xml_writer);
      xmlTextWriterEndDocument (xml_writer);

      xmlFreeTextWriter (xml_writer);
    }
  }
}

// --------------------------------------------------------------------------------
gchar *Contest_c::GetSaveFileName (gchar *title)
{
  //gchar     *name    = g_strdup_printf ("%s.%s", _name, "csb");
  GtkWidget *chooser;
  char      *filename = NULL;

  chooser = GTK_WIDGET (gtk_file_chooser_dialog_new ("Choisissez un fichier de sauvegarde ...",
                                                     GTK_WINDOW (_glade->GetRootWidget ()),
                                                     GTK_FILE_CHOOSER_ACTION_SAVE,
                                                     GTK_STOCK_CANCEL,
                                                     GTK_RESPONSE_CANCEL,
                                                     GTK_STOCK_SAVE_AS,
                                                     GTK_RESPONSE_ACCEPT,
                                                     NULL));
  gtk_file_chooser_set_do_overwrite_confirmation (GTK_FILE_CHOOSER (chooser),
                                                  true);

  //gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (chooser),
  //name);
  //g_free (name);

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
  }

  gtk_widget_destroy (chooser);

  return filename;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_save_toolbutton_clicked (GtkWidget *widget,
                                                            GdkEvent  *event,
                                                            gpointer  *data)
{
  Contest_c *c = (Contest_c *) g_object_get_data (G_OBJECT (widget),
                                                  "instance");
  c->on_save_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_save_toolbutton_clicked ()
{
  if (_filename == NULL)
  {
    _filename = GetSaveFileName ("Choisissez un fichier de sauvegarde ...");
  }

  if (_filename)
  {
    Save ();
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_properties_toolbutton_clicked (GtkWidget *widget,
                                                                  GdkEvent  *event,
                                                                  gpointer  *data)
{
  Contest_c *c = (Contest_c *) g_object_get_data (G_OBJECT (widget),
                                                  "instance");
  c->on_properties_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_properties_toolbutton_clicked ()
{
  if (_name)
  {
    gtk_entry_set_text (GTK_ENTRY (_glade->GetWidget ("title_entry")),
                        _name);
  }
  if (_backup)
  {
    gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("backupfile_label")),
                        _backup);
  }

  if (gtk_dialog_run (GTK_DIALOG (_properties_dlg)) == GTK_RESPONSE_ACCEPT)
  {
    ReadProperties ();
  }
  gtk_widget_hide (_properties_dlg);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_formula_toolbutton_clicked (GtkWidget *widget,
                                                               GdkEvent  *event,
                                                               gpointer  *data)
{
  Contest_c *c = (Contest_c *) g_object_get_data (G_OBJECT (widget),
                                                  "instance");
  c->on_formula_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_formula_toolbutton_clicked ()
{
  if (gtk_dialog_run (GTK_DIALOG (_formula_dlg)) == GTK_RESPONSE_ACCEPT)
  {
  }
  gtk_widget_hide (_formula_dlg);
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_previous_stage_toolbutton_clicked (GtkWidget *widget,
                                                                      GdkEvent  *event,
                                                                      gpointer  *data)
{
  Contest_c *c = (Contest_c *) g_object_get_data (G_OBJECT (widget),
                                                  "instance");
  c->on_previous_stage_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_previous_stage_toolbutton_clicked ()
{
  _schedule->PreviousStage ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_next_stage_toolbutton_clicked (GtkWidget *widget,
                                                                  GdkEvent  *event,
                                                                  gpointer  *data)
{
  Contest_c *c = (Contest_c *) g_object_get_data (G_OBJECT (widget),
                                                  "instance");
  c->on_next_stage_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_next_stage_toolbutton_clicked ()
{
  _schedule->NextStage ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_backupfile_button_clicked (GtkWidget *widget,
                                                              GdkEvent  *event,
                                                              gpointer  *data)
{
  Contest_c *c = (Contest_c *) g_object_get_data (G_OBJECT (widget),
                                                  "instance");
  c->on_backupfile_button_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_backupfile_button_clicked ()
{
  gchar *filename = GetSaveFileName ("Choisissez un fichier de secours ...");

  if (filename)
  {
    gtk_label_set_text (GTK_LABEL (_glade->GetWidget ("backupfile_label")),
                        filename);
    g_free (filename);
  }
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_contest_close_button_clicked (GtkWidget *widget,
                                                                 GdkEvent  *event,
                                                                 gpointer  *data)
{
  Contest_c *c = (Contest_c *) g_object_get_data (G_OBJECT (widget),
                                                  "instance");
  c->on_contest_close_button_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_contest_close_button_clicked ()
{
  Release ();
}
