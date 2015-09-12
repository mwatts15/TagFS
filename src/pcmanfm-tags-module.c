#include <libfm/fm.h>
#include <pcmanfm-modules.h>
#include <assert.h>

FM_DEFINE_MODULE(tab_page_status, tagfs_list)

#include "lt.h"
#include "util.h"

static char *tagfs_list_sel_message(FmFileInfoList *files, gint n_files)
{
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
    int chars_left;
    XATTR_TAG_LIST (path_str, tag_name, unused)
    {
        chars_left = buf_end - p;
        if (chars_left > 3)
        {
            ssize_t e = snprintf(p, chars_left, "%s,", tag_name);
            if (chars_left > e)
                p += e;
            else
                p = buf_end - 1;
            assert(*p == 0);
        }
        else
        {
            break;
        }
    } XATTR_TAG_LIST_END

    if (p == after_label)
    {
        buffer[0] = 0;
    }
    else if (p != buf_end - 1)
    {
        *(p - 1) = ')';
    }
    g_free(path_str);
	return buffer;
}

static gboolean tagfs_list_init ()
{
	return TRUE;
}

FmTabPageStatusInit fm_module_init_tab_page_status = {
    .sel_message = tagfs_list_sel_message,
    .init = tagfs_list_init,
};