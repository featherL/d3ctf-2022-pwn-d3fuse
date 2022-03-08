#include "fs.h"

#include <stdio.h>


static const struct fuse_operations operations = {
	.init = my_init,
	.getattr = my_getattr,
	.open = my_open,
	.opendir = my_opendir,
	.read = my_read,
	.write = my_write,
	.release = my_release,
	.readdir = my_readdir,
	.releasedir = my_releasedir,
	.create = my_create,
	.mkdir = my_mkdir,
	.rmdir = my_rmdir,
	.truncate = my_truncate,
	.unlink = my_unlink,
	.destroy = my_destroy,
	.access = my_access,
	.rename = my_rename,
	.flush = my_flush,
};


int main(int argc, char *argv[])
{
	int ret;
	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);

	ret = fuse_main(args.argc, args.argv, &operations, NULL);
	fuse_opt_free_args(&args);

	return ret;
}
