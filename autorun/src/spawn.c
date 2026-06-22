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
#include "spawn.h"
#include "autorun.h" 
 
void dispatch_run(const gchar * interceptor, GeanyDocument *doc)
{
	GSList* command_list=NULL;
	g_message ("target interceptor %s" ,interceptor);

	//build the commands backwads
	GSList* elem=NULL;
	foreach_slist(elem,autorun_globals->filedef_commands )
	{
		AUTORUN_CMD*cmd =((AUTORUN_CMD*)elem->data);
		if(!cmd->invalid && g_strcmp0(cmd->interceptor,interceptor)==0 && cmd->file_type==doc->file_type  )
		{
			command_list =g_slist_prepend(command_list, cmd);
		}
	}
	
	foreach_slist(elem,autorun_globals->project_commands )
	{
		AUTORUN_CMD*cmd =((AUTORUN_CMD*)elem->data);
		if(!cmd->invalid && g_strcmp0(cmd->interceptor,interceptor)==0 && cmd->file_type==doc->file_type  )
		{
			command_list =g_slist_prepend(command_list, cmd);
		}
	}
	//now flip it on them.
	command_list = g_slist_reverse(command_list);
	g_message ("command list in %s %i" ,interceptor,g_slist_length(command_list));
	if(g_slist_length(command_list)>0){
		//now if there are any cmds in scope, honor order
		
		GSList* meh[g_slist_length(command_list)]= {};
		foreach_slist(elem, command_list){
			AUTORUN_CMD*cmd =((AUTORUN_CMD*)elem->data);
			g_message("Staged %s in %s # %i", cmd->command, cmd->working_dir, cmd->order );
			meh[cmd->order] =elem;
		}

		for(uint i=0; i<g_slist_length(command_list); i++){
			if(meh[i]==NULL){
				break;//stop on the first null;
			}
			AUTORUN_CMD* cmd=(AUTORUN_CMD*)meh[i]->data;
			g_message("Run %s in %s # %i", cmd->command, cmd->working_dir, cmd->order );
		}
	}

	g_slist_free(command_list);
}

gboolean run_command(AUTORUN_CMD* cmd)
{
	if(cmd->invalid)
	{
		return FALSE;
	}
	return TRUE;
}
