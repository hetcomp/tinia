#pragma once
#include <GL/glew.h>
#include <librenderlist/RenderList.hpp>
#include <librenderlist/gl/Renderer.hpp>

namespace librenderlist {
namespace gl {

class RenderSetShader : public RenderAction
{
public:
    RenderSetShader( Renderer& renderer, Id id );

    void
    pull( const SetShader* a );

    void
    invoke( RenderState& state );

protected:
    GLuint  m_gl_program;
};



} // of namespace gl
} // of namespace librenderlist
