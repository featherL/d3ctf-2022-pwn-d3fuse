//
// Created by xi4oyu on 2022/1/31.
//

#include "file.h"

#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>

struct File *file_find(struct File *pDir, const char *path)
{
	char *tmp;
	char *name;
	char *last;
	struct File *cur = pDir;
	struct File *next = NULL;

	if (!strcmp("/", path))
		return pDir;

	tmp = strdup(path);
	if (!tmp)
		return NULL;

	name = strtok_r(tmp, "/", &last);

	while (name != NULL) {
		int i;
		for (i = 0; i < cur->size; i++) {
			if (!strcmp(cur->entries[i].name, name)) {
				next = &cur->entries[i];
				break;
			}
		}

		if (i == cur->size) { // 没找到
			free(tmp);
			return NULL;
		}

		cur = next;
		name = strtok_r(NULL, "/", &last);
	}

	free(tmp);
	return cur;
}


int file_truncate(struct File* pFile, off_t off)
{
	void *tmp;

	if (off == 0) {
		free(pFile->content);
		pFile->content = NULL;
		pFile->size = 0;
		return 0;
	}

	tmp = realloc(pFile->content, off);
	if (!tmp)
		return -ENOMEM;

	pFile->content = tmp;
	pFile->size = off;

	return 0;
}


int file_unlink(struct File* pFile)
{
	pFile->name[0] = '\0';
	free(pFile->content);

	pFile->content = NULL;

	return 0;
}

int dir_empty(const struct File *pDir)
{
	for (int i = 0; i < pDir->size; i++) {
		if (pDir->entries[i].name[0] != '\0')
			return 0;
	}

	return 1;
}


int create_file(struct File *pRoot, const char* path, uint32_t metadata, struct File** pOut)
{
	int result;
	char *path1;
	char *path2;
	char *parent;
	char *name;
	void *tmp;
	struct File *lpDir;
	uint32_t newSize;

	result = 0;

	path1 = strdup(path);
	if (!path1) {
		result =  -ENOMEM;
		goto ret;
	}

	path2 = strdup(path);
	if (!path2) {
		result = -ENOMEM;
		goto err1;
	}

	parent = dirname(path1);
	name = basename(path2);

	// if (strlen(name) + 1 >= MAX_NAME) {
	// 	result = -ENOMEM;
	// 	goto err2;
	// }

	lpDir = file_find(pRoot, parent);
	if (!lpDir) {
		result = -ENOENT;
		goto err2;
	}

	int idx = -1;
	for (int i = 0; i < lpDir->size; i++) {
		if (!(strcmp(lpDir->entries[i].name, name))) {
			result = -EEXIST;
			goto err2;
		}

		if (idx < 0 && lpDir->entries[i].name[0] == '\0') {
			idx = i;
		}
	}

	if (idx < 0) { // 没有空目录项
		newSize = lpDir->size + 0x10;
		tmp = realloc(lpDir->entries, newSize * sizeof(struct File));
		if (!tmp) {
			result = -ENOMEM;
			goto err2;
		}

		memset(tmp + lpDir->size * sizeof(struct File), 0, 0x10 * sizeof(struct File));

		idx = lpDir->size;
		lpDir->size = newSize;
		lpDir->entries = tmp;
	}

	memset(&lpDir->entries[idx], 0, sizeof(struct File));
	strcpy(lpDir->entries[idx].name, name);
	lpDir->entries[idx].metadata = metadata;

	if (pOut != NULL)
		*pOut = &lpDir->entries[idx];

err2:
	free(path2);
err1:
	free(path1);
ret:
	return result;
}
