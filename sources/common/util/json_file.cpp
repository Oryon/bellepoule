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

#include "json_file.hpp"

// --------------------------------------------------------------------------------
JsonFile::JsonFile (Listener    *listener,
                    const gchar *path)
  : Object ("JsonFile")
{
  _listener = listener;
  _tag      = 0;

  _path = g_strdup (path);
}

// --------------------------------------------------------------------------------
JsonFile::~JsonFile ()
{
  if (_tag)
  {
    g_source_remove (_tag);
  }

  g_free (_path);
}

// --------------------------------------------------------------------------------
gboolean JsonFile::Load ()
{
  gboolean    result = FALSE;
  JsonParser *parser = json_parser_new ();

  if (json_parser_load_from_file (parser,
                                  _path,
                                  NULL))
  {
    JsonReader *reader = json_reader_new (json_parser_get_root (parser));

    result = _listener->ReadJson (reader);

    g_object_unref (reader);
  }

  g_object_unref (parser);

  return result;
}

// --------------------------------------------------------------------------------
void JsonFile::MakeDirty ()
{
  if (_tag == 0)
  {
    _tag = g_timeout_add_seconds (30,
                                  (GSourceFunc) OnTimeout,
                                  this);
  }
}

// --------------------------------------------------------------------------------
void JsonFile::Save ()
{
  JsonBuilder *builder = json_builder_new ();

  _tag = 0;

  {
    json_builder_begin_object (builder);
    _listener->FeedJsonBuilder (builder);
    json_builder_end_object (builder);
  }

  {
    JsonGenerator *generator = json_generator_new ();
    JsonNode      *root      = json_builder_get_root (builder);

    json_generator_set_pretty (generator,
                               TRUE);
    json_generator_set_root (generator, root);

    json_generator_to_file (generator,
                            _path,
                            NULL);

    json_node_free (root);
    g_object_unref (generator);
  }

  g_object_unref (builder);
}

// --------------------------------------------------------------------------------
gboolean JsonFile::OnTimeout (JsonFile *json_file)
{
  json_file->Save ();

  return G_SOURCE_REMOVE;
}
