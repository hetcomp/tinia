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

#include <iostream>
#include <tinia/renderlist/Logger.hpp>
#include <tinia/renderlist/Draw.hpp>
#include <tinia/renderlist/gl/RenderDraw.hpp>
#include <tinia/renderlist/gl/Renderer.hpp>
#include <tinia/renderlist/gl/RenderBuffer.hpp>
#include "Utils.hpp"

namespace tinia {
namespace renderlist {
namespace gl {

static const std::string package = "renderlist.gl.RenderDraw";


RenderDraw::RenderDraw( Renderer& renderer, Id id )
    : RenderAction( renderer, id ),
      m_index_buffer( 0 ),
      m_index_type( GL_UNSIGNED_INT ),
      m_index_size( 0 ),
      m_mode( GL_POINTS ),
      m_first( 0 ),
      m_count( 0 )
{}

void
RenderDraw::pull( const Draw* a )
{
    Logger log = getLogger( package + ".pull" );

    m_index_buffer = 0;
    if( a->isIndexed() ) {
        const RenderBuffer* b = m_renderer.buffer( a->indexBufferId() );
        if( b == NULL ) {
            RL_LOG_ERROR( log, "Failed to locate index buffer id=" << a->indexBufferId() );
            return;
        }
        GLint buffer = b->buffer();
        switch( b->type() ) {
        case GL_INT:
            m_index_type = GL_UNSIGNED_INT;
            m_index_size = sizeof(GLuint);
            break;
        default:
            RL_LOG_ERROR( log, "Unsupported index type 0x" << std::hex << b->type() << std::dec );
            return;
        }
        m_index_buffer = buffer;
    }
    switch( a->primitiveType() ) {
    case PRIMITIVE_POINTS:
        m_mode = GL_POINTS;
        break;
    case PRIMITIVE_LINES:
        m_mode = GL_LINES;
        break;
    case PRIMITIVE_LINE_STRIP:
        m_mode = GL_LINE_STRIP;
        break;
    case PRIMITIVE_LINE_LOOP:
        m_mode = GL_LINE_LOOP;
        break;
    case PRIMITIVE_TRIANGLES:
        m_mode = GL_TRIANGLES;
        break;
    case PRIMITIVE_TRIANGLE_STRIP:
        m_mode = GL_TRIANGLE_STRIP;
        break;
    case PRIMITIVE_TRIANGLE_FAN:
        m_mode = GL_TRIANGLE_FAN;
        break;
    case PRIMITIVE_QUADS:
        m_mode = GL_QUADS;
        break;
    case PRIMITIVE_QUAD_STRIP:
        m_mode = GL_QUAD_STRIP;
        break;
    }
    m_first = a->first();
    m_count = a->count();
    RL_LOG_TRACE( log, "Created draw, first=" << m_first << ", count=" << m_count );
}

void
RenderDraw::invoke( RenderState& state )
{

    if( m_index_buffer != 0u ) {
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, m_index_buffer );
        glDrawElements( m_mode,
                        m_count,
                        m_index_type,
                        reinterpret_cast<const GLvoid*>( m_index_size*m_first ) );
        glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, 0 );
    }
    else {
        glDrawArrays( m_mode, m_first, m_count );
    }
#ifdef DEBUG
    Logger log = getLogger( package + ".invoke" );
    CHECK_GL;
#endif
}


} // of namespace gl
} // of namespace renderlist
} // of namespace tinia
