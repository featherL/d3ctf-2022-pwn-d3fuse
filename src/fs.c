//
// Created by xi4oyu on 2022/1/31.
//

#include "fs.h"
#include "file.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <libgen.h>

static struct File root;

void* my_init(struct fuse_conn_info* conn, struct fuse_config* cfg)
{
	(void )conn;

	cfg->kernel_cache = 1;

	root.name[0] = '\0';

	root.metadata = META_DIRECTORY;
	root.size = 0x10;
	root.entries = malloc(0x10 * sizeof(struct File));
	memset(root.entries, 0, 0x10 * sizeof(struct File));

	return NULL;
}

int my_getattr(const char* path, struct stat* st, struct fuse_file_info* fi)
{
	(void )fi; // fi == NULL

	const struct File *pFile = file_find(&root, path);

	if (!pFile)
		return -ENOENT;

	if (pFile->metadata & META_DIRECTORY) {
		st->st_nlink = 2;
		st->st_mode = S_IFDIR;
	} else {
		st->st_nlink = 1;
		st->st_mode = S_IFREG;
		st->st_size = pFile->size;
	}

	if (pFile->metadata & META_READONLY) {
		st->st_mode |= 0555;
	} else {
		st->st_mode |= 0777;
	}

	return 0;
}

int my_open(const char* path, struct fuse_file_info* fi)
{
	int ret;
	struct File *pFile = file_find(&root, path);
	if (!pFile)
		return -ENOENT;

	if (pFile->metadata & META_DIRECTORY)
		return -EISDIR;

	if ((fi->flags & O_TRUNC) && (ret = file_truncate(pFile, 0)) < 0)
		return ret;

	fi->fh = (uint64_t) pFile;
	return 0;
}

int my_opendir(const char* path, struct fuse_file_info* fi)
{
	struct File *pFile = file_find(&root, path);
	if (!pFile)
		return -ENOENT;

	if (!(pFile->metadata & META_DIRECTORY))
		return -ENOTDIR;

	fi->fh = (uint64_t )pFile;

	return 0;
}

int my_read(const char* path, char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
	(void )path;

	struct File *pFile = (struct File *) fi->fh;

	if (off > pFile->size)
		off = pFile->size;

	if (off + size > pFile->size)
		size = pFile->size - off;

	memcpy(buf, pFile->content + off, size);



	return (int )size;
}

int my_write(const char* path, const char* buf, size_t size, off_t off, struct fuse_file_info* fi)
{
	(void )path;

	struct File *pFile = (struct File *) fi->fh;
	uint32_t newSize;
	void *tmp;

	if (off > pFile->size)
		off = pFile->size;

	newSize = off + size;
	if (newSize > pFile->size) { // 扩容
		tmp = realloc(pFile->content, newSize);
		if (!tmp)
			return -ENOMEM;

		pFile->content = tmp;
		pFile->size = newSize;
	}

	memcpy(pFile->content + off, buf, size);

	return (int )size;
}

int my_release(const char* path, struct fuse_file_info* fi)
{
	(void )path;

	fi->fh = 0;
	return 0;
}

int my_readdir(const char* path,
	void* buf,
	fuse_fill_dir_t fillDir,
	off_t off,
	struct fuse_file_info* fi,
	enum fuse_readdir_flags flags)
{
	(void )path;
	(void )buf;
	(void )flags;

	struct File *pFile = (struct File *)fi->fh;

	for (int i = 0; i < pFile->size; i++) {
		if (pFile->entries[i].name[0] != '\x00') {
			fillDir(buf, pFile->entries[i].name, NULL, 0, 0);
		}
	}

	return 0;
}

int my_releasedir(const char* path, struct fuse_file_info* fi)
{
	(void )path;

	fi->fh = 0;

	return 0;
}

int my_create(const char* path, mode_t mode, struct fuse_file_info* fi)
{
	return create_file(&root, path, 0, (struct File **)&fi->fh);
}

int my_mkdir(const char* path, mode_t mode)
{
	return create_file(&root, path, META_DIRECTORY, NULL);
}


int my_truncate(const char* path, off_t off, struct fuse_file_info* fi)
{
	(void )path;

	struct File *pFile = (struct File *) fi->fh;
	return file_truncate(pFile, off);
}

int my_rmdir(const char* path)
{
	struct File *pFile = file_find(&root, path);
	if (!pFile)
		return -ENOENT;

	file_unlink(pFile);

	return 0;
}

int my_unlink(const char* path)
{
	struct File *pFile = file_find(&root, path);
	if (!pFile)
		return -ENOENT;

	file_unlink(pFile);

	return 0;
}

void my_destroy(void* param)
{
	(void )param;
}

int my_access(const char* path, int flags)
{
	(void )path;
	(void )flags;

	return 0;
}

int my_rename(const char* oldName, const char* newName, unsigned int flags)
{
	(void ) flags;
	int ret;
	char *path1;
	char *name;
	struct File *pOldFile;
	struct File *pNewFile;

	path1 = strdup(newName);
	if (!path1)
		return -ENOMEM;

	name = basename(path1);

	pOldFile = file_find(&root, oldName);
	pNewFile = file_find(&root, newName);

	if (pOldFile == NULL)
		return -ENOENT;

	if (pNewFile == NULL) { // 文件不存在
		if ((ret = create_file(&root, newName, 0, &pNewFile)) < 0) {
			free(path1);
			return ret;
		}
	} else {
		if ((pNewFile->metadata & META_DIRECTORY) && !dir_empty(pNewFile)) { // 非空目录不能覆盖
			free(path1);
			return -ENOTEMPTY;
		}

		free(pNewFile->content);  // 释放原有文件的资源
	}

	memcpy(pNewFile, pOldFile, sizeof(struct File));
	strcpy(pNewFile->name, name);

	file_unlink(pOldFile);

	free(path1);
	return 0;
}

int my_flush(const char* path, struct fuse_file_info* fi)
{
	(void )path;
	(void )fi;

	return 0;
}
