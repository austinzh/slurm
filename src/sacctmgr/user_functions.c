/*****************************************************************************\
 *  user_functions.c - functions dealing with users in the accounting system.
 *****************************************************************************
 *  Copyright (C) 2008 Lawrence Livermore National Security.
 *  Copyright (C) 2002-2007 The Regents of the University of California.
 *  Produced at Lawrence Livermore National Laboratory (cf, DISCLAIMER).
 *  Written by Danny Auble <da@llnl.gov>
 *  CODE-OCEC-09-009. All rights reserved.
 *
 *  This file is part of SLURM, a resource management program.
 *  For details, see <https://computing.llnl.gov/linux/slurm/>.
 *  Please also read the included file: DISCLAIMER.
 *
 *  SLURM is free software; you can redistribute it and/or modify it under
 *  the terms of the GNU General Public License as published by the Free
 *  Software Foundation; either version 2 of the License, or (at your option)
 *  any later version.
 *
 *  In addition, as a special exception, the copyright holders give permission
 *  to link the code of portions of this program with the OpenSSL library under
 *  certain conditions as described in each individual source file, and
 *  distribute linked combinations including the two. You must obey the GNU
 *  General Public License in all respects for all of the code used other than
 *  OpenSSL. If you modify file(s) with this exception, you may extend this
 *  exception to your version of the file(s), but you are not obligated to do
 *  so. If you do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source files in
 *  the program, then also delete it here.
 *
 *  SLURM is distributed in the hope that it will be useful, but WITHOUT ANY
 *  WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 *  FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 *  details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with SLURM; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA.
\*****************************************************************************/

#include "src/sacctmgr/sacctmgr.h"
#include "src/common/assoc_mgr.h"
#include "src/common/uid.h"

static int _set_cond(int *start, int argc, char *argv[],
		     slurmdb_user_cond_t *user_cond,
		     List format_list)
{
	int i;
	int u_set = 0;
	int a_set = 0;
	int end = 0;
	slurmdb_association_cond_t *assoc_cond = NULL;
	int command_len = 0;
	int option = 0;

	if(!user_cond) {
		error("No user_cond given");
		return -1;
	}

	if(!user_cond->assoc_cond)
		user_cond->assoc_cond =
			xmalloc(sizeof(slurmdb_association_cond_t));

	assoc_cond = user_cond->assoc_cond;

	/* we need this to make sure we only change users, not
	 * accounts if this list didn't exist it would change
	 * accounts. Having it blank is fine, it just needs to
	 * exist.
	 */
	if(!assoc_cond->user_list)
		assoc_cond->user_list = list_create(slurm_destroy_char);

	for (i=(*start); i<argc; i++) {
		end = parse_option_end(argv[i]);
		if(!end)
			command_len=strlen(argv[i]);
		else {
			command_len=end-1;
			if(argv[i][end] == '=') {
				option = (int)argv[i][end-1];
				end++;
			}
		}

		if (!strncasecmp(argv[i], "Set", MAX(command_len, 3))) {
			i--;
			break;
		} else if (!end && !strncasecmp(argv[i], "WithAssoc",
						 MAX(command_len, 5))) {
			user_cond->with_assocs = 1;
		} else if (!end &&
			   !strncasecmp(argv[i], "WithCoordinators",
					 MAX(command_len, 5))) {
			user_cond->with_coords = 1;
		} else if (!end &&
			   !strncasecmp(argv[i], "WithDeleted",
					 MAX(command_len, 5))) {
			user_cond->with_deleted = 1;
			assoc_cond->with_deleted = 1;
		} else if (!end &&
			   !strncasecmp(argv[i], "WithRawQOSLevel",
					 MAX(command_len, 5))) {
			assoc_cond->with_raw_qos = 1;
		} else if (!end && !strncasecmp(argv[i], "WOPLimits",
						 MAX(command_len, 4))) {
			assoc_cond->without_parent_limits = 1;
		} else if(!end && !strncasecmp(argv[i], "where",
					       MAX(command_len, 5))) {
			continue;
		} else if(!end
			  || !strncasecmp(argv[i], "Names",
					   MAX(command_len, 1))
			  || !strncasecmp(argv[i], "Users",
					   MAX(command_len, 1))) {
			if(slurm_addto_char_list(assoc_cond->user_list,
						 argv[i]+end))
				u_set = 1;
			else
				exit_code=1;
		} else if (!strncasecmp(argv[i], "AdminLevel",
					 MAX(command_len, 2))) {
			user_cond->admin_level =
				str_2_slurmdb_admin_level(argv[i]+end);
			u_set = 1;
		} else if (!strncasecmp(argv[i], "DefaultAccount",
					 MAX(command_len, 8))) {
			if(!user_cond->def_acct_list) {
				user_cond->def_acct_list =
					list_create(slurm_destroy_char);
			}
			if(slurm_addto_char_list(user_cond->def_acct_list,
						 argv[i]+end))
				u_set = 1;
			else
				exit_code=1;
		} else if (!strncasecmp(argv[i], "DefaultWCKey",
					 MAX(command_len, 8))) {
			if(!user_cond->def_wckey_list) {
				user_cond->def_wckey_list =
					list_create(slurm_destroy_char);
			}
			if(slurm_addto_char_list(user_cond->def_wckey_list,
						 argv[i]+end))
				u_set = 1;
			else
				exit_code=1;
		} else if (!strncasecmp(argv[i], "Format",
					 MAX(command_len, 1))) {
			if(format_list)
				slurm_addto_char_list(format_list, argv[i]+end);
		} else if(!(a_set = sacctmgr_set_association_cond(
				    assoc_cond, argv[i], argv[i]+end,
				    command_len))) {
			exit_code=1;
			fprintf(stderr, " Unknown condition: %s\n"
				" Use keyword 'set' to modify value\n",
				argv[i]);
		}
	}

	(*start) = i;

	if(u_set && a_set)
		return 3;
	else if(a_set) {
		return 2;
	} else if(u_set)
		return 1;

	return 0;
}

