//
// Created by xi4oyu on 2022/1/31.
//

#ifndef FS_H
#define FS_H

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

// fuse {
void * my_init(struct fuse_conn_info *conn, struct fuse_config *cfg);
int my_getattr(const char *path, struct stat *st, struct fuse_file_info *fi);
int my_open(const char *path, struct fuse_file_info *fi);
int my_opendir(const char *path, struct fuse_file_info *fi);
int my_read(const char *path, char *buf, size_t size, off_t off, struct fuse_file_info *fi);
int my_write(const char *path, const char *buf, size_t size, off_t off, struct fuse_file_info *fi);
int my_release(const char *path, struct fuse_file_info *fi);
int my_readdir(const char *path, void *buf, fuse_fill_dir_t fillDir, off_t off,
	struct fuse_file_info *fi, enum fuse_readdir_flags flags);
int my_releasedir(const char *path, struct fuse_file_info *fi);
int my_create(const char *path, mode_t mode, struct fuse_file_info *fi);
int my_mkdir(const char *path, mode_t mode);
int my_truncate(const char *path, off_t off, struct fuse_file_info *fi);
int my_rmdir(const char *path);
int my_unlink(const char *path);
void my_destroy(void *param);
int my_access(const char *path, int flags);
int my_rename(const char *oldName, const char *newName, unsigned int flags);
int my_flush(const char *path, struct fuse_file_info *fi);

// }


#endif //FS_H
