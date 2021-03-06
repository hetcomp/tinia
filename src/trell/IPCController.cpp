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

#include <sys/stat.h>   // mode constants
#include <sys/mman.h>
#include <sys/wait.h>
#include <time.h>
#include <list>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>      // O_* constants
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include "tinia/trell/IPCController.hpp"
#include <tinia/ipc/ipc_msg.h>

namespace tinia {
namespace trell {

namespace {
static const std::string package = "IPCController";

}



IPCController::IPCController( bool is_master )
    : m_ipc_pid( getpid() ),
      m_cleanup_pid( -1 ),
      m_msgbox( NULL ),
      m_is_master( is_master ),
      m_job_state( TRELL_JOBSTATE_NOT_STARTED )
{
    //instances.push_back(this);
}

void
IPCController::finish()
{
    m_job_state = TRELL_JOBSTATE_FINISHED;
    ipc_msg_server_mainloop_break( m_msgbox );
}

void
IPCController::fail()
{
    m_job_state = TRELL_JOBSTATE_FAILED;
    ipc_msg_server_mainloop_break( m_msgbox );
}

void
IPCController::failHard()
{
    fail();
    shutdown();
}

void
IPCController::notify()
{
    ipc_msg_server_notify( m_msgbox );
}

void IPCController::addScript(const std::string &script)
{
    m_scripts.push_back(script);
}

bool IPCController::onGetScripts(size_t &result_size, char *buffer, const size_t buffer_size)
{
    // Initialize result
    result_size = 0;
    std::string header("");
    header.copy(buffer, buffer_size);
    buffer += header.size();
    result_size = header.size();
    for(size_t i = 0; i < m_scripts.size(); ++i) {
        m_scripts[i].copy(buffer, buffer_size);
        buffer += m_scripts[i].size();
        result_size += m_scripts[i].size();
        if(result_size > buffer_size) {
            return false;
        }
    }
    return true;
}



void
IPCController::shutdown()
{
    if( m_cleanup_pid != getpid() ) {
        // The Master (which subclass this class) forks, and the child inherits
        // signal handlers and open io stuff. If the subsequent exec fails, the
        // child terminates and the signal handler is invoked, which in turn
        // closes the parent's message box. To avoid this, we only allow the
        // process that created the message box to clean it up. Also, by setting
        // the cleanup-pid to -1 after we've finished cleaning up avoids trying
        // to cleanup multiple times (we have both a signal handler and an
        // at_exit-handler).
        return;
    }
    if( m_job_state != TRELL_JOBSTATE_TERMINATED_SUCCESSFULLY ) {
        m_job_state = TRELL_JOBSTATE_TERMINATED_UNSUCCESSFULLY;
    }
    sendHeartBeat();
    std::cerr << "Cleaning up.\n";
    cleanup();

    //if( !m_is_master ) {
    //    if( tinia_ipc_msg_client_release( m_master_mbox ) != 0 ) {
    //        std::cerr << "Failed to close messenger.\n";
    //    }
    //    else {
    //        delete reinterpret_cast<char*>( m_master_mbox );
    //    }
    //}

    ipc_msg_server_delete( m_msgbox );

    std::cerr << "Done.\n";
    m_cleanup_pid = -1;
}



IPCController::~IPCController()
{
}


int
IPCController::message_input_handler( tinia_ipc_msg_consumer_func_t* consumer,
                                      void** consumer_data,
                                      void* handler_data,
                                      const char* buffer,
                                      const size_t buffer_bytes )
{
    *consumer = message_consumer;
    *consumer_data = handler_data;
    return 0;
}


int
IPCController::message_output_handler( tinia_ipc_msg_producer_func_t* producer,
                                       void** producer_data,
                                       void* handler_data )
{
    *producer = message_producer;
    *producer_data = handler_data;
    return 0;
}


int
IPCController::message_consumer( void*                     data,
                                 const char*               buffer,
                                 const size_t              buffer_bytes,
                                 const int                 iteration,
                                 const int                 more ) 
{
    static const std::string who = package + ".message_consumer";
    
    IPCController::Context* ctx = reinterpret_cast<IPCController::Context*>( data );
    if( iteration == 0 ) {
        ctx->m_buffer_offset = 0;
    }
    
    if( ctx->m_buffer_size <= ctx->m_buffer_offset + buffer_bytes ) {
        ctx->m_ipc_controller->m_logger_callback( ctx->m_ipc_controller->m_logger_data, 0, who.c_str(),
                                                  "Buffer too small (bufsiz=%ld, bytes=%ld).",
                                                  ctx->m_buffer_size,
                                                  ctx->m_buffer_offset + buffer_bytes );
        return -1;
    }
    memcpy( ctx->m_buffer + ctx->m_buffer_offset, buffer, buffer_bytes );
    ctx->m_buffer_offset += buffer_bytes;
    
    if( !more ) {
        try {
            ctx->m_output_bytes = ctx->m_ipc_controller->handle( reinterpret_cast<tinia_msg_t*>( ctx->m_buffer ),
                                                                 ctx->m_buffer_offset,
                                                                 ctx->m_buffer_size );
        }
        catch( const std::exception& e ) {
            ctx->m_ipc_controller->m_logger_callback( ctx->m_ipc_controller->m_logger_data, 0, who.c_str(),
                                                      "Caught exception: %s.", e.what() );
            return -1;
        }
    }
    return 0;
}

int
IPCController::message_producer( void*         data,
                  int*          more,
                  char*         buffer,
                  size_t*       buffer_bytes,
                  const size_t  buffer_size,
                  const int     iteration )
{
    IPCController::Context* ctx = reinterpret_cast<IPCController::Context*>( data );
    if( iteration == 0 ) {
        ctx->m_buffer_offset = 0;
    }
    size_t bytes = ctx->m_output_bytes - ctx->m_buffer_offset;
    if( buffer_size < bytes ) {
        bytes = buffer_size;
    }
    memcpy( buffer, ctx->m_buffer + ctx->m_buffer_offset, bytes );
    ctx->m_buffer_offset += bytes;
    *buffer_bytes = bytes;
    *more = ctx->m_buffer_offset < ctx->m_output_bytes ? 1 : 0;
    return 0;
}

int
IPCController::handle_periodic(void* data)
{
    IPCController::Context* ctx = reinterpret_cast<IPCController::Context*>( data );
    if( ctx->m_ipc_controller->periodic() ) {
        return 0;
    }
    else {
        return -1;
    }
}


int
IPCController::run(int argc, char **argv)
{
    static const std::string who = package + ".run";
   
    m_cleanup_pid = getpid();

    for( int i=0; environ[i] != NULL; i++ ) {
        m_logger_callback( m_logger_data, 2, who.c_str(),
                           "environ[%d] = \"%s\".", i, environ[i] );
    }

    for( int i=0; i<argc; i++ ) {
        m_logger_callback( m_logger_data, 2, who.c_str(),
                           "argv[%d] = \"%s\".", i, argv[i] );
    }    

    // --- get job id ----------------------------------------------------------
    const char* tinia_job_id = getenv( "TINIA_JOB_ID" );
    if( tinia_ipc_util_valid_jobid( m_logger_callback, m_logger_data, tinia_job_id ) == 0 ) {
        m_job_state = TRELL_JOBSTATE_FAILED;
        m_logger_callback( m_logger_data, 0, who.c_str(),
                           "Problem retrieving env['TINIA_JOB_ID']." );
    }
    else {
        m_id = tinia_job_id;
        
        // --- get master id ---------------------------------------------------
        const char* master_id = getenv( "TINIA_MASTER_ID" );
        if( tinia_ipc_util_valid_jobid( m_logger_callback, m_logger_data, master_id ) == 0 ) {
            m_job_state = TRELL_JOBSTATE_FAILED;
            m_logger_callback( m_logger_data, 0, who.c_str(),
                               "Problem retrieving env['TINIA_MASTER_ID']." );
        }
        else {
            m_master_id = master_id;

            // --- create message server ---------------------------------------
            m_msgbox = ipc_msg_server_create( m_id.c_str(), m_logger_callback, m_logger_data );
            if( m_msgbox == NULL ) {
                m_job_state = TRELL_JOBSTATE_FAILED;
                m_logger_callback( m_logger_data, 0, who.c_str(),
                                   "Problem initializing message server." );
            }
            else {
                //if( m_job_state == TRELL_JOBSTATE_NOT_STARTED && !m_is_master ) {
                //    m_master_mbox = reinterpret_cast<tinia_ipc_msg_client_t*>( new char[tinia_ipc_msg_client_t_sizeof] );
                //    if( tinia_ipc_msg_client_init(  m_master_mbox,
                //                                              m_master_id.c_str(),
                //                                              m_logger_callback,
                //                                              m_logger_data ) != 0 ) {
                //        std::cerr << "Failed to connect to master.\n";
                //        m_job_state = TRELL_JOBSTATE_FAILED;
                //    }
                //}
                
                m_logger_callback( m_logger_data, 2, who.c_str(),
                                   "Finished running ipc setup code, id='%s', master='%s.",
                                   m_id.c_str(), m_master_id.c_str() );
                
                // --- invoke virtual init func --------------------------------
                if( !init() ) {
                    m_job_state = TRELL_JOBSTATE_FAILED;
                    m_logger_callback( m_logger_data, 0, who.c_str(),
                                       "init() failed." );
                }
                else {

                    // --- wee, we're ready to start running! ------------------
                    m_job_state = TRELL_JOBSTATE_RUNNING;
                    sendHeartBeat();
                    
                    Context ctx;
                    ctx.m_ipc_controller = this;
                    ctx.m_buffer_size = 1000*1024*1024;
                    ctx.m_buffer = new char[ctx.m_buffer_size];
                    if( ipc_msg_server_mainloop( m_msgbox,
                                                 handle_periodic, &ctx,
                                                 message_input_handler, &ctx,
                                                 message_output_handler, &ctx ) == 0 )
                    {
                        m_job_state = TRELL_JOBSTATE_TERMINATED_SUCCESSFULLY;
                    }
                    else {
                        m_job_state = TRELL_JOBSTATE_TERMINATED_UNSUCCESSFULLY;
                    }
                    delete reinterpret_cast<char*>( ctx.m_buffer );
                    sendHeartBeat();
                }
            }
        }
    }
    failHard();
    m_logger_callback( m_logger_data, 2, who.c_str(), "Exiting." );
    return EXIT_SUCCESS;
}

TrellMessageType
IPCController::sendSmallMessage( const std::string& message_box_id, TrellMessageType query )
{
    tinia_msg_t q;
    q.type = query;
    
    tinia_msg_t r;
    size_t rs;
    
    if( ipc_msg_client_sendrecv_buffered_by_name( message_box_id.c_str(),
                                                  m_logger_callback, m_logger_data,
                                                  reinterpret_cast<const char*>(&q), sizeof(q),
                                                  reinterpret_cast<char*>(&r), &rs, sizeof(r)) == 0 )
    {
        if( rs == sizeof(r) ) {
            m_logger_callback( m_logger_data, 2, package.c_str(), "Successfully sent small message to '%s'.",
                               message_box_id.c_str() );
            return r.type;
        }
        else {
            return TRELL_MESSAGE_ERROR;
        }
    }
    else {
        m_logger_callback( m_logger_data, 0, package.c_str(), "Failed to send small message to '%s'.",
                           message_box_id.c_str());
        return TRELL_MESSAGE_ERROR;
    }
}


bool
IPCController::sendHeartBeat()
{
    static const std::string func = package + ".sendHeartBeat";
    
    if( m_is_master ) {
        return true; // Don't send heartbeats to oneself.
    }
    

    tinia_msg_heartbeat_t query;
    query.msg.type = TRELL_MESSAGE_HEARTBEAT;
    query.state = m_job_state;
    strncpy( query.job_id, m_id.c_str(), TINIA_IPC_JOBID_MAXLENGTH );
    query.job_id[ TINIA_IPC_JOBID_MAXLENGTH ] = '\0';
    
    tinia_msg_t reply;
    size_t reply_actual;
#ifdef DEBUG
    m_logger_callback( m_logger_data, 2, func.c_str(), "Sending heartbeat to '%s'.", m_master_id.c_str() );
#endif
    if( ipc_msg_client_sendrecv_buffered_by_name( m_master_id.c_str(),
                                                  m_logger_callback, m_logger_data,
                                                  reinterpret_cast<const char*>(&query), sizeof(query),
                                                  reinterpret_cast<char*>(&reply), &reply_actual, sizeof(reply)) != 0 )
    {
        m_logger_callback( m_logger_data, 0, func.c_str(), "Failed to send message to master job." );
        return false;
    }
    else {
        return true;
    }
}




bool
IPCController::periodic()
{
    sendHeartBeat();
    return true;
}


bool
IPCController::init()
{
    return true;
}

void
IPCController::cleanup()
{
}

}

}