static int _set_rec(int *start, int argc, char *argv[],
		    slurmdb_user_rec_t *user,
		    slurmdb_association_rec_t *assoc)
{
	int i;
	int u_set = 0;
	int a_set = 0;
	int end = 0;
	int command_len = 0;
	int option = 0;

	for (i=(*start); i<argc; i++) {
		end = parse_option_end(argv[i]);
		if(!end)
			command_len=strlen(argv[i]);
		else {
			command_len=end-1;
			if(argv[i][end] == '=') {
				option = (int)argv[i][end-1];
				end++;
			}
		}

		if (!strncasecmp(argv[i], "Where", MAX(command_len, 5))) {
			i--;
			break;
		} else if(!end && !strncasecmp(argv[i], "set",
					       MAX(command_len, 3))) {
			continue;
		} else if(!end) {
			exit_code=1;
			fprintf(stderr,
				" Bad format on %s: End your option with "
				"an '=' sign\n", argv[i]);
		} else if (!strncasecmp(argv[i], "AdminLevel",
					 MAX(command_len, 2))) {
			user->admin_level =
				str_2_slurmdb_admin_level(argv[i]+end);
			u_set = 1;
		} else if (!strncasecmp(argv[i], "DefaultAccount",
					 MAX(command_len, 8))) {
			if(user->default_acct)
				xfree(user->default_acct);
			user->default_acct = strip_quotes(argv[i]+end, NULL, 1);
			u_set = 1;
		} else if (!strncasecmp(argv[i], "DefaultWCKey",
					 MAX(command_len, 8))) {
			if(user->default_wckey)
				xfree(user->default_wckey);
			user->default_wckey =
				strip_quotes(argv[i]+end, NULL, 1);
			u_set = 1;
		} else if (!strncasecmp(argv[i], "NewName",
					 MAX(command_len, 1))) {
			if(user->name)
				xfree(user->name);
			user->name = strip_quotes(argv[i]+end, NULL, 1);
			u_set = 1;
		} else if (!strncasecmp (argv[i], "RawUsage",
					 MAX(command_len, 7))) {
			uint32_t usage;
			if(!assoc)
				continue;
			assoc->usage = xmalloc(sizeof(
						assoc_mgr_association_usage_t));
			if (get_uint(argv[i]+end, &usage,
				     "RawUsage") == SLURM_SUCCESS) {
				assoc->usage->usage_raw = usage;
				a_set = 1;
			}
		} else if(!assoc ||
			  (assoc && !(a_set = sacctmgr_set_association_rec(
					      assoc, argv[i], argv[i]+end,
					      command_len, option)))) {
			exit_code=1;
			fprintf(stderr, " Unknown option: %s\n"
				" Use keyword 'where' to modify condition\n",
				argv[i]);
		}
	}

	(*start) = i;

	if(u_set && a_set)
		return 3;
	else if(u_set)
		return 1;
	else if(a_set)
		return 2;
	return 0;
}

/*
 * IN: user_cond - used for the assoc_cond pointing to the user and
 *     account list
 * IN: check - whether or not to check if the existance of the above lists
 */
static int _check_coord_request(slurmdb_user_cond_t *user_cond, bool check)
{
	ListIterator itr = NULL, itr2 = NULL;
	char *name = NULL;
	slurmdb_user_rec_t *user_rec = NULL;
	slurmdb_account_rec_t *acct_rec = NULL;
	slurmdb_account_cond_t account_cond;
	List local_acct_list = NULL;
	List local_user_list = NULL;
	int rc = SLURM_SUCCESS;

	if(!user_cond) {
		exit_code=1;
		fprintf(stderr, " You need to specify the user_cond here.\n");
		return SLURM_ERROR;
	}

	if(check && (!user_cond->assoc_cond->user_list
		     || !list_count(user_cond->assoc_cond->user_list))) {
		exit_code=1;
		fprintf(stderr, " You need to specify a user list here.\n");
		return SLURM_ERROR;
	}

	if(check && (!user_cond->assoc_cond->acct_list
		     || !list_count(user_cond->assoc_cond->acct_list))) {
		exit_code=1;
		fprintf(stderr, " You need to specify a account list here.\n");
		return SLURM_ERROR;
	}

	memset(&account_cond, 0, sizeof(slurmdb_account_cond_t));
	account_cond.assoc_cond = user_cond->assoc_cond;
	local_acct_list =
		acct_storage_g_get_accounts(db_conn, my_uid, &account_cond);
	if(!local_acct_list) {
		exit_code=1;
		fprintf(stderr, " Problem getting accounts from database.  "
			"Contact your admin.\n");
		return SLURM_ERROR;
	}

	if(user_cond->assoc_cond->acct_list &&
	   (list_count(local_acct_list) !=
	    list_count(user_cond->assoc_cond->acct_list))) {

		itr = list_iterator_create(user_cond->assoc_cond->acct_list);
		itr2 = list_iterator_create(local_acct_list);

		while((name = list_next(itr))) {
			while((acct_rec = list_next(itr2))) {
				if(!strcmp(name, acct_rec->name))
					break;
			}
			list_iterator_reset(itr2);
			if(!acct_rec) {
				fprintf(stderr,
					" You specified a non-existant "
					"account '%s'.\n", name);
				exit_code=1;
				rc = SLURM_ERROR;
			}
		}
		list_iterator_destroy(itr);
		list_iterator_destroy(itr2);
	}

	local_user_list = acct_storage_g_get_users(db_conn, my_uid, user_cond);
	if(!local_user_list) {
		exit_code=1;
		fprintf(stderr, " Problem getting users from database.  "
			"Contact your admin.\n");
		if(local_acct_list)
			list_destroy(local_acct_list);
		return SLURM_ERROR;
	}

	if(user_cond->assoc_cond->user_list &&
	   (list_count(local_user_list) !=
	    list_count(user_cond->assoc_cond->user_list))) {

		itr = list_iterator_create(user_cond->assoc_cond->user_list);
		itr2 = list_iterator_create(local_user_list);

		while((name = list_next(itr))) {
			while((user_rec = list_next(itr2))) {
				if(!strcmp(name, user_rec->name))
					break;
			}
			list_iterator_reset(itr2);
			if(!user_rec) {
				fprintf(stderr,
					" You specified a non-existant "
					"user '%s'.\n", name);
				exit_code=1;
				rc = SLURM_ERROR;
			}
		}
		list_iterator_destroy(itr);
		list_iterator_destroy(itr2);
	}

	if(local_acct_list)
		list_destroy(local_acct_list);
	if(local_user_list)
		list_destroy(local_user_list);

	return rc;
}

static int _check_user_has_acct(char *user, char *acct)
{
	slurmdb_association_cond_t assoc_cond;
	List ret_list = NULL;

	memset(&assoc_cond, 0, sizeof(slurmdb_association_cond_t));
	assoc_cond.acct_list = list_create(NULL);
	list_push(assoc_cond.acct_list, acct);
	assoc_cond.user_list = list_create(NULL);
	list_push(assoc_cond.user_list, user);
	ret_list = acct_storage_g_get_associations(db_conn, my_uid,
						   &assoc_cond);

	if(ret_list && (list_count(ret_list)))
		return 1;

	return 0;
}

