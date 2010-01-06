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

#include "checkin.hpp"

#include "contest.hpp"

// --------------------------------------------------------------------------------
Contest_c::Contest_c ()
  : Module_c ("contest.glade")
{
  InitInstance ();

  if (gtk_dialog_run (GTK_DIALOG (_properties_dlg)) == GTK_RESPONSE_ACCEPT)
  {
    ReadProperties ();
  }
  gtk_widget_hide (_properties_dlg);

  _schedule->DisplayList ();
}

// --------------------------------------------------------------------------------
Contest_c::Contest_c (gchar *filename)
  : Module_c ("contest.glade")
{
  InitInstance ();

  if (g_path_is_absolute (filename) == FALSE)
  {
    gchar *current_dir = g_get_current_dir ();

    _filename = g_build_filename (current_dir,
                                  filename, NULL);
    g_free (current_dir);
  }
  else
  {
    _filename = g_strdup (filename);
  }

  if (g_file_test (_filename,
                   G_FILE_TEST_IS_REGULAR))
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
      }
      xmlXPathFreeObject  (xml_object);
      xmlXPathFreeContext (xml_context);
    }

    _schedule->Load (doc);

    xmlFreeDoc (doc);
  }
  else
  {
    _schedule->DisplayList ();
  }
}

// --------------------------------------------------------------------------------
Contest_c::~Contest_c ()
{
  g_free (_name);
  g_free (_filename);
  g_free (_backup);

  Object_c::Release (_schedule);

  gtk_widget_destroy (_properties_dlg);
}

// --------------------------------------------------------------------------------
gchar *Contest_c::GetFilename ()
{
  return _filename;
}

// --------------------------------------------------------------------------------
void Contest_c::AddPlayer (Player_c *player)
{
  if (_schedule)
  {
    Checkin *checkin = dynamic_cast <Checkin *> (_schedule->GetStage (0));

    if (checkin)
    {
      checkin->Add (player);
    }
  }
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
  _name     = NULL;
  _filename = NULL;
  _backup   = NULL;

  {
    _schedule = new Schedule_c ();

    Plug (_schedule,
          _glade->GetWidget ("schedule_viewport"),
          GTK_TOOLBAR (_glade->GetWidget ("contest_toolbar")));
  }

  // Properties dialog
  {
    GtkWidget *content_area;

    _properties_dlg = gtk_dialog_new_with_buttons ("New competition",
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
}

// --------------------------------------------------------------------------------
void Contest_c::Init ()
{
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
      }

      _schedule->Save (xml_writer);

      xmlTextWriterEndElement (xml_writer);
      xmlTextWriterEndDocument (xml_writer);

      xmlFreeTextWriter (xml_writer);
    }
  }
}

// --------------------------------------------------------------------------------
gchar *Contest_c::GetSaveFileName (gchar *title)
{
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

  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
  {
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));

    if (filename)
    {
      if (strcmp ((const char *) ".cotcot", (const char *) &filename[strlen (filename) - strlen (".cotcot")]) != 0)
      {
        gchar *with_suffix;

        with_suffix = g_strdup_printf ("%s.cotcot", filename);
        g_free (filename);
        filename = with_suffix;
      }
    }
  }

  gtk_widget_destroy (chooser);

  return filename;
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_save_toolbutton_clicked (GtkWidget *widget,
                                                            Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

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
                                                                  Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

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
                                                               Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

  c->on_formula_toolbutton_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_formula_toolbutton_clicked ()
{
  _schedule->DisplayList ();
}

// --------------------------------------------------------------------------------
extern "C" G_MODULE_EXPORT void on_backupfile_button_clicked (GtkWidget *widget,
                                                              Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

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
                                                                 Object_c  *owner)
{
  Contest_c *c = dynamic_cast <Contest_c *> (owner);

  c->on_contest_close_button_clicked ();
}

// --------------------------------------------------------------------------------
void Contest_c::on_contest_close_button_clicked ()
{
  Release ();
}
