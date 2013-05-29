int file_info_read (struct fuse_file_info *f_info);
int file_info_write (struct fuse_file_info *f_info, const char *buf, size_t size, off_t offset);
int file_info_truncate (struct fuse_file_info *f_info, off_t size);
int file_info_fsync (struct fuse_file_info *f_info, int datasync);