extern int sacctmgr_add_user(int argc, char *argv[])
{
	int rc = SLURM_SUCCESS;
	int i=0;
	ListIterator itr = NULL;
	ListIterator itr_a = NULL;
	ListIterator itr_c = NULL;
	ListIterator itr_p = NULL;
	ListIterator itr_w = NULL;
	slurmdb_user_rec_t *user = NULL;
	slurmdb_association_rec_t *assoc = NULL;
	slurmdb_association_rec_t start_assoc;
	char *default_acct = NULL;
	char *default_wckey = NULL;
	slurmdb_association_cond_t *assoc_cond = NULL;
	slurmdb_wckey_rec_t *wckey = NULL;
	slurmdb_wckey_cond_t *wckey_cond = NULL;
	slurmdb_admin_level_t admin_level = SLURMDB_ADMIN_NOTSET;
	char *name = NULL, *account = NULL, *cluster = NULL, *partition = NULL;
	int partition_set = 0;
	List user_list = NULL;
	List assoc_list = NULL;
	List wckey_list = NULL;
	List local_assoc_list = NULL;
	List local_acct_list = NULL;
	List local_user_list = NULL;
	List local_wckey_list = NULL;
	char *user_str = NULL;
	char *assoc_str = NULL;
	char *wckey_str = NULL;
	int limit_set = 0;
	int first = 1;
	int acct_first = 1;
	int command_len = 0;
	int option = 0;
	uint16_t track_wckey = slurm_get_track_wckey();

/* 	if(!list_count(sacctmgr_cluster_list)) { */
/* 		printf(" Can't add users, no cluster defined yet.\n" */
/* 		       " Please contact your administrator.\n"); */
/* 		return SLURM_ERROR; */
/* 	} */
	slurmdb_init_association_rec(&start_assoc);

	assoc_cond = xmalloc(sizeof(slurmdb_association_cond_t));

	assoc_cond->user_list = list_create(slurm_destroy_char);
	assoc_cond->acct_list = list_create(slurm_destroy_char);
	assoc_cond->cluster_list = list_create(slurm_destroy_char);
	assoc_cond->partition_list = list_create(slurm_destroy_char);

	wckey_cond = xmalloc(sizeof(slurmdb_wckey_cond_t));

	wckey_cond->name_list = list_create(slurm_destroy_char);

	for (i=0; i<argc; i++) {
		int end = parse_option_end(argv[i]);
		if(!end)
			command_len=strlen(argv[i]);
		else {
			command_len=end-1;
			if(argv[i][end] == '=') {
				option = (int)argv[i][end-1];
				end++;
			}
		}

		if(!end
		   || !strncasecmp(argv[i], "Names", MAX(command_len, 1))
		   || !strncasecmp(argv[i], "Users", MAX(command_len, 1))) {
			if(!slurm_addto_char_list(assoc_cond->user_list,
						 argv[i]+end))
				exit_code=1;
		} else if (!strncasecmp(argv[i], "AdminLevel",
					 MAX(command_len, 2))) {
			admin_level = str_2_slurmdb_admin_level(argv[i]+end);
		} else if (!strncasecmp(argv[i], "DefaultAccount",
					 MAX(command_len, 8))) {
			if(default_acct) {
				fprintf(stderr,
					" Already listed DefaultAccount %s\n",
					default_acct);
				exit_code = 1;
				continue;
			}
			default_acct = strip_quotes(argv[i]+end, NULL, 1);
			slurm_addto_char_list(assoc_cond->acct_list,
					      default_acct);
		} else if (!strncasecmp(argv[i], "DefaultWCKey",
					 MAX(command_len, 8))) {
			if(default_wckey) {
				fprintf(stderr,
					" Already listed DefaultWCKey %s\n",
					default_wckey);
				exit_code = 1;
				continue;
			}
			default_wckey = strip_quotes(argv[i]+end, NULL, 1);
			slurm_addto_char_list(wckey_cond->name_list,
					      default_wckey);
		} else if (!strncasecmp(argv[i], "WCKeys",
					 MAX(command_len, 1))) {
			slurm_addto_char_list(wckey_cond->name_list,
					      argv[i]+end);
		} else if(!(limit_set = sacctmgr_set_association_rec(
				    &start_assoc, argv[i], argv[i]+end,
				    command_len, option))
			  && !(limit_set = sacctmgr_set_association_cond(
				       assoc_cond, argv[i], argv[i]+end,
				       command_len))) {
			exit_code=1;
			fprintf(stderr, " Unknown option: %s\n", argv[i]);
		}
	}

	if(exit_code) {
		slurmdb_destroy_wckey_cond(wckey_cond);
		slurmdb_destroy_association_cond(assoc_cond);
		return SLURM_ERROR;
	} else if(!list_count(assoc_cond->user_list)) {
		slurmdb_destroy_wckey_cond(wckey_cond);
		slurmdb_destroy_association_cond(assoc_cond);
		exit_code=1;
		fprintf(stderr, " Need name of user to add.\n");
		return SLURM_ERROR;
	} else {
 		slurmdb_user_cond_t user_cond;

		memset(&user_cond, 0, sizeof(slurmdb_user_cond_t));
		user_cond.assoc_cond = assoc_cond;

		local_user_list = acct_storage_g_get_users(
			db_conn, my_uid, &user_cond);

	}

	if(!local_user_list) {
		exit_code=1;
		fprintf(stderr, " Problem getting users from database.  "
			"Contact your admin.\n");
		slurmdb_destroy_wckey_cond(wckey_cond);
		slurmdb_destroy_association_cond(assoc_cond);
		return SLURM_ERROR;
	}


	if(!list_count(assoc_cond->cluster_list)) {
		List cluster_list = NULL;
		slurmdb_cluster_rec_t *cluster_rec = NULL;

		cluster_list = acct_storage_g_get_clusters(db_conn,
							   my_uid, NULL);
		if(!cluster_list) {
			exit_code=1;
			fprintf(stderr,
				" Problem getting clusters from database.  "
				"Contact your admin.\n");
			slurmdb_destroy_wckey_cond(wckey_cond);
			slurmdb_destroy_association_cond(assoc_cond);
			list_destroy(local_user_list);
			if(local_acct_list)
				list_destroy(local_acct_list);
			return SLURM_ERROR;
		}

		itr_c = list_iterator_create(cluster_list);
		while((cluster_rec = list_next(itr_c))) {
			list_append(assoc_cond->cluster_list,
				    xstrdup(cluster_rec->name));
		}
		list_iterator_destroy(itr_c);

		if(!list_count(assoc_cond->cluster_list)) {
			exit_code=1;
			fprintf(stderr,
				"  Can't add users, no cluster defined yet.\n"
				" Please contact your administrator.\n");
			slurmdb_destroy_wckey_cond(wckey_cond);
			slurmdb_destroy_association_cond(assoc_cond);
			list_destroy(local_user_list);
			if(local_acct_list)
				list_destroy(local_acct_list);
			return SLURM_ERROR;
		}
	} else {
		List temp_list = NULL;
		slurmdb_cluster_cond_t cluster_cond;

		slurmdb_init_cluster_cond(&cluster_cond);
		cluster_cond.cluster_list = assoc_cond->cluster_list;

		temp_list = acct_storage_g_get_clusters(db_conn, my_uid,
							&cluster_cond);

		itr_c = list_iterator_create(assoc_cond->cluster_list);
		itr = list_iterator_create(temp_list);
		while((cluster = list_next(itr_c))) {
			slurmdb_cluster_rec_t *cluster_rec = NULL;

			list_iterator_reset(itr);
			while((cluster_rec = list_next(itr))) {
				if(!strcasecmp(cluster_rec->name, cluster))
					break;
			}
			if(!cluster_rec) {
				exit_code=1;
				fprintf(stderr, " This cluster '%s' "
					"doesn't exist.\n"
					"        Contact your admin "
					"to add it to accounting.\n",
					cluster);
				list_delete_item(itr_c);
			}
		}
		list_iterator_destroy(itr);
		list_iterator_destroy(itr_c);
		list_destroy(temp_list);

		if(!list_count(assoc_cond->cluster_list)) {
			slurmdb_destroy_wckey_cond(wckey_cond);
			slurmdb_destroy_association_cond(assoc_cond);
			list_destroy(local_user_list);
			if(local_acct_list)
				list_destroy(local_acct_list);
			return SLURM_ERROR;
		}
	}

	if(!list_count(assoc_cond->acct_list)) {
		if(!list_count(wckey_cond->name_list)) {
			slurmdb_destroy_wckey_cond(wckey_cond);
			slurmdb_destroy_association_cond(assoc_cond);
			exit_code=1;
			fprintf(stderr, " Need name of account to "
				"add user to.\n");
			return SLURM_ERROR;
		}
	} else {
 		slurmdb_account_cond_t account_cond;
		slurmdb_association_cond_t query_assoc_cond;

		memset(&account_cond, 0, sizeof(slurmdb_account_cond_t));
		account_cond.assoc_cond = assoc_cond;

		local_acct_list = acct_storage_g_get_accounts(
			db_conn, my_uid, &account_cond);

		if(!local_acct_list) {
			exit_code=1;
			fprintf(stderr, " Problem getting accounts "
				"from database.  Contact your admin.\n");
			list_destroy(local_user_list);
			slurmdb_destroy_wckey_cond(wckey_cond);
			slurmdb_destroy_association_cond(assoc_cond);
			return SLURM_ERROR;
		}

		if(!default_acct)
			default_acct =
				xstrdup(list_peek(assoc_cond->acct_list));

		memset(&query_assoc_cond, 0,
		       sizeof(slurmdb_association_cond_t));
		query_assoc_cond.acct_list = assoc_cond->acct_list;
		query_assoc_cond.cluster_list = assoc_cond->cluster_list;
		local_assoc_list = acct_storage_g_get_associations(
			db_conn, my_uid, &query_assoc_cond);

		if(!local_assoc_list) {
			exit_code=1;
			fprintf(stderr, " Problem getting associations "
				"from database.  Contact your admin.\n");
			list_destroy(local_user_list);
			list_destroy(local_acct_list);
			slurmdb_destroy_wckey_cond(wckey_cond);
			slurmdb_destroy_association_cond(assoc_cond);
			return SLURM_ERROR;
		}
	}

	if(track_wckey || default_wckey) {
		if(!default_wckey)
			default_wckey =
				xstrdup(list_peek(wckey_cond->name_list));
		wckey_cond->cluster_list = assoc_cond->cluster_list;
		wckey_cond->user_list = assoc_cond->user_list;
		if(!(local_wckey_list = acct_storage_g_get_wckeys(
			     db_conn, my_uid, wckey_cond)))
			info("If you are a coordinator ignore "
			     "the previous error");

		wckey_cond->cluster_list = NULL;
		wckey_cond->user_list = NULL;

	}

	/* we are adding these lists to the global lists and will be
	   freed when they are */
	user_list = list_create(slurmdb_destroy_user_rec);
	assoc_list = list_create(slurmdb_destroy_association_rec);
	wckey_list = list_create(slurmdb_destroy_wckey_rec);

	itr = list_iterator_create(assoc_cond->user_list);
	while((name = list_next(itr))) {
		if(!name[0]) {
			exit_code=1;
			fprintf(stderr, " No blank names are "
				"allowed when adding.\n");
			rc = SLURM_ERROR;
			continue;
		}

		user = NULL;
		if(!sacctmgr_find_user_from_list(local_user_list, name)) {
			uid_t pw_uid;
			if(!default_acct || !default_acct[0]) {
				exit_code=1;
				fprintf(stderr, " Need a default account for "
					"these users to add.\n");
				rc = SLURM_ERROR;
				goto no_default;
			}
			if(first) {
				if(!sacctmgr_find_account_from_list(
					   local_acct_list, default_acct)) {
					exit_code=1;
					fprintf(stderr, " This account '%s' "
						"doesn't exist.\n"
						"        Contact your admin "
						"to add this account.\n",
						default_acct);
					continue;
				}
				first = 0;
			}
			if (uid_from_string (name, &pw_uid) < 0) {
				char *warning = xstrdup_printf(
					"There is no uid for user '%s'"
					"\nAre you sure you want to continue?",
					name);

				if(!commit_check(warning)) {
					xfree(warning);
					rc = SLURM_ERROR;
					list_flush(user_list);
					goto end_it;
				}
				xfree(warning);
			}

			user = xmalloc(sizeof(slurmdb_user_rec_t));
			user->assoc_list =
				list_create(slurmdb_destroy_association_rec);
			user->wckey_list =
				list_create(slurmdb_destroy_wckey_rec);
			user->name = xstrdup(name);
			user->default_acct = xstrdup(default_acct);
			user->default_wckey = xstrdup(default_wckey);

			user->admin_level = admin_level;

			xstrfmtcat(user_str, "  %s\n", name);

			list_append(user_list, user);
		}

		itr_a = list_iterator_create(assoc_cond->acct_list);
		while((account = list_next(itr_a))) {
			if(acct_first) {
				if(!sacctmgr_find_account_from_list(
					   local_acct_list, default_acct)) {
					exit_code=1;
					fprintf(stderr, " This account '%s' "
						"doesn't exist.\n"
						"        Contact your admin "
						"to add this account.\n",
						account);
					continue;
				}
			}
			itr_c = list_iterator_create(assoc_cond->cluster_list);
			while((cluster = list_next(itr_c))) {
				if(!sacctmgr_find_account_base_assoc_from_list(
					   local_assoc_list, account,
					   cluster)) {
					if(acct_first) {
						exit_code=1;
						fprintf(stderr, " This "
							"account '%s' "
							"doesn't exist on "
							"cluster %s\n"
							"        Contact your "
							"admin to add "
							"this account.\n",
							account, cluster);
					}
					continue;
				}

				itr_p = list_iterator_create(
					assoc_cond->partition_list);
				while((partition = list_next(itr_p))) {
					partition_set = 1;
					if(sacctmgr_find_association_from_list(
						   local_assoc_list,
						   name, account,
						   cluster, partition))
						continue;
					assoc = xmalloc(
						sizeof(slurmdb_association_rec_t));
					slurmdb_init_association_rec(assoc);
					assoc->user = xstrdup(name);
					assoc->acct = xstrdup(account);
					assoc->cluster = xstrdup(cluster);
					assoc->partition = xstrdup(partition);

					assoc->shares_raw =
						start_assoc.shares_raw;

					assoc->grp_cpu_mins =
						start_assoc.grp_cpu_mins;
					assoc->grp_cpus = start_assoc.grp_cpus;
					assoc->grp_jobs = start_assoc.grp_jobs;
					assoc->grp_nodes =
						start_assoc.grp_nodes;
					assoc->grp_submit_jobs =
						start_assoc.grp_submit_jobs;
					assoc->grp_wall = start_assoc.grp_wall;

					assoc->max_cpu_mins_pj =
						start_assoc.max_cpu_mins_pj;
					assoc->max_cpus_pj =
						start_assoc.max_cpus_pj;
					assoc->max_jobs = start_assoc.max_jobs;
					assoc->max_nodes_pj =
						start_assoc.max_nodes_pj;
					assoc->max_submit_jobs =
						start_assoc.max_submit_jobs;
					assoc->max_wall_pj =
						start_assoc.max_wall_pj;

					assoc->qos_list = copy_char_list(
						start_assoc.qos_list);

					if(user)
						list_append(user->assoc_list,
							    assoc);
					else
						list_append(assoc_list, assoc);
					xstrfmtcat(assoc_str,
						   "  U = %-9.9s"
						   " A = %-10.10s"
						   " C = %-10.10s"
						   " P = %-10.10s\n",
						   assoc->user, assoc->acct,
						   assoc->cluster,
						   assoc->partition);
				}
				list_iterator_destroy(itr_p);
				if(partition_set)
					continue;

				if(sacctmgr_find_association_from_list(
					   local_assoc_list,
					   name, account, cluster, NULL)) {
					continue;
				}

				assoc = xmalloc(
					sizeof(slurmdb_association_rec_t));
				slurmdb_init_association_rec(assoc);
				assoc->user = xstrdup(name);
				assoc->acct = xstrdup(account);
				assoc->cluster = xstrdup(cluster);

				assoc->shares_raw = start_assoc.shares_raw;

				assoc->grp_cpu_mins =
					start_assoc.grp_cpu_mins;
				assoc->grp_cpus = start_assoc.grp_cpus;
				assoc->grp_jobs = start_assoc.grp_jobs;
				assoc->grp_nodes = start_assoc.grp_nodes;
				assoc->grp_submit_jobs =
					start_assoc.grp_submit_jobs;
				assoc->grp_wall = start_assoc.grp_wall;

				assoc->max_cpu_mins_pj =
					start_assoc.max_cpu_mins_pj;
				assoc->max_cpus_pj = start_assoc.max_cpus_pj;
				assoc->max_jobs = start_assoc.max_jobs;
				assoc->max_nodes_pj = start_assoc.max_nodes_pj;
				assoc->max_submit_jobs =
					start_assoc.max_submit_jobs;
				assoc->max_wall_pj = start_assoc.max_wall_pj;

				assoc->qos_list =
					copy_char_list(start_assoc.qos_list);

				if(user)
					list_append(user->assoc_list, assoc);
				else
					list_append(assoc_list, assoc);
				xstrfmtcat(assoc_str,
					   "  U = %-9.9s"
					   " A = %-10.10s"
					   " C = %-10.10s\n",
					   assoc->user, assoc->acct,
					   assoc->cluster);
			}
			list_iterator_destroy(itr_c);
		}
		list_iterator_destroy(itr_a);
		acct_first = 0;

		/* continue here if not doing wckeys */
		if(!track_wckey && !default_wckey)
			continue;

		itr_w = list_iterator_create(wckey_cond->name_list);
		while((account = list_next(itr_w))) {
			itr_c = list_iterator_create(assoc_cond->cluster_list);
			while((cluster = list_next(itr_c))) {
				if(sacctmgr_find_wckey_from_list(
					   local_wckey_list, name, account,
					   cluster)) {
					continue;
				}
				wckey = xmalloc(sizeof(slurmdb_wckey_rec_t));
				wckey->user = xstrdup(name);
				wckey->name = xstrdup(account);
				wckey->cluster = xstrdup(cluster);
				if(user)
					list_append(user->wckey_list, wckey);
				else
					list_append(wckey_list, wckey);
				xstrfmtcat(wckey_str,
					   "  U = %-9.9s"
					   " W = %-10.10s"
					   " C = %-10.10s\n",
					   wckey->user, wckey->name,
					   wckey->cluster);

			}
			list_iterator_destroy(itr_c);
		}
		list_iterator_destroy(itr_w);
	}
no_default:
	list_iterator_destroy(itr);
	list_destroy(local_user_list);
	if(local_acct_list)
		list_destroy(local_acct_list);
	if(local_assoc_list)
		list_destroy(local_assoc_list);
	if(local_wckey_list)
		list_destroy(local_wckey_list);
	slurmdb_destroy_wckey_cond(wckey_cond);
	slurmdb_destroy_association_cond(assoc_cond);

	if(!list_count(user_list) && !list_count(assoc_list)
	   && !list_count(wckey_list)) {
		printf(" Nothing new added.\n");
		goto end_it;
	} else if(!assoc_str && !wckey_str) {
		exit_code=1;
		fprintf(stderr, " No associations or wckeys created.\n");
		goto end_it;
	}

	if(user_str) {
		printf(" Adding User(s)\n%s", user_str);
		printf(" Settings =\n");
		printf("  Default Account = %s\n", default_acct);
		if(default_wckey)
			printf("  Default WCKey   = %s\n", default_wckey);

		if(admin_level != SLURMDB_ADMIN_NOTSET)
			printf("  Admin Level     = %s\n",
			       slurmdb_admin_level_str(admin_level));
		xfree(user_str);
	}

	if(assoc_str) {
		printf(" Associations =\n%s", assoc_str);
		xfree(assoc_str);
	}

	if(wckey_str) {
		printf(" WCKeys =\n%s", wckey_str);
		xfree(wckey_str);
	}

	if(limit_set) {
		printf(" Non Default Settings\n");
		sacctmgr_print_assoc_limits(&start_assoc);
		if(start_assoc.qos_list)
			list_destroy(start_assoc.qos_list);
	}

	notice_thread_init();
	if(list_count(user_list)) {
		rc = acct_storage_g_add_users(db_conn, my_uid, user_list);
	}

	if(rc == SLURM_SUCCESS) {
		if(list_count(assoc_list))
			rc = acct_storage_g_add_associations(db_conn, my_uid,
							     assoc_list);
	}

	if(rc == SLURM_SUCCESS) {
		if(list_count(wckey_list))
			rc = acct_storage_g_add_wckeys(db_conn, my_uid,
						       wckey_list);
	} else {
		exit_code=1;
		fprintf(stderr, " Problem adding users: %s\n",
			slurm_strerror(rc));
		rc = SLURM_ERROR;
		notice_thread_fini();
		goto end_it;
	}

	notice_thread_fini();

	if(rc == SLURM_SUCCESS) {
		if(commit_check("Would you like to commit changes?")) {
			acct_storage_g_commit(db_conn, 1);
		} else {
			printf(" Changes Discarded\n");
			acct_storage_g_commit(db_conn, 0);
		}
	} else {
		exit_code=1;
		fprintf(stderr, " Problem adding user associations: %s\n",
			slurm_strerror(rc));
		rc = SLURM_ERROR;
	}

end_it:
	list_destroy(user_list);
	list_destroy(assoc_list);
	list_destroy(wckey_list);
	xfree(default_acct);
	xfree(default_wckey);

	return rc;
}

