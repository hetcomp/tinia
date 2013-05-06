/* Copyright STIFTELSEN SINTEF 2012
 * 
 * This file is part of the Tinia Framework.
 * 
 * The Tinia Framework is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * The Tinia Framework is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 * 
 * You should have received a copy of the GNU Affero General Public License
 * along with the Tinia Framework.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <httpd.h>
#include <http_config.h>
#include <http_protocol.h>
#include <http_main.h>
#include <util_script.h>
#include <ap_config.h>
#include <apr_strings.h>
#include <http_log.h>

#include <sys/types.h>
#include <unistd.h>

#include <semaphore.h>
#include <strings.h>

#include <stdio.h>

#include "mod_trell.h"





module AP_MODULE_DECLARE_DATA trell_module;






/** Apache's entry-point to mod_trell. */
static int trell_handler_body(request_rec *r)
{
    if (!r->handler || strcmp(r->handler, "trell") ) {
        return DECLINED;
    }

    // And we require a path
    if( r->path_info == NULL ) {
        ap_log_rerror( APLOG_MARK, APLOG_NOTICE, HTTP_NOT_FOUND, r,
                       "mod_trell: Path missing" );
        return HTTP_NOT_FOUND;
    }

    // Now, get the server setup. Check if it is OK, or bail out if not.
    trell_sconf_t* sconf = ap_get_module_config( r->server->module_config, &trell_module );
    if( sconf == NULL || sconf->m_well_formed == 0 ) {
        ap_log_rerror( APLOG_MARK, APLOG_CRIT, 0, r,
                       "mod_trell: Illegal configuration." );
        return HTTP_INTERNAL_SERVER_ERROR;
    }

    int code;
    trell_dispatch_info_t dispatch_info;
    code = trell_decode_path_info( &dispatch_info, r );
    if( code != OK ) {
        ap_log_rerror( APLOG_MARK, APLOG_ERR, 0, r,
                       "mod_trell: decode_path_info failed to decode request='%s'", r->path_info );
        return code;
    }


#if 0
    ap_log_rerror( APLOG_MARK, APLOG_ERR, 0, r,
                   "mod_trell: decode_path_info returned c=%d, r=%d, j='%s', s='%s', r='%s' rev=%d, ac=%d, key=%s, w=%d, h=%d",
                   dispatch_info.m_component,
                   dispatch_info.m_request,
                   dispatch_info.m_jobid,
                   dispatch_info.m_sessionid,
                   dispatch_info.m_requestname,
                   dispatch_info.m_revision,
                   dispatch_info.m_mod_action,
                   dispatch_info.m_key,
                   dispatch_info.m_width,
                   dispatch_info.m_height );
#endif

    switch( dispatch_info.m_component ) {

    case TRELL_COMPONENT_OPS:
        switch( dispatch_info.m_request ) {
        case TRELL_REQUEST_RPC_XML:
            switch( dispatch_info.m_mod_action ) {
            case TRELL_MOD_ACTION_RESTART_MASTER:
                return trell_ops_do_restart_master( sconf, r );
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
        break;

    case TRELL_COMPONENT_MASTER:
        switch( dispatch_info.m_request ) {
        case TRELL_REQUEST_RPC_XML:
            return trell_job_rpc_handle( sconf, r,
                                         sconf->m_rpc_master_schema,
                                         sconf->m_master_id );
        default:
            break;
        }

        break;

    case TRELL_COMPONENT_JOB:

        switch( dispatch_info.m_request ) {
        case TRELL_REQUEST_STATIC_FILE:
            return trell_send_reply_static_file( sconf, r, &dispatch_info );
            break;
        case TRELL_REQUEST_POLICY_UPDATE_XML:
            return trell_handle_get_model_update( sconf, r, &dispatch_info );
            break;
        case TRELL_REQUEST_STATE_UPDATE_XML:
            return trell_handle_update_state( sconf, r, &dispatch_info );
            break;
        case TRELL_REQUEST_PNG:
            // Check if a model update is piggy-backed on request.
            if( r->method_number == M_POST ) {
                int rv = trell_handle_update_state( sconf, r, &dispatch_info );
                if( rv != HTTP_NO_CONTENT ) {
                    // Something went wrong with the update, bail out.
                    return rv;
                }
            }
            return trell_handle_get_snapshot( sconf, r, &dispatch_info );
            break;
        case TRELL_REQUEST_GET_RENDERLIST:
            // Check if a model update is piggy-backed on request.
            if( r->method_number == M_POST ) {
                int rv = trell_handle_update_state( sconf, r, &dispatch_info );
                if( rv != HTTP_NO_CONTENT ) {
                    // Something went wrong with the update, bail out.
                    return rv;
                }
            }
            return trell_handle_get_renderlist( sconf, r, &dispatch_info );
            break;
        case TRELL_REQUEST_GET_SCRIPT:
            return trell_handle_get_script( sconf, r, &dispatch_info);

        default:
            break;
        }
        break;

    default:
        break;
    }
    ap_log_rerror( APLOG_MARK, APLOG_ERR, HTTP_NOT_FOUND, r,
                   "mod_trell: Illegal path '%s'", r->path_info );
    return HTTP_NOT_FOUND;
}


static int trell_handler(request_rec *r)
{
    ap_log_rerror( APLOG_MARK, APLOG_NOTICE, 0, r, "mod_trell: %d: begin.", getpid() );
    int retval = trell_handler_body( r );
    ap_log_rerror( APLOG_MARK, APLOG_NOTICE, 0, r, "mod_trell: %d: end = %d.", getpid(), retval );
    return retval;
}


static
xmlSchemaPtr
trell_child_init_parse_schema( const char* path, const char* file, apr_pool_t* process_pool )
{
    const char* url = apr_psprintf( process_pool, "%s/%s", path, file );
    ap_log_perror( APLOG_MARK, APLOG_NOTICE, 0, process_pool,
                   "mod_trell: Parsing schema %s", url );

    xmlSchemaPtr schema = NULL;
    xmlSchemaParserCtxtPtr pctx = xmlSchemaNewParserCtxt( url );
    if( pctx == NULL ) {
        ap_log_perror( APLOG_MARK, APLOG_WARNING, 0, process_pool,
                       "mod_trell: Failed to read job rpc schema" );
    }
    else {
        schema = xmlSchemaParse( pctx );
        if( schema == NULL ) {
            ap_log_perror( APLOG_MARK, APLOG_WARNING, 0, process_pool,
                           "mod_trell: Failed to parse schema %s", url );
        }
        xmlSchemaFreeParserCtxt( pctx );
    }
    return schema;
}

static void
trell_child_init(apr_pool_t *p, server_rec *s)
{
    // Check if config is well-formed
    struct mod_trell_svr_conf* svr_conf = ap_get_module_config( s->module_config, &trell_module );
    if( svr_conf == NULL ) {
        ap_log_perror( APLOG_MARK, APLOG_CRIT, 0, s->process->pool,
                       "mod_trell: No config." );
        return;
    }
    svr_conf->m_well_formed = 1;

    if( svr_conf->m_master_id == NULL ) {
        ap_log_perror( APLOG_MARK, APLOG_CRIT, 0, s->process->pool,
                       "mod_trell: TrellMasterId has not been set" );
        svr_conf->m_well_formed = 0;
    }

    if( svr_conf->m_master_exe == NULL ) {
        ap_log_perror( APLOG_MARK, APLOG_CRIT, 0, s->process->pool,
                       "mod_trell: TrellMasterExe has not been set" );
        svr_conf->m_well_formed = 0;
    }

    if( svr_conf->m_app_root_dir == NULL ) {
        ap_log_perror( APLOG_MARK, APLOG_CRIT, 0, s->process->pool,
                       "mod_trell: TrellAppRootDir has not been set" );
        svr_conf->m_well_formed = 0;
    }

    if( svr_conf->m_job_www_root == NULL ) {
        ap_log_perror( APLOG_MARK, APLOG_CRIT, 0, s->process->pool,
                       "mod_trell: TrellJobWWWRoot has not been set" );
        svr_conf->m_well_formed = 0;
    }

    if( svr_conf->m_well_formed == 0 ) {
        ap_log_perror( APLOG_MARK, APLOG_CRIT, 0, s->process->pool,
                       "mod_trell: Illegal configuration, giving up" );
    }



    xmlInitParser();
    xmlInitThreads();

    // Parse schemas
    if( svr_conf->m_schema_root_dir != NULL ) {
        void* orig_error_cb = xmlGenericErrorContext;
        xmlGenericErrorFunc orig_error_func = xmlGenericError;
        xmlSetGenericErrorFunc( (void*)s, trell_xml_error_s_cb );

        svr_conf->m_rpc_ops_schema =
                trell_child_init_parse_schema( svr_conf->m_schema_root_dir,
                                               "rpc_ops.xsd",
                                               s->process->pool );

        svr_conf->m_rpc_master_schema =
                trell_child_init_parse_schema( svr_conf->m_schema_root_dir,
                                               "rpc_master.xsd",
                                               s->process->pool );

        svr_conf->m_rpc_job_schema =
                trell_child_init_parse_schema( svr_conf->m_schema_root_dir,
                                               "rpc_job.xsd",
                                               s->process->pool );

        svr_conf->m_rpc_reply_schema =
                trell_child_init_parse_schema( svr_conf->m_schema_root_dir,
                                               "reply.xsd",
                                               s->process->pool );

        xmlSetGenericErrorFunc( orig_error_cb, orig_error_func );
    }

}


/** Function to create our per-server configuration struct. */
static void*
mod_trell_create_svr_conf( apr_pool_t* pool, server_rec* s )
{
    struct mod_trell_svr_conf* cfg = apr_pcalloc( pool, sizeof( struct mod_trell_svr_conf ) );
    cfg->m_well_formed = 0;
    cfg->m_master_id = NULL;
    cfg->m_master_exe = NULL;
    cfg->m_app_root_dir = NULL;
    cfg->m_schema_root_dir = NULL;
    cfg->m_job_www_root = NULL;
    cfg->m_rpc_ops_schema = NULL;
    cfg->m_rpc_master_schema = NULL;
    cfg->m_rpc_job_schema = NULL;
    cfg->m_rpc_reply_schema = NULL;
    return cfg;
}

/** Function to merge two of our per-server configuration structs. */
static void*
mod_trell_merge_svr_conf( apr_pool_t* pool, void* base_, void* add_ )
{
    struct mod_trell_svr_conf* base = base_;
    struct mod_trell_svr_conf* add = add_;
    struct mod_trell_svr_conf* res = mod_trell_create_svr_conf( pool,  NULL );
    res->m_master_id    = (add->m_master_id == NULL )   ? base->m_master_id    : add->m_master_id;
    res->m_master_exe   = (add->m_master_exe == NULL )  ? base->m_master_exe   : add->m_master_exe;
    res->m_app_root_dir = (add->m_app_root_dir == NULL) ? base->m_app_root_dir : add->m_app_root_dir;
    res->m_schema_root_dir = (add->m_schema_root_dir == NULL) ? base->m_schema_root_dir : add->m_schema_root_dir;
    res->m_job_www_root = (add->m_job_www_root == NULL) ? base->m_job_www_root : add->m_job_www_root;
    return res;
}

/** Callback used by the apache configuration parsing.
  *
  * It sets a string constant pointer at the offset passed in the mconfig.
  *
  */
static const char*
mod_trell_conf_string_set_callback( cmd_parms* cmd, void* cfg, const char* val )
{
    char* svr = ap_get_module_config( cmd->server->module_config, &trell_module );
    int offset = (int)(long)cmd->info;
    *(const char**)((char*)svr + offset) = val;
    return NULL;
}


/** Definitions of the commands that we accept from httpd.conf. */
static const command_rec mod_trell_commands[] = {
    AP_INIT_TAKE1( "TrellMasterId",
                   mod_trell_conf_string_set_callback,
                   (void*)APR_OFFSETOF(struct mod_trell_svr_conf, m_master_id),
                   RSRC_CONF,
                   "Id of the master"
    ),

    AP_INIT_TAKE1( "TrellMasterExe",
                   mod_trell_conf_string_set_callback,
                   (void*)APR_OFFSETOF(struct mod_trell_svr_conf, m_master_exe),
                   RSRC_CONF,
                   "Path of the master executable."
    ),

    AP_INIT_TAKE1( "TrellAppRoot",
                   mod_trell_conf_string_set_callback,
                   (void*)APR_OFFSETOF(struct mod_trell_svr_conf, m_app_root_dir),
                   RSRC_CONF,
                   "Root directory where trell applications reside"
    ),
    AP_INIT_TAKE1( "TrellSchemaRoot",
                   mod_trell_conf_string_set_callback,
                   (void*)APR_OFFSETOF(struct mod_trell_svr_conf, m_schema_root_dir),
                   RSRC_CONF,
                   "Root directory where trell schemas reside"
    ),
    AP_INIT_TAKE1( "TrellJobWWWRoot",
                   mod_trell_conf_string_set_callback,
                   (void*)APR_OFFSETOF(struct mod_trell_svr_conf, m_job_www_root),
                   RSRC_CONF,
                   "Root directory where static job www resources reside"
    ),


    { NULL }
};


static void
mod_trell_register_hooks (apr_pool_t *p)
{
    ap_hook_handler(trell_handler, NULL, NULL, APR_HOOK_LAST );
//    ap_hook_pre_config( trell_pre_config, NULL, NULL, APR_HOOK_LAST );
//    ap_hook_post_config( trell_post_config, NULL, NULL, APR_HOOK_LAST );
    ap_hook_child_init( trell_child_init, NULL, NULL, APR_HOOK_LAST );
}

module AP_MODULE_DECLARE_DATA trell_module =
{
	STANDARD20_MODULE_STUFF,
	NULL,
	NULL,
        mod_trell_create_svr_conf,
        mod_trell_merge_svr_conf,
        mod_trell_commands,
	mod_trell_register_hooks,
};

