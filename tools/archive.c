/*
 * Copyright (C) 2001 Sistina Software (UK) Limited.
 *
 * This file is released under the GPL.
 */

#include "log.h"
#include "archive.h"
#include "dbg_malloc.h"
#include "format-text.h"
#include "lvm-string.h"
#include "toollib.h"

#include <limits.h>

static struct {
	int enabled;
	char *dir;
	unsigned int keep_days;
	unsigned int keep_number;

} _archive_params;

int archive_init(const char *dir,
		 unsigned int keep_days, unsigned int keep_min)
{
	_archive_params.dir = NULL;

	if (!*dir)
		return 1;

	if (!create_dir(dir))
		return 0;

	if (!(_archive_params.dir = dbg_strdup(dir))) {
		log_error("Couldn't copy archive directory name.");
		return 0;
	}

	_archive_params.keep_days = keep_days;
	_archive_params.keep_number = keep_min;
	_archive_params.enabled = 1;
	return 1;
}

void archive_exit(void)
{
	if (_archive_params.dir)
		dbg_free(_archive_params.dir);
	memset(&_archive_params, 0, sizeof(_archive_params));
}

void archive_enable(int flag)
{
	_archive_params.enabled = flag;
}

static int __archive(struct volume_group *vg)
{
	int r;
	struct format_instance *archiver;

	if (!(archiver = backup_format_create(vg->cmd,
					      _archive_params.dir,
					      _archive_params.keep_days,
					      _archive_params.keep_number))) {
		log_error("Couldn't create archiver object.");
		return 0;
	}

	if (!(r = archiver->ops->vg_write(archiver, vg)))
		stack;

	archiver->ops->destroy(archiver);
	return r;
}

int archive(struct volume_group *vg)
{
	if (!_archive_params.enabled || !_archive_params.dir)
		return 1;

	if (test_mode()) {
		log_print("Test mode: Skipping archiving of volume group.");
		return 1;
	}

	log_verbose("Archiving volume group %s metadata.", vg->name);
	if (!__archive(vg)) {
		log_error("Volume group %s metadata archive failed.", 
			  vg->name);
		return 0;
	}

	return 1;
}



static struct {
	int enabled;
	char *dir;

} _backup_params;

int backup_init(const char *dir)
{
	_backup_params.dir = NULL;
	if (!*dir)
		return 1;

	if (!create_dir(dir))
		return 0;

	if (!(_backup_params.dir = dbg_strdup(dir))) {
		log_error("Couldn't copy backup directory name.");
		return 0;
	}

	return 1;
}

void backup_exit(void)
{
	if (_backup_params.dir)
		dbg_free(_backup_params.dir);
	memset(&_backup_params, 0, sizeof(_backup_params));
}

void backup_enable(int flag)
{
	_backup_params.enabled = flag;
}

static int __backup(struct volume_group *vg)
{
	int r;
	struct format_instance *tf;
	char name[PATH_MAX];

	if (lvm_snprintf(name, sizeof(name), "%s/%s",
			 _backup_params.dir, vg->name) < 0) {
		log_error("Failed to generate volume group metadata backup "
			  "filename.");
		return 0;
	}

	log_verbose("Creating volume group backup %s", name);

	if (!(tf = text_format_create(vg->cmd, name))) {
		stack;
		return 0;
	}

	if (!(r = tf->ops->vg_write(tf, vg)))
		stack;

	tf->ops->destroy(tf);
	return r;
}

int backup(struct volume_group *vg)
{
	if (!_backup_params.enabled || !_backup_params.dir) {
		log_print("WARNING: This metadata update is NOT backed up");
		return 1;
	}

	if (test_mode()) {
		log_print("Test mode: Skipping volume group backup.");
		return 1;
	}

	if (!__backup(vg)) {
		log_error("Backup of volume group %s metadata failed.",
			  vg->name);
		return 0;
	}

	return 1;
}