extern int sacctmgr_add_coord(int argc, char *argv[])
{
	int rc = SLURM_SUCCESS;
	int i=0;
	int cond_set = 0, prev_set = 0;
	slurmdb_user_cond_t *user_cond = xmalloc(sizeof(slurmdb_user_cond_t));
	char *name = NULL;
	char *user_str = NULL;
	char *acct_str = NULL;
	ListIterator itr = NULL;

	for (i=0; i<argc; i++) {
		int command_len = strlen(argv[i]);
		if (!strncasecmp(argv[i], "Where", MAX(command_len, 5))
		    || !strncasecmp(argv[i], "Set", MAX(command_len, 3)))
			i++;
		prev_set = _set_cond(&i, argc, argv, user_cond, NULL);
		cond_set |= prev_set;
	}

	if(exit_code) {
		slurmdb_destroy_user_cond(user_cond);
		return SLURM_ERROR;
	} else if(!cond_set) {
		exit_code=1;
		fprintf(stderr, " You need to specify conditions to "
			"to add the coordinator.\n");
		slurmdb_destroy_user_cond(user_cond);
		return SLURM_ERROR;
	}

	if((_check_coord_request(user_cond, true) == SLURM_ERROR)
	   || exit_code) {
		slurmdb_destroy_user_cond(user_cond);
		return SLURM_ERROR;
	}

	itr = list_iterator_create(user_cond->assoc_cond->user_list);
	while((name = list_next(itr))) {
		xstrfmtcat(user_str, "  %s\n", name);
	}
	list_iterator_destroy(itr);

	itr = list_iterator_create(user_cond->assoc_cond->acct_list);
	while((name = list_next(itr))) {
		xstrfmtcat(acct_str, "  %s\n", name);
	}
	list_iterator_destroy(itr);

	printf(" Adding Coordinator User(s)\n%s", user_str);
	printf(" To Account(s) and all sub-accounts\n%s", acct_str);

	notice_thread_init();
	rc = acct_storage_g_add_coord(db_conn, my_uid,
				      user_cond->assoc_cond->acct_list,
				      user_cond);
	notice_thread_fini();
	slurmdb_destroy_user_cond(user_cond);

	if(rc == SLURM_SUCCESS) {
		if(commit_check("Would you like to commit changes?")) {
			acct_storage_g_commit(db_conn, 1);
		} else {
			printf(" Changes Discarded\n");
			acct_storage_g_commit(db_conn, 0);
		}
	} else {
		exit_code=1;
		fprintf(stderr, " Problem adding coordinator: %s\n",
			slurm_strerror(rc));
		rc = SLURM_ERROR;
	}

	return rc;
}

