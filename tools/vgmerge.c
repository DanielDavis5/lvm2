/*
 * Copyright (C) 2001  Sistina Software
 *
 * LVM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * LVM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LVM; see the file COPYING.  If not, write to
 * the Free Software Foundation, 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#include "tools.h"

int vgmerge_single(const char *vg_name_to, const char *vg_name_from);

int vgmerge(int argc, char **argv)
{
	char *vg_name_to;
	int opt = 0;
	int ret = 0, ret_max = 0;

	if (argc < 2) {
		log_error("Please enter 2 or more volume groups to merge");
		return EINVALID_CMD_LINE;
	}

	vg_name_to = argv[0];
	argc--;
	argv++;

	for (; opt < argc; opt++) {
		ret = vgmerge_single(vg_name_to, argv[opt]);
		if (ret > ret_max)
			ret_max = ret;
	}

	return ret_max;
}

int vgmerge_single(const char *vg_name_to, const char *vg_name_from)
{
	struct volume_group *vg_to, *vg_from;
	struct list *lvh1, *lvh2;

	if (!strcmp(vg_name_to, vg_name_from)) {
		log_error("Duplicate volume group name %s", vg_name_from);
		return ECMD_FAILED;
	}

	log_verbose("Checking for volume group %s", vg_name_to);
	if (!(vg_to = fid->ops->vg_read(fid, vg_name_to))) {
		log_error("Volume group %s doesn't exist", vg_name_to);
		return ECMD_FAILED;
	}

	log_verbose("Checking for volume group %s", vg_name_from);
	if (!(vg_from = fid->ops->vg_read(fid, vg_name_from))) {
		log_error("Volume group %s doesn't exist", vg_name_from);
		return ECMD_FAILED;
	}

	/* FIXME status - confirm no active LVs? */
	if (vg_from->status & ACTIVE) {
		log_error("Volume group %s must be inactive", vg_name_from);
		return ECMD_FAILED;
	}

	/* Check compatibility */
	if (vg_to->extent_size != vg_from->extent_size) {
		log_error("Extent sizes differ: %d (%s) and %d (%s)",
			  vg_to->extent_size, vg_to->name,
			  vg_from->extent_size, vg_from->name);
		return ECMD_FAILED;
	}

	if (vg_to->max_pv < vg_to->pv_count + vg_from->pv_count) {
		log_error("Maximum number of physical volumes (%d) exceeded "
			  " for %s and %s", vg_to->max_pv, vg_to->name,
			  vg_from->name);
		return ECMD_FAILED;
	}

	if (vg_to->max_lv < vg_to->lv_count + vg_from->lv_count) {
		log_error("Maximum number of logical volumes (%d) exceeded "
			  " for %s and %s", vg_to->max_lv, vg_to->name,
			  vg_from->name);
		return ECMD_FAILED;
	}

	/* Check no conflicts with LV names */
	list_iterate(lvh1, &vg_to->lvs) {
		list_iterate(lvh2, &vg_from->lvs) {
			char *name1 = list_item(lvh1, struct lv_list)->lv.name;
			char *name2 = list_item(lvh2, struct lv_list)->lv.name;
			if (!strcmp(name1, name2)) {
				log_error("Duplicate logical volume name %s "
					  "in %s and %s", name1, vg_to->name,
					  vg_from->name);
				return ECMD_FAILED;
			}
		}
	}

	/* FIXME List arg: vg_show_with_pv_and_lv(vg_to); */

	if (!archive(vg_from) || !archive(vg_to))
		return ECMD_FAILED;

	/* Merge volume groups */
	while (!list_empty(&vg_from->pvs)) {
		struct list *pvh = vg_from->pvs.n;
		struct physical_volume *pv;

		list_del(pvh);
		list_add(&vg_to->pvs, pvh);

		pv = &list_item(pvh, struct pv_list)->pv;
		pv->vg_name = pool_strdup(fid->cmd->mem, vg_to->name);
	}
	vg_to->pv_count += vg_from->pv_count;

	while (!list_empty(&vg_from->lvs)) {
		struct list *lvh = vg_from->lvs.n;

		list_del(lvh);
		list_add(&vg_to->lvs, lvh);
	}
	vg_to->lv_count += vg_from->lv_count;

	vg_to->extent_count += vg_from->extent_count;
	vg_to->free_count += vg_from->free_count;

	/* store it on disks */
	log_verbose("Writing out updated volume group");
	if (!(fid->ops->vg_write(fid, vg_to))) {
		return ECMD_FAILED;
	}

	/* FIXME Remove /dev/vgfrom */

	backup(vg_to);

	log_print("Volume group %s successfully merged into %s",
		  vg_from->name, vg_to->name);
	return 0;
}
