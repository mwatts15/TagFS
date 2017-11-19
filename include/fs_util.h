#ifndef FS_UTIL_H
#define FS_UTIL_H
#include <glib.h>
#include "params.h"

typedef struct FWrapper FWrapper;

int file_info_read (struct fuse_file_info *f_info, char *buffer, size_t size, off_t offset);
int file_info_write (struct fuse_file_info *f_info, const char *buf, size_t size, off_t offset);
int file_info_truncate (struct fuse_file_info *f_info, off_t size);
int file_info_fsync (struct fuse_file_info *f_info, int datasync);

/** Wrap the user data in an FWrapper.
 *
 * @param data the data to wrap
 */
FWrapper *fwrapper_wrap (void *data);

/** Wrap the user data in an FWrapper.
 *
 * @param data the data to wrap
 * @param data_destroy function for destroying the user data in a call to
 *                     fwapper_destroy
 */
FWrapper *fwrapper_wrap_full (void *data, GDestroyNotify data_destroy);

/** Get the data for the fwrapper */
void *fwrapper_get_data(FWrapper *fw);

/** Get a statbuf for the wrapper  */
struct stat *fwrapper_stat(FWrapper *fw);

/** Destroy the FWrapper object.
 *
 * If a destroy function was passed in with fwrapper_wrap_full, then it will be
 * called to free the user data.
 *
 * @param w the fwrapper to destroy
 */
void fwrapper_destroy (FWrapper *w);


#endif