extern int sacctmgr_list_user(int argc, char *argv[])
{
	int rc = SLURM_SUCCESS;
	slurmdb_user_cond_t *user_cond = xmalloc(sizeof(slurmdb_user_cond_t));
	List user_list;
	int i=0, cond_set=0, prev_set=0;
	ListIterator itr = NULL;
	ListIterator itr2 = NULL;
	slurmdb_user_rec_t *user = NULL;
	slurmdb_association_rec_t *assoc = NULL;

	print_field_t *field = NULL;
	int field_count = 0;

	List format_list = list_create(slurm_destroy_char);
	List print_fields_list; /* types are of print_field_t */

	user_cond->with_assocs = with_assoc_flag;

	for (i=0; i<argc; i++) {
		int command_len = strlen(argv[i]);
		if (!strncasecmp(argv[i], "Where", MAX(command_len, 5))
		    || !strncasecmp(argv[i], "Set", MAX(command_len, 3)))
			i++;
		prev_set = _set_cond(&i, argc, argv, user_cond, format_list);
		cond_set |= prev_set;
	}

	if(exit_code) {
		slurmdb_destroy_user_cond(user_cond);
		list_destroy(format_list);
		return SLURM_ERROR;
	}

	if(!list_count(format_list)) {
		if(slurm_get_track_wckey())
			slurm_addto_char_list(format_list,
					      "U,DefaultA,DefaultW,Ad");
		else
			slurm_addto_char_list(format_list, "U,DefaultA,Ad");
		if(user_cond->with_assocs)
			slurm_addto_char_list(format_list,
					      "Cl,Acc,Part,Share,"
					      "MaxJ,MaxN,MaxCPUs,MaxS,MaxW,"
					      "MaxCPUMins,QOS,DefaultQOS");
		if(user_cond->with_coords)
			slurm_addto_char_list(format_list, "Coord");
	}

	if(!user_cond->with_assocs && cond_set > 1) {
		if(!commit_check("You requested options that are only vaild "
				 "when querying with the withassoc option.\n"
				 "Are you sure you want to continue?")) {
			printf("Aborted\n");
			list_destroy(format_list);
			slurmdb_destroy_user_cond(user_cond);
			return SLURM_SUCCESS;
		}
	}

	print_fields_list = sacctmgr_process_format_list(format_list);
	list_destroy(format_list);

	if(exit_code) {
		slurmdb_destroy_user_cond(user_cond);
		list_destroy(print_fields_list);
		return SLURM_ERROR;
	}

	user_list = acct_storage_g_get_users(db_conn, my_uid, user_cond);
	slurmdb_destroy_user_cond(user_cond);

	if(!user_list) {
		exit_code=1;
		fprintf(stderr, " Problem with query.\n");
		list_destroy(print_fields_list);
		return SLURM_ERROR;
	}

	itr = list_iterator_create(user_list);
	itr2 = list_iterator_create(print_fields_list);
	print_fields_header(print_fields_list);

	field_count = list_count(print_fields_list);

	while((user = list_next(itr))) {
		if(user->assoc_list) {
			ListIterator itr3 =
				list_iterator_create(user->assoc_list);

			while((assoc = list_next(itr3))) {
				int curr_inx = 1;
				while((field = list_next(itr2))) {
					switch(field->type) {
					case PRINT_ADMIN:
						field->print_routine(
							field,
							slurmdb_admin_level_str(
								user->
								admin_level),
							(curr_inx ==
							 field_count));
						break;
					case PRINT_COORDS:
						field->print_routine(
							field,
							user->coord_accts,
							(curr_inx ==
							 field_count));
						break;
					case PRINT_DACCT:
						field->print_routine(
							field,
							user->default_acct,
							(curr_inx ==
							 field_count),
							(curr_inx ==
							 field_count));
						break;
					case PRINT_DWCKEY:
						field->print_routine(
							field,
							user->default_wckey,
							(curr_inx ==
							 field_count),
							(curr_inx ==
							 field_count));
						break;
					default:
						sacctmgr_print_association_rec(
							assoc, field, NULL,
							(curr_inx ==
							 field_count));
						break;
					}
					curr_inx++;
				}
				list_iterator_reset(itr2);
				printf("\n");
			}
			list_iterator_destroy(itr3);
		} else {
			int curr_inx = 1;
			while((field = list_next(itr2))) {
				switch(field->type) {
				case PRINT_QOS:
					field->print_routine(
						field, NULL,
						NULL,
						(curr_inx == field_count));
					break;
				case PRINT_ADMIN:
					field->print_routine(
						field,
						slurmdb_admin_level_str(
							user->admin_level),
						(curr_inx == field_count));
					break;
				case PRINT_COORDS:
					field->print_routine(
						field,
						user->coord_accts,
						(curr_inx == field_count));
					break;
				case PRINT_DACCT:
					field->print_routine(
						field,
						user->default_acct,
						(curr_inx == field_count));
					break;
				case PRINT_DWCKEY:
					field->print_routine(
						field,
						user->default_wckey,
						(curr_inx == field_count));
					break;
				case PRINT_USER:
					field->print_routine(
						field,
						user->name,
						(curr_inx == field_count));
					break;
				default:
					field->print_routine(
						field, NULL,
						(curr_inx == field_count));
					break;
				}
				curr_inx++;
			}
			list_iterator_reset(itr2);
			printf("\n");
		}
	}

	list_iterator_destroy(itr2);
	list_iterator_destroy(itr);
	list_destroy(user_list);
	list_destroy(print_fields_list);

	return rc;
}

