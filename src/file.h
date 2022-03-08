//
// Created by xi4oyu on 2022/1/31.
//

#ifndef FILE_H
#define FILE_H

#include <stdint.h>
#include <aio.h>

#define MAX_NAME 0x20

#define META_DIRECTORY 1
#define META_READONLY	(1<<1)

struct File {
	char name[MAX_NAME];		// 文件名
	uint32_t metadata;			// 属性
	uint32_t size;				// 文件大小，或者目录项数
	union {
		char *content;
		struct File *entries;
	};
};

/**
 * 查找文件
 * @param pDir 目录
 * @param path 路径
 * @return 返回文件结构体，NULL 表示未找到
 */
struct File *file_find(struct File *pDir, const char *path);


/**
 * 截断文件大小
 * @param pFile 目标文件
 * @param off 阶段偏移
 * @return 返回 0 表示成功，返回负数表示出错
 */
int file_truncate(struct File *pFile, off_t off);

/**
 * 删除文件
 * @param pFile 目标文件
 * @return 返回 0 表示成功，返回负数表示出错
 */
int file_unlink(struct File *pFile);

/**
 * 判断目录是否空
 * @param pDir 目标目录
 * @return 返回 1 表示目录为空，反之目录非空
 */
int dir_empty(const struct File *pDir);


/**
 * 创建文件
 * @param pRoot 根目录
 * @param path 文件路径
 * @param metadata 文件属性
 * @param out 用于返回文件结构体指针
 * @return 返回 0 表示成功，返回负数表示失败
 */
int create_file(struct File *pRoot, const char *path, uint32_t metadata, struct File **pOut);

#endif //FILE_H
