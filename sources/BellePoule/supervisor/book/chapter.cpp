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

#include "util/canvas.hpp"
#include "util/module.hpp"
#include "../stage.hpp"

#include "section.hpp"
#include "chapter.hpp"

// --------------------------------------------------------------------------------
Chapter::Chapter (Module           *module,
                  BookSection      *section,
                  Stage::StageView  stage_view,
                  guint             first_page,
                  guint             page_count)
: Object ("Chapter")
{
  _module     = module;
  _stage_view = stage_view;
  _first_page = first_page;
  _last_page  = _first_page + page_count-1;

  section->Retain ();
  _section = section;

  if (stage_view == Stage::StageView::CLASSIFICATION)
  {
    _name = g_strdup_printf ("%s\n\n"
                             "<span size=\"xx-small\">%s</span>",
                             _section->_title,
                             gettext ("Classification"));
  }
  else
  {
    _name = g_strdup (_section->_title);
  }
}

// --------------------------------------------------------------------------------
Chapter::~Chapter ()
{
  _section->Release ();
  g_free (_name);
}

// --------------------------------------------------------------------------------
guint Chapter::GetFirstPage ()
{
  return _first_page;
}

// --------------------------------------------------------------------------------
guint Chapter::GetLastPage ()
{
  return _last_page;
}

// --------------------------------------------------------------------------------
Module *Chapter::GetModule ()
{
  return _module;
}

// --------------------------------------------------------------------------------
void Chapter::ConfigurePrintOperation (GtkPrintOperation *operation)
{
  g_object_set_data (G_OBJECT (operation),
                     "PRINT_STAGE_VIEW",  GUINT_TO_POINTER (_stage_view));

  _section->Retain ();
  g_object_set_data_full (G_OBJECT (operation),
                          "Print::Data",
                          dynamic_cast <Object *> (_section),
                          (GDestroyNotify) Object::TryToRelease);

  g_object_set_data_full (G_OBJECT (operation),
                          "Print::PageName",
                          (void *) _module->GetPrintName (),
                          g_free);
}

// --------------------------------------------------------------------------------
void Chapter::DrawFrontPage (GtkPrintOperation *operation,
                             GtkPrintContext   *context,
                             Module            *owner)
{
  ConfigurePrintOperation (operation);

  owner->DrawPage (operation,
                   context,
                   1);

  {
    cairo_t *cr = gtk_print_context_get_cairo_context (context);

    cairo_save (cr);

    {
      GooCanvas      *canvas           = Canvas::CreatePrinterCanvas (context);
      cairo_matrix_t *operation_matrix = (cairo_matrix_t *) g_object_get_data (G_OBJECT (operation),
                                                                               "operation_matrix");

      if (operation_matrix)
      {
        cairo_matrix_t own_matrix;
        cairo_matrix_t result_matrix;

        goo_canvas_item_get_transform (goo_canvas_get_root_item (canvas),
                                       &own_matrix);
        cairo_matrix_multiply (&result_matrix,
                               operation_matrix,
                               &own_matrix);
        goo_canvas_item_set_transform (goo_canvas_get_root_item (canvas),
                                       &result_matrix);
      }

      goo_canvas_text_new (goo_canvas_get_root_item (canvas),
                           _name,
                           50.0, 50.0,
                           100.0,
                           GTK_ANCHOR_CENTER,
                           "alignment",  PANGO_ALIGN_CENTER,
                           "fill-color", "black",
                           "font",       BP_FONT "Bold 10.0px",
                           "use-markup", TRUE,
                           NULL);

      goo_canvas_render (canvas,
                         gtk_print_context_get_cairo_context (context),
                         nullptr,
                         1.0);

      gtk_widget_destroy (GTK_WIDGET (canvas));
    }

    cairo_restore (cr);
  }
}

// --------------------------------------------------------------------------------
void Chapter::DrawPage (GtkPrintOperation *operation,
                        GtkPrintContext   *context,
                        gint               page)
{
  ConfigurePrintOperation (operation);

  _module->DrawPage (operation,
                     context,
                     page);
}