extern int sacctmgr_modify_user(int argc, char *argv[])
{
	int rc = SLURM_SUCCESS;
	slurmdb_user_cond_t *user_cond = xmalloc(sizeof(slurmdb_user_cond_t));
	slurmdb_user_rec_t *user = xmalloc(sizeof(slurmdb_user_rec_t));
	slurmdb_association_rec_t *assoc =
		xmalloc(sizeof(slurmdb_association_rec_t));
	int i=0;
	int cond_set = 0, prev_set = 0, rec_set = 0, set = 0;
	List ret_list = NULL;

	slurmdb_init_association_rec(assoc);

	for (i=0; i<argc; i++) {
		int command_len = strlen(argv[i]);
		if (!strncasecmp(argv[i], "Where", MAX(command_len, 5))) {
			i++;
			prev_set = _set_cond(&i, argc, argv, user_cond, NULL);
			cond_set |= prev_set;
		} else if (!strncasecmp(argv[i], "Set", MAX(command_len, 3))) {
			i++;
			prev_set = _set_rec(&i, argc, argv, user, assoc);
			rec_set |= prev_set;
		} else {
			prev_set = _set_cond(&i, argc, argv, user_cond, NULL);
			cond_set |= prev_set;
		}
	}

	if(exit_code) {
		slurmdb_destroy_user_cond(user_cond);
		slurmdb_destroy_user_rec(user);
		slurmdb_destroy_association_rec(assoc);
		return SLURM_ERROR;
	} else if(!rec_set) {
		exit_code=1;
		fprintf(stderr, " You didn't give me anything to set\n");
		slurmdb_destroy_user_cond(user_cond);
		slurmdb_destroy_user_rec(user);
		slurmdb_destroy_association_rec(assoc);
		return SLURM_ERROR;
	} else if(!cond_set) {
		if(!commit_check("You didn't set any conditions with 'WHERE'.\n"
				 "Are you sure you want to continue?")) {
			printf("Aborted\n");
			slurmdb_destroy_user_cond(user_cond);
			slurmdb_destroy_user_rec(user);
			slurmdb_destroy_association_rec(assoc);
			return SLURM_SUCCESS;
		}
	}

	// Special case:  reset raw usage only
	if (assoc->usage) {
		rc = SLURM_ERROR;
		if (user_cond->assoc_cond->acct_list) {
			if (assoc->usage->usage_raw == 0.0)
				rc = sacctmgr_remove_assoc_usage(
					user_cond->assoc_cond);
			else
				error("Raw usage can only be set to 0 (zero)");
		} else {
			error("An account must be specified");
		}

		slurmdb_destroy_user_cond(user_cond);
		slurmdb_destroy_user_rec(user);
		slurmdb_destroy_association_rec(assoc);
		return rc;
	}

	notice_thread_init();
	if(rec_set & 1) { // process the account changes
		if(cond_set == 2) {
			rc = SLURM_ERROR;
			exit_code=1;
			fprintf(stderr,
				" There was a problem with your "
				"'where' options.\n");
			goto assoc_start;
		}

		if(user_cond->assoc_cond
		   && user_cond->assoc_cond->acct_list
		   && list_count(user_cond->assoc_cond->acct_list)) {
			notice_thread_fini();
			if(commit_check(
				   " You specified Accounts in your "
				   "request.  Did you mean "
				   "DefaultAccounts?\n")) {
				if(!user_cond->def_acct_list)
					user_cond->def_acct_list =
						list_create(slurm_destroy_char);
				list_transfer(user_cond->def_acct_list,
					      user_cond->assoc_cond->acct_list);
			}
			notice_thread_init();
		}

		ret_list = acct_storage_g_modify_users(
			db_conn, my_uid, user_cond, user);
		if(ret_list && list_count(ret_list)) {
			char *object = NULL;
			List regret_list = NULL;
			ListIterator itr = list_iterator_create(ret_list);

			while((object = list_next(itr))) {
				/* We have to check here for the user names to
				 * make sure the user has an association with
				 * the new default account.  We have to wait
				 * until we get the ret_list of names since
				 * names are required to change a user since
				 * you can specfy a user by something else
				 * like default_account or something.  If the
				 * user doesn't have the account make
				 * note of it.
				 */
				if(user->default_acct &&
				   !_check_user_has_acct(
					   object, user->default_acct)) {
					if(!regret_list)
						regret_list = list_create(NULL);
					list_append(regret_list, object);
					continue;
				}
			}
			if(regret_list) {
				list_iterator_destroy(itr);
				itr = list_iterator_create(regret_list);
				printf(" Can't modify because these users "
				       "aren't associated with new "
				       "default account '%s'...\n",
				       user->default_acct);
				while((object = list_next(itr))) {
					printf("  %s\n", object);
				}
				list_iterator_destroy(itr);
				exit_code=1;
				rc = SLURM_ERROR;
				list_destroy(regret_list);
			} else {
				list_iterator_reset(itr);
				printf(" Modified users...\n");
				while((object = list_next(itr))) {
					printf("  %s\n", object);
				}
				list_iterator_destroy(itr);
				set = 1;
			}
		} else if(ret_list) {
			printf(" Nothing modified\n");
		} else {
			exit_code=1;
			fprintf(stderr, " Error with request: %s\n",
				slurm_strerror(errno));
			if(errno == ESLURM_ONE_CHANGE)
				fprintf(stderr, " If you are changing a users "
					"name you can only specify 1 user "
					"at a time.\n");
			rc = SLURM_ERROR;
		}

		if(ret_list)
			list_destroy(ret_list);
	}

assoc_start:
	if(rec_set & 2) { // process the association changes
		if(cond_set == 1
		   && !list_count(user_cond->assoc_cond->user_list)) {
			rc = SLURM_ERROR;
			exit_code=1;
			fprintf(stderr,
				" There was a problem with your "
				"'where' options.\n");
			goto assoc_end;
		}

		ret_list = acct_storage_g_modify_associations(
			db_conn, my_uid, user_cond->assoc_cond, assoc);

		if(ret_list && list_count(ret_list)) {
			char *object = NULL;
			ListIterator itr = list_iterator_create(ret_list);
			printf(" Modified account associations...\n");
			while((object = list_next(itr))) {
				printf("  %s\n", object);
			}
			list_iterator_destroy(itr);
			set = 1;
		} else if(ret_list) {
			printf(" Nothing modified\n");
		} else {
			exit_code=1;
			fprintf(stderr, " Error with request: %s\n",
				slurm_strerror(errno));
			rc = SLURM_ERROR;
		}

		if(ret_list)
			list_destroy(ret_list);
	}
assoc_end:

	notice_thread_fini();
	if(set) {
		if(commit_check("Would you like to commit changes?"))
			acct_storage_g_commit(db_conn, 1);
		else {
			printf(" Changes Discarded\n");
			acct_storage_g_commit(db_conn, 0);
		}
	}

	slurmdb_destroy_user_cond(user_cond);
	slurmdb_destroy_user_rec(user);
	slurmdb_destroy_association_rec(assoc);

	return rc;
}

