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

#include "module.hpp"
#include "stage.hpp"
#include "chapter.hpp"

#include "book.hpp"

// --------------------------------------------------------------------------------
Book::Book ()
: Object ("Book")
{
}

// --------------------------------------------------------------------------------
Book::~Book ()
{
}

// --------------------------------------------------------------------------------
void Book::Prepare (GtkPrintOperation *operation,
                    GtkPrintContext   *context,
                    GList             *stage_list)
{
  GList *current = stage_list;

  g_object_set_data (G_OBJECT (operation), "DEFAULT_PRINT_SETTINGS",  (void *) 1);

  _chapters = NULL;
  _n_pages  = 0;

  while (current)
  {
    Stage  *stage  = (Stage *) current->data;
    Module *module = dynamic_cast <Module *> (stage);
    guint   n;

    n = module->PreparePrint (operation,
                              context);
    if (n)
    {
      Chapter *chapter = new Chapter (module,
                                      _n_pages,
                                      _n_pages + n-1);

      _chapters = g_slist_append (_chapters,
                                  chapter);
      _n_pages += n;
    }

    current = g_list_next (current);
  }

  gtk_print_operation_set_n_pages (operation,
                                   _n_pages);
}

// --------------------------------------------------------------------------------
void Book::Print (GtkPrintOperation *operation,
                 GtkPrintContext   *context,
                 gint               page_nr)
{
  GSList *current = _chapters;

  while (current)
  {
    Chapter *chapter    = (Chapter *) current->data;
    Module  *module     = chapter->GetModule ();
    gint     first_page = chapter->GetFirstPage ();
    gint     last_page  = chapter->GetLastPage ();

    if ((first_page <= page_nr) && (last_page >= page_nr))
    {
      g_object_set_data_full (G_OBJECT (operation),
                              "Print::PageName",
                              (void *) module->GetPrintName (),
                              g_free);
      module->DrawPage (operation,
                        context,
                        page_nr-first_page);
      break;
    }

    current = g_slist_next (current);
  }
}

// --------------------------------------------------------------------------------
void Book::Stop (GtkPrintOperation *operation,
                 GtkPrintContext   *context)
{
  GSList *current = _chapters;

  while (current)
  {
    Chapter *chapter = (Chapter *) current->data;
    Module  *module  = chapter->GetModule ();

    module->OnEndPrint (operation,
                        context);

    current = g_slist_next (current);
  }

  g_slist_free (_chapters);
}
