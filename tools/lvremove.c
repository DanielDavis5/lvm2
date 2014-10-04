/*
 * Copyright (C) 2001-2004 Sistina Software, Inc. All rights reserved.
 * Copyright (C) 2004-2014 Red Hat, Inc. All rights reserved.
 *
 * This file is part of LVM2.
 *
 * This copyrighted material is made available to anyone wishing to use,
 * modify, copy, or redistribute it subject to the terms and conditions
 * of the GNU Lesser General Public License v.2.1.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "tools.h"

static int lvremove_single(struct cmd_context *cmd, struct logical_volume *lv,
			   void *handle __attribute__((unused)))
{
	/*
	 * Single force is equivalent to sinle --yes
	 * Even multiple --yes are equivalent to single --force
	 * When we require -ff it cannot be replaces with -f -y
	 */
	force_t force = (force_t) arg_count(cmd, force_ARG)
		? : (arg_is_set(cmd, yes_ARG) ? DONT_PROMPT : PROMPT);

	if (!lv_remove_with_dependencies(cmd, lv, force, 0))
		return_ECMD_FAILED;

	return ECMD_PROCESSED;
}

int lvremove(struct cmd_context *cmd, int argc, char **argv)
{
	if (!argc) {
		log_error("Please enter one or more logical volume paths");
		return EINVALID_CMD_LINE;
	}

	cmd->handles_missing_pvs = 1;

	return process_each_lv(cmd, argc, argv, READ_FOR_UPDATE, NULL,
			       &lvremove_single);
}