extern int sacctmgr_delete_user(int argc, char *argv[])
{
	int rc = SLURM_SUCCESS;
	slurmdb_user_cond_t *user_cond = xmalloc(sizeof(slurmdb_user_cond_t));
	int i=0;
	List ret_list = NULL;
	int cond_set = 0, prev_set = 0;

	for (i=0; i<argc; i++) {
		int command_len = strlen(argv[i]);
		if (!strncasecmp(argv[i], "Where", MAX(command_len, 5))
		    || !strncasecmp(argv[i], "Set", MAX(command_len, 3)))
			i++;
		prev_set = _set_cond(&i, argc, argv, user_cond, NULL);
		cond_set |= prev_set;
	}

	if(!cond_set) {
		exit_code=1;
		fprintf(stderr,
			" No conditions given to remove, not executing.\n");
		slurmdb_destroy_user_cond(user_cond);
		return SLURM_ERROR;
	}

	if(exit_code) {
		slurmdb_destroy_user_cond(user_cond);
		return SLURM_ERROR;
	}

	notice_thread_init();
	if(cond_set == 1) {
		ret_list = acct_storage_g_remove_users(
			db_conn, my_uid, user_cond);
	} else if(cond_set & 2) {
		ret_list = acct_storage_g_remove_associations(
			db_conn, my_uid, user_cond->assoc_cond);
	}
	rc = errno;
	notice_thread_fini();

	slurmdb_destroy_user_cond(user_cond);

	if(ret_list && list_count(ret_list)) {
		char *object = NULL;
		ListIterator itr = list_iterator_create(ret_list);
		/* If there were jobs running with an association to
		   be deleted, don't.
		*/
		if(rc == ESLURM_JOBS_RUNNING_ON_ASSOC) {
			fprintf(stderr, " Error with request: %s\n",
				slurm_strerror(rc));
			while((object = list_next(itr))) {
				fprintf(stderr,"  %s\n", object);
			}
			list_iterator_destroy(itr);
			list_destroy(ret_list);
			acct_storage_g_commit(db_conn, 0);
			return rc;
		}
		if(cond_set == 1) {
			printf(" Deleting users...\n");
		} else if(cond_set & 2) {
			printf(" Deleting user associations...\n");
		}
		while((object = list_next(itr))) {
			printf("  %s\n", object);
		}
		list_iterator_destroy(itr);
		if(commit_check("Would you like to commit changes?")) {
			acct_storage_g_commit(db_conn, 1);
		} else {
			printf(" Changes Discarded\n");
			acct_storage_g_commit(db_conn, 0);
		}
	} else if(ret_list) {
		printf(" Nothing deleted\n");
	} else {
		exit_code=1;
		fprintf(stderr, " Error with request: %s\n",
			slurm_strerror(errno));
		rc = SLURM_ERROR;
	}


	if(ret_list)
		list_destroy(ret_list);

	return rc;
}

