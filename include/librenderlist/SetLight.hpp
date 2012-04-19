#pragma once
#include <algorithm>
#include <librenderlist/RenderList.hpp>
#include <librenderlist/Action.hpp>

namespace librenderlist {

class SetLight : public Action
{
    friend class DataBase;
public:

    const LightType
    type() const { return m_type;}

    const unsigned int
    index() const { return m_index; }

    const float*
    color() const { return m_color; }

    const float*
    attenuation() const { return m_attenuation; }

    const float*
    falloff() const { return m_falloff; }

    const float*
    fromWorld() const { return m_from_world; }

    const float*
    toWorld() const { return m_to_world; }


    SetLight*
    setType( LightType type )
    { m_type = type; m_db.taint( this, true ); return this; }

    SetLight*
    setIndex( unsigned int index )
    { m_index = index; m_db.taint( this, true ); return this; }

    SetLight*
    setColor( const float* v4 )
    { std::copy( v4, v4+4, m_color ); m_db.taint( this, false ); return this; }

    SetLight*
    setColor( const float red, const float green, const float blue, const float alpha=1.f )
    {
        m_color[0] = red;
        m_color[1] = green;
        m_color[2] = blue;
        m_color[3] = alpha;
        m_db.taint( this, false );
        return this;
    }

    SetLight*
    setAttenuation( float constant, float linear, float quadratic )
    {
        m_attenuation[0] = constant;
        m_attenuation[1] = linear;
        m_attenuation[2] = quadratic;
        m_db.taint( this, false ); return this;
    }

    SetLight*
    setFalloff( const float angle, const float exponent )
    {
        m_falloff[0] = angle;
        m_falloff[1] = exponent;
        m_db.taint( this, false ); return this;
    }


    SetLight*
    setOrientation( const float* from_world, const float* to_world )
    {
        std::copy_n( from_world, 16, m_from_world );
        std::copy_n( to_world, 16, m_to_world );
        m_db.taint( this, false );
        return this;
    }

private:
    LightType       m_type;
    unsigned int    m_index;
    float           m_color[4];
    float           m_attenuation[3];
    float           m_falloff[2];
    float           m_from_world[16];
    float           m_to_world[16];

    SetLight( Id id, DataBase& db, const std::string& name )
        : Action( id, db, name ),
          m_type( LIGHT_AMBIENT ),
          m_index( 0 )
    {
        static const float unit[16] = { 1.f, 0.f, 0.f, 0.f,
                                        0.f, 1.f, 0.f, 0.f,
                                        0.f, 0.f, 1.f, 0.f,
                                        0.f, 0.f, 0.f, 1.f };
        m_color[0] = 1.f;
        m_color[1] = 1.f;
        m_color[2] = 1.f;
        m_color[3] = 1.f;
        m_attenuation[0] = 1.f;
        m_attenuation[1] = 0.f;
        m_attenuation[2] = 0.f;
        m_falloff[0] = 3.14;
        m_falloff[1] = 0.0;
        std::copy_n( unit, 16, m_from_world );
        std::copy_n( unit, 16, m_to_world );
    }
};




} // of namespace librenderlist
