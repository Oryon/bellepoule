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

#include <qrencode.h>

#include "flash_code.hpp"

// --------------------------------------------------------------------------------
FlashCode::FlashCode (const gchar *text)
  : Object ("FlashCode")
{
  _text = g_strdup (text);
}

// --------------------------------------------------------------------------------
FlashCode::~FlashCode ()
{
  g_free (_text);
}

// --------------------------------------------------------------------------------
void FlashCode::DestroyPixbuf (guchar   *pixels,
                               gpointer  data)
{
  g_free (pixels);
}

// --------------------------------------------------------------------------------
gchar *FlashCode::GetText ()
{
  return g_strdup (_text);
}

// --------------------------------------------------------------------------------
GdkPixbuf *FlashCode::GetPixbuf (guint pixel_size)
{
  gchar     *text   = GetText ();
  GdkPixbuf *pixbuf = GetPixbuf (text,
                                 pixel_size);

  g_free (text);

  return pixbuf;
}

// --------------------------------------------------------------------------------
GdkPixbuf *FlashCode::GetPixbuf (const gchar *text,
                                 guint        pixel_size)
{
  GdkPixbuf *pixbuf = nullptr;

  {
    QRcode *qr_code;

    qr_code = QRcode_encodeString (text,
                                   0,
                                   QR_ECLEVEL_L,
                                   QR_MODE_8,
                                   1);

    {
      GdkPixbuf   *small_pixbuf;
      const guint  margin     = 4;
      guint        dst_width  = qr_code->width + 2*margin;
      guint        dst_stride = dst_width * 3;
      guchar      *data       = g_new (guchar, dst_width * dst_stride);

      memset (data, 0xFF, dst_width * dst_stride);

      for (gint y = 0; y < qr_code->width; y++)
      {
        guchar *src  = qr_code->data + (y * qr_code->width);
        guchar *dest = data + margin*3 + ((y + margin) * dst_stride);

        for (gint x = 0; x < qr_code->width; x++)
        {
          if (src[x] & 0x1)
          {
            dest[0] = 0x0;
            dest[1] = 0x0;
            dest[2] = 0x0;
          }
          dest += 3;
        }
      }

      small_pixbuf = gdk_pixbuf_new_from_data (data,
                                               GDK_COLORSPACE_RGB,
                                               FALSE,
                                               8,
                                               dst_width,
                                               dst_width,
                                               dst_stride,
                                               DestroyPixbuf,
                                               nullptr);

      pixbuf = gdk_pixbuf_scale_simple (small_pixbuf,
                                        dst_width * pixel_size,
                                        dst_width * pixel_size,
                                        GDK_INTERP_NEAREST);
      g_object_unref (small_pixbuf);
    }
  }

  return pixbuf;
}

// --------------------------------------------------------------------------------
gchar *FlashCode::GetKey256 ()
{
  static const guint32  data_length = 256 / 8;
  GRand                *random      = g_rand_new ();
  gchar                *key         = g_new (gchar, data_length+1);

  for (guint i = 0; i < data_length; i++)
  {
    guint ascii_set = g_rand_int_range (random, 0, 3);

    if (ascii_set == 0)
    {
      key[i] = g_rand_int_range (random,
                                 '0',
                                 '9');
    }
    else if (ascii_set == 1)
    {
      key[i] = g_rand_int_range (random,
                                 'A',
                                 'Z');
    }
    else
    {
      key[i] = g_rand_int_range (random,
                                 'a',
                                 'z');
    }
  }
  key[data_length] = '\0';

  g_rand_free (random);

  return key;
}
