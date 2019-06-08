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

#include "batch.hpp"
#include "batch_panel.hpp"

namespace Marshaller
{
  // --------------------------------------------------------------------------------
  BatchPanel::BatchPanel (GtkTable       *table,
                          GtkRadioButton *radio_group,
                          Batch          *batch,
                          Listener       *listener)
    : Object ("BatchPanel")
  {
    _batch     = batch;
    _listener  = listener;
    _separator = nullptr;

    {
      guint rows;
      guint columns;

      gtk_table_get_size (table,
                          &rows,
                          &columns);
      if (rows >= 1)
      {
        _separator = gtk_hseparator_new ();

        gtk_table_attach (table, _separator,
                          0, 3,
                          rows+0, rows+1,
                          (GtkAttachOptions) GTK_FILL,
                          (GtkAttachOptions) GTK_FILL,
                          0, 0);
        gtk_widget_show (_separator);
      }

      {
        _radio = gtk_radio_button_new_with_label_from_widget (radio_group,
                                                              batch->GetName ());
        g_signal_connect (G_OBJECT (_radio),
                          "toggled",
                          G_CALLBACK (OnToggled),
                          this);
        gtk_table_attach (table, _radio,
                          0, 1,
                          rows+1, rows+3,
                          (GtkAttachOptions) GTK_FILL,
                          (GtkAttachOptions) GTK_FILL,
                          0, 0);
        gtk_widget_show (_radio);
      }

      {
        _label = gtk_label_new ("Full");
        gtk_table_attach (table, _label,
                          1, 2,
                          rows+1, rows+2,
                          (GtkAttachOptions) GTK_FILL,
                          (GtkAttachOptions) (0),
                          0, 0);
        gtk_widget_show (_label);
      }

      {
        _progress = gtk_progress_bar_new ();
        gtk_widget_set_size_request (_progress,
                                     50,
                                     5);
        gtk_table_attach (table, _progress,
                          1, 2,
                          rows+2, rows+3,
                          (GtkAttachOptions) GTK_FILL,
                          (GtkAttachOptions) (0),
                          0, 0);
        gtk_widget_show (_progress);
      }

      {
        _status = gtk_image_new_from_stock (GTK_STOCK_OK,
                                            GTK_ICON_SIZE_MENU);
        gtk_table_attach (table, _status,
                          2, 3,
                          rows+0, rows+2,
                          (GtkAttachOptions) GTK_FILL,
                          (GtkAttachOptions) GTK_FILL,
                          0, 0);
        gtk_widget_show (_status);
      }
    }
  }

  // --------------------------------------------------------------------------------
  BatchPanel::~BatchPanel ()
  {
    GtkContainer *parent = GTK_CONTAINER (gtk_widget_get_parent (_label));

    if (_separator)
    {
      gtk_container_remove (parent,
                            _separator);
    }
    gtk_container_remove (parent,
                          _label);
    gtk_container_remove (parent,
                          _radio);
    gtk_container_remove (parent,
                          _progress);
    gtk_container_remove (parent,
                          _status);
  }

  // --------------------------------------------------------------------------------
  void BatchPanel::SetActive ()
  {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (_radio),
                                  TRUE);
  }

  // --------------------------------------------------------------------------------
  void BatchPanel::Expose (guint expected_jobs,
                           guint ready_jobs)
  {
    if (_separator)
    {
      gtk_widget_show (_separator);
    }
    gtk_widget_show (_label);
    gtk_widget_show (_radio);
    gtk_widget_show (_progress);
    gtk_widget_show (_status);

    {
      GString *text = g_string_new ("<span size=\"small\"");

      if (expected_jobs == ready_jobs)
      {
        g_string_append (text, " font_weight=\"bold\"");
      }

      g_string_append        (text, ">");
      g_string_append_printf (text, "%d/%d", ready_jobs, expected_jobs);
      g_string_append        (text, "</span>");

      gtk_label_set_markup (GTK_LABEL (_label),
                            text->str);

      g_string_free (text,
                     TRUE);
    }

    gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (_progress),
                                   (gdouble) ready_jobs / (gdouble) expected_jobs);
  }

  // --------------------------------------------------------------------------------
  void BatchPanel::Mask ()
  {
    if (_separator)
    {
      gtk_widget_hide (_separator);
    }
    gtk_widget_hide (_label);
    gtk_widget_hide (_radio);
    gtk_widget_hide (_progress);
    gtk_widget_hide (_status);
  }

  // --------------------------------------------------------------------------------
  gboolean BatchPanel::IsVisible ()
  {
    return gtk_widget_get_visible (_label);
  }

  // --------------------------------------------------------------------------------
  void BatchPanel::OnToggled (GtkToggleButton *button,
                              BatchPanel      *panel)
  {
    if (gtk_toggle_button_get_active (button))
    {
      panel->_listener->OnBatchSelected (panel->_batch);
    }
  }
}
