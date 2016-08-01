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
// QT's moc doesn't like BOOST_JOIN ( can be removed in QT 5.0 we think)
#ifndef Q_MOC_RUN 
#include <boost/test/unit_test.hpp>
#endif
#include "test_fixture.hpp"

BOOST_AUTO_TEST_SUITE( IpcMsgNotification )

struct NotificationFixture
    : public SendRecvFixtureBase
{
    int             m_flag;
    int             m_longpolling_clients;
    pthread_cond_t  m_longpolling_clients_cond;
    int             m_clients_that_got_flag;

    NotificationFixture()
        : m_flag(0),
          m_longpolling_clients(0),
          m_longpolling_clients_cond( PTHREAD_COND_INITIALIZER ),
          m_clients_that_got_flag( 0 )
    {}
    
    
    void
    run()
    {
        fprintf( stderr, "Test notification.\n" );
        m_clients_should_longpoll = 1;
        SendRecvFixtureBase::run();
    }

    int
    inner()
    {
        BOOST_REQUIRE( pthread_mutex_unlock( &lock ) == 0 );

        ScopeTrace scope_trace( this, __func__ );
        // assuming that lock is held.

        struct timespec timeout;
        clock_gettime( CLOCK_REALTIME, &timeout );
        timeout.tv_sec += 10;

        int ret=0;
        {
            Locker locker( client_lock );
            while( m_longpolling_clients != m_clients ) {
                int rc = pthread_cond_timedwait( &m_longpolling_clients_cond,
                                                 &client_lock,
                                                 &timeout );
                if( rc == ETIMEDOUT ) {
                    ret = -1;
                    BOOST_CHECK( false && "timed out while waiting for clients to start longpolling" );
                }
            }
        }

        // All clients should be longpolling, now set the flag and notify server
        // that something has changed.
        {
            Locker locker( server_lock );
            m_flag = 1;
        }
        int rc = ipc_msg_server_notify( m_server );
        
        BOOST_REQUIRE( pthread_mutex_lock( &lock ) == 0 );
        BOOST_CHECK_EQUAL( rc, 0 );
        return ret;
    }
    
    
    int
    serverConsumer( const char* buffer,
                    const size_t buffer_bytes,
                    const int part,
                    const int more )
    {
        ScopeTrace scope_trace( this, __func__ );
        return 0;
    }
    
    int
    serverProducer( int* more,
                    char* buffer,
                    size_t* buffer_bytes,
                    const size_t buffer_size,
                    const int part )
    {
        ScopeTrace scope_trace( this, __func__ );
        Locker locker( this->server_lock );
        int flag = m_flag;
        *((int*)buffer) = flag;
        *buffer_bytes = sizeof(flag);
        *more = 0;
        return 0;   
    }

    int
    clientProducer( int* more,
                    char* buffer,
                    size_t* buffer_bytes,
                    const size_t buffer_size,
                    const int part )
    {
        *buffer_bytes = 0;
        *more = 0;
        return 0;   
    }
    
    int
    clientConsumer( const char* buffer,
                    const size_t buffer_bytes,
                    const int part,
                    const int more ) 
    {
        ScopeTrace scope_trace( this, __func__ );
        if( *((int*)buffer) == 0 ) {
            Locker locker( this->client_lock );
            m_longpolling_clients++;

            if( m_longpolling_clients == m_clients ) {
                NOT_MAIN_THREAD_REQUIRE( this, pthread_cond_signal( &m_longpolling_clients_cond ) == 0  );
            }
            return 1;
        }
        else {
            Locker locker( this->client_lock );
            m_clients_that_got_flag++;
        }
        return 0;
    }                                        
    
};

BOOST_FIXTURE_TEST_CASE( wait, NotificationFixture )
{
    ipc_msg_fake_shmem = 1;
    m_clients = 3;
    m_failure_is_an_option = 0;
    BOOST_REQUIRE_EQUAL( ipc_msg_fake_shmem_users, 0 );
    run();
    BOOST_REQUIRE_EQUAL( ipc_msg_fake_shmem_users, 0 );    
    BOOST_REQUIRE_EQUAL( m_clients_that_got_flag, m_clients );
}


BOOST_AUTO_TEST_SUITE_END()
