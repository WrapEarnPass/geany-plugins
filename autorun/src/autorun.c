/*
 * Copyright 2026 WrapEarnPass
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include <geanyplugin.h> 
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "autorun.h"

AUTORUN_GLOBALS* autorun_globals;

void autorun_globals_init(GeanyPlugin* plugin)
{
	autorun_globals = g_new(AUTORUN_GLOBALS,1);
	autorun_globals->plugin=plugin;
	autorun_globals->data=plugin->geany_data;
}
void autorun_globals_cleanup(void)
{
	autorun_globals->plugin=NULL;
	autorun_globals->data=NULL;
	g_free(autorun_globals);
	autorun_globals=NULL;
}
void load_filedefs(void)
{
	// if there are any filetypes.FILE autorun sections
	gchar * filedef_path = g_build_path(G_DIR_SEPARATOR_S, autorun_globals->data->app->configdir, "filedefs", NULL); 
	guint filedef_len;
	GSList * file_list = utils_get_file_list(filedef_path, &filedef_len, NULL);
	if( filedef_len > 0){
		// stash filetypes.FILE to fallback on-project-close
		GSList* node;
		foreach_slist(node,file_list){
			if( g_str_has_prefix(node->data, "filetypes.") && g_strcmp0(node->data, "filetypes.README")!=0 )
			{
				//we have files to process
				gsize key_len;
				gchar * filedef_file = g_build_filename(G_DIR_SEPARATOR_S, filedef_path, (gchar*)node->data, NULL);
				GKeyFile* config=g_key_file_new ();
				g_key_file_load_from_file(config,filedef_file ,G_KEY_FILE_NONE, NULL);
				GError * gerr=NULL;
				gchar** handlers = g_key_file_get_keys(config, "autorun", &key_len, &gerr);
				//if the keyfile has an [autorun] section
				if( gerr == NULL ){
					g_message("found %s autorun",(gchar*)node->data);
					gchar** handler_key;
					foreach_strv( handler_key, handlers){
						g_message(" %s", *handler_key);
					}
					
				}else
				{
					g_free(gerr);
				}
				g_free(filedef_file);
				g_free(handlers);
			}
		}
		
	}
			//cleanup
	g_slist_foreach(file_list, (GFunc) g_free, NULL);
	g_slist_free(file_list); 
	g_free(filedef_path);
}

void load_projectdefs(GKeyFile* config){
	gsize key_len;
	GError * gerr=NULL;
	gchar** handlers = g_key_file_get_keys(config, "autorun", &key_len, &gerr);
	//if the keyfile has an [autorun] section
	if( gerr==NULL ){
	g_message("found project autorun");
	gchar** handler_key;
	//load any handlers
	foreach_strv( handler_key, handlers){
		//over any existing [autorun] handlers
		g_message(" %s", *handler_key);
	}
	}else{
		g_free(gerr);
	}
	//The caller of g_key_file_get_keys takes ownership of the returned data, and is responsible for freeing it.
	g_free(handlers);
}
