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

#include "util/module.hpp"

#include "../stage.hpp"
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
  _chapters   = NULL;
  _page_count = 0;

  GList *current = stage_list;

  while (current)
  {
    Stage  *stage  = (Stage *) current->data;
    Module *module = dynamic_cast <Module *> (stage);

    for (guint view = 1; view < Stage::N_STAGE_VIEW; view++)
    {
      guint module_page_count;

      g_object_set_data (G_OBJECT (operation),
                         "PRINT_STAGE_VIEW",  GUINT_TO_POINTER (view));

      module_page_count = module->PreparePrint (operation,
                                                context);
      if (module_page_count)
      {
        Chapter *chapter = new Chapter (module,
                                        stage->GetFullName (),
                                        (Stage::StageView) view,
                                        _page_count,
                                        module_page_count+1);

        _chapters = g_slist_append (_chapters,
                                    chapter);
        _page_count += module_page_count+1;
      }
    }

    current = g_list_next (current);
  }

  gtk_print_operation_set_n_pages (operation,
                                   _page_count);
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
      g_object_set_data (G_OBJECT (operation),
                         "PRINT_STAGE_VIEW",  GUINT_TO_POINTER (chapter->GetStageView ()));

      g_object_set_data_full (G_OBJECT (operation),
                              "Print::PageName",
                              (void *) module->GetPrintName (),
                              g_free);
      if (page_nr == first_page)
      {
        chapter->DrawHeaderPage (operation,
                                 context);
      }
      else
      {
        module->DrawPage (operation,
                          context,
                          page_nr - first_page-1);
      }
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
