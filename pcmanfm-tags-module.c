/*
 *      pcmanfm-image-size-plugin.c
 *
 *      This file is module for PCManFM.
 *      It can be used with LibFM and PCManFM version 1.2.0 or newer.
 *
 *      Copyright 2014 Alexander Varnin (fenixk19) <fenixk19@mail.ru>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#include <string.h>
#include <libfm/fm.h>
#include <pcmanfm-modules.h>

FM_DEFINE_MODULE(tab_page_status, tagfs_list)
#include "lt.h"
#include "util.h"

static char * tagfs_list_sel_message(FmFileInfoList *files, gint n_files) {
	char *buffer;
    int unused;
    const int buf_size = 128;
    if (n_files > 1)
    {
        return 0;
    }
	FmFileInfo* fi = fm_file_info_list_peek_head(files);
    buffer = g_malloc(buf_size);
    char * path_str = fm_path_to_str(fm_file_info_get_path(fi));
    int label_size = 1;
    buffer[0] = '(';
    char *after_label = buffer + label_size;
    char *buf_end = buffer + buf_size;
    char *p = after_label;
    XATTR_TAG_LIST (path_str, tag_name, unused)
    {
        p = p + snprintf(p, buf_end - p, "%s,", tag_name);
    } XATTR_TAG_LIST_END
    if (p == after_label)
    {
        buffer[0] = 0;
    }
    else
    {
        *(p - 1) = ')';
    }
    g_free(path_str);
	return buffer;
}

static gboolean tagfs_list_init(void) {
	return TRUE;
}

static void tagfs_list_finalize(void) {
}

FmTabPageStatusInit fm_module_init_tab_page_status = {
    .sel_message = tagfs_list_sel_message,
    .init = tagfs_list_init,
    .finalize = tagfs_list_finalize,
};