extern int sacctmgr_delete_coord(int argc, char *argv[])
{
	int rc = SLURM_SUCCESS;
	int i=0, set=0;
	int cond_set = 0, prev_set = 0;
	slurmdb_user_cond_t *user_cond = xmalloc(sizeof(slurmdb_user_cond_t));
	char *name = NULL;
	char *user_str = NULL;
	char *acct_str = NULL;
	ListIterator itr = NULL;
	List ret_list = NULL;


	for (i=0; i<argc; i++) {
		int command_len = strlen(argv[i]);
		if (!strncasecmp(argv[i], "Where", MAX(command_len, 5))
		    || !strncasecmp(argv[i], "Set", MAX(command_len, 3)))
			i++;
		prev_set = _set_cond(&i, argc, argv, user_cond, NULL);
		cond_set |= prev_set;
	}

	if(exit_code) {
		slurmdb_destroy_user_cond(user_cond);
		return SLURM_ERROR;
	} else if(!cond_set) {
		exit_code=1;
		fprintf(stderr, " You need to specify a user list "
			"or account list here.\n");
		slurmdb_destroy_user_cond(user_cond);
		return SLURM_ERROR;
	}
	if((_check_coord_request(user_cond, false) == SLURM_ERROR)
	   || exit_code) {
		slurmdb_destroy_user_cond(user_cond);
		return SLURM_ERROR;
	}

	if(user_cond->assoc_cond->user_list) {
		itr = list_iterator_create(user_cond->assoc_cond->user_list);
		while((name = list_next(itr))) {
			xstrfmtcat(user_str, "  %s\n", name);

		}
		list_iterator_destroy(itr);
	}

	if(user_cond->assoc_cond->acct_list) {
		itr = list_iterator_create(user_cond->assoc_cond->acct_list);
		while((name = list_next(itr))) {
			xstrfmtcat(acct_str, "  %s\n", name);

		}
		list_iterator_destroy(itr);
	}

	if(!user_str && !acct_str) {
		exit_code=1;
		fprintf(stderr, " You need to specify a user list "
			"or an account list here.\n");
		slurmdb_destroy_user_cond(user_cond);
		return SLURM_ERROR;
	}
	/* FIX ME: This list should be received from the slurmdbd not
	 * just assumed.  Right now it doesn't do it correctly though.
	 * This is why we are doing it this way.
	 */
	if(user_str) {
		printf(" Removing Coordinators with user name\n%s", user_str);
		if(acct_str)
			printf(" From Account(s)\n%s", acct_str);
		else
			printf(" From all accounts\n");
	} else
		printf(" Removing all users from Accounts\n%s", acct_str);

	notice_thread_init();
        ret_list = acct_storage_g_remove_coord(db_conn, my_uid,
					       user_cond->assoc_cond->acct_list,
					       user_cond);
	slurmdb_destroy_user_cond(user_cond);


	if(ret_list && list_count(ret_list)) {
		char *object = NULL;
		ListIterator itr = list_iterator_create(ret_list);
		printf(" Removed Coordinators (sub accounts not listed)...\n");
		while((object = list_next(itr))) {
			printf("  %s\n", object);
		}
		list_iterator_destroy(itr);
		set = 1;
	} else if(ret_list) {
		printf(" Nothing removed\n");
	} else {
		exit_code=1;
		fprintf(stderr, " Error with request: %s\n",
			slurm_strerror(errno));
		rc = SLURM_ERROR;
	}

	if(ret_list)
		list_destroy(ret_list);
	notice_thread_fini();
	if(set) {
		if(commit_check("Would you like to commit changes?"))
			acct_storage_g_commit(db_conn, 1);
		else {
			printf(" Changes Discarded\n");
			acct_storage_g_commit(db_conn, 0);
		}
	}

	return rc;
}
