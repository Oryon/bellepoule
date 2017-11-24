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
#include "section.hpp"
#include "chapter.hpp"

#include "book.hpp"

// --------------------------------------------------------------------------------
Book::Book (Module *owner)
  : Object ("Book")
{
  _owner = owner;
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
  GList *current_stage = stage_list;

  _chapters   = NULL;
  _page_count = 1;

  while (current_stage)
  {
    Stage  *stage  = (Stage *) current_stage->data;
    Module *module = dynamic_cast <Module *> (stage);

    for (guint view = 1; view < Stage::N_STAGE_VIEW; view++)
    {
      GList *sections        = stage->GetBookSections ((Stage::StageView) view);
      GList *current_section = sections;

      g_object_set_data (G_OBJECT (operation),
                         "PRINT_STAGE_VIEW",  GUINT_TO_POINTER (view));

      while (current_section)
      {
        BookSection *section            = (BookSection *) current_section->data;
        guint        section_page_count;

        section->Retain ();
        g_object_set_data_full (G_OBJECT (operation),
                                "Print::Data", dynamic_cast <Object *> (section),
                                (GDestroyNotify) Object::TryToRelease);

        section_page_count = module->PreparePrint (operation,
                                                   context);
        if (section_page_count)
        {
          Chapter *chapter = new Chapter (module,
                                          section,
                                          (Stage::StageView) view,
                                          _page_count,
                                          section_page_count+1);

          _chapters = g_list_append (_chapters,
                                     chapter);
          _page_count += section_page_count+1;
        }

        section->Release ();
        current_section = g_list_next (current_section);
      }

      g_list_free (sections);
    }

    current_stage = g_list_next (current_stage);
  }

  gtk_print_operation_set_n_pages (operation,
                                   _page_count);
}

// --------------------------------------------------------------------------------
void Book::Print (GtkPrintOperation *operation,
                  GtkPrintContext   *context,
                  gint               page_nr)
{
  if (page_nr == 0)
  {
    g_object_set_data_full (G_OBJECT (operation),
                            "Print::PageName", (void *) g_strdup (gettext ("Formula")),
                            g_free);
    _owner->DrawPage (operation,
                      context,
                      page_nr);
  }
  else
  {
    GList *current = _chapters;

    while (current)
    {
      Chapter *chapter    = (Chapter *) current->data;
      gint     first_page = chapter->GetFirstPage ();
      gint     last_page  = chapter->GetLastPage ();

      if ((first_page <= page_nr) && (last_page >= page_nr))
      {
        if (page_nr == first_page)
        {
          chapter->DrawFrontPage (operation,
                                  context,
                                  _owner);
        }
        else
        {
          chapter->DrawPage (operation,
                             context,
                             page_nr - first_page-1);
        }
        break;
      }

      current = g_list_next (current);
    }
  }
}

// --------------------------------------------------------------------------------
void Book::Stop (GtkPrintOperation *operation,
                 GtkPrintContext   *context)
{
  GList *current = _chapters;

  while (current)
  {
    Chapter *chapter = (Chapter *) current->data;
    Module  *module  = chapter->GetModule ();

    module->OnEndPrint (operation,
                        context);

    chapter->Release ();
    current = g_list_next (current);
  }

  g_list_free (_chapters);
}
