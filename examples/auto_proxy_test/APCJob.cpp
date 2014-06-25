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

#include <GL/glew.h>
#include <tinia/renderlist/Buffer.hpp>
#include <tinia/renderlist/Draw.hpp>
#include <tinia/renderlist/SetViewCoordSys.hpp>
#include <tinia/renderlist/Shader.hpp>
#include <tinia/renderlist/SetShader.hpp>
#include <tinia/renderlist/SetInputs.hpp>
#include <tinia/renderlist/SetUniforms.hpp>
#include <tinia/renderlist/SetLight.hpp>
#include <tinia/renderlist/SetLocalCoordSys.hpp>
#include <tinia/renderlist/SetFramebuffer.hpp>
#include <tinia/renderlist/SetFramebufferState.hpp>
#include <tinia/renderlist/SetPixelState.hpp>
#include <tinia/renderlist/SetRasterState.hpp>
#include "utils.hpp"
#include "APCJob.hpp"
#include "tinia/model/GUILayout.hpp"
#include <iostream>
#include "tinia/model/File.hpp"

namespace tinia {
namespace example {
APCJob::APCJob()
{
}


bool APCJob::init()
{
    tinia::model::Viewer viewer;
    m_model->addElement("viewer", viewer);
    m_model->addElement<std::string>( "boundingbox", "-2.0 -2.0 -2.0 2.0 2.0 2.0" );

    // Adding variables to the model
    // Note that these values are not communicated to the ProxyRenderer until they are actually changed, due to the use
    // of listeners. (Should maybe fix this, by some initialization routine.)
    {
        m_model->addElement<bool>( "useAutoProxy", true );          // This turns on the new autoProxy
        m_model->addElement<bool>( "autoProxyDebugging", true );    // Should not be modified through the GUI. Not defining equals "false". (Is it ok to toggle this? Not sure. Maybe.)
        m_model->addAnnotation("autoProxyDebugging", "Debug mode");

        m_model->addElement<bool>( "debugSplatCol", false );
        m_model->addAnnotation("debugSplatCol", "Index coloring (r, g, b, y, c, m)");
        m_model->addElement<bool>( "decayMode", false );
        m_model->addAnnotation("decayMode", "Splats decaying from center");
        m_model->addElement<bool>( "roundSplats", false );
        m_model->addAnnotation("roundSplats", "Circular splats");
        m_model->addElement<bool>( "screenSpaceSized", true );
        m_model->addAnnotation("screenSpaceSized", "Screen-space-sized splats");
        m_model->addConstrainedElement<int>("overlap", 200, 1, 300);
        m_model->addAnnotation("overlap", "Overlap factor)");
        m_model->addElement<bool>( "alwaysShowMostRecent", true );
        m_model->addAnnotation("alwaysShowMostRecent", "Always show most recent proxy model");
        m_model->addConstrainedElement<int>("splats", 16, 2, 512);
        m_model->addAnnotation("splats", "Number of splats)");
        m_model->addElement<bool>( "resetAllModels", false );
        m_model->addAnnotation("resetAllModels", "Remove all models, and update just once");
        m_model->addElement<bool>( "useISTC", true );
        m_model->addAnnotation("useISTC", "Use intra-splat texcoo");
        m_model->addElement<bool>( "splatOutline", false );
        m_model->addAnnotation("splatOutline", "Square splat outline");
        m_model->addElement<bool>( "reloadShader", false );
        m_model->addAnnotation("reloadShader", "Reload shader");
        m_model->addElement<bool>( "useFragExt", true );
        m_model->addAnnotation("useFragExt", "Use FragDepthExt if available");
        m_model->addElement( "fragExtStatus", "---" );
        m_model->addElement( "consoleLog", "---" );
        m_model->addElement<int>( "cntr", 0 );
    }

    // Setting up the mainGrid containing the GUI elements
    tinia::model::gui::Grid *mainGrid = new tinia::model::gui::Grid(100, 4);
    {
        int row = 0;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("useAutoProxy"));
        mainGrid->setChild(row, 1, new tinia::model::gui::CheckBox("autoProxyDebugging"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("debugSplatCol"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("decayMode"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("roundSplats"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("screenSpaceSized"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("overlap"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("overlap", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("overlap", true));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("alwaysShowMostRecent"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::HorizontalSlider("splats"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("splats", false));
        mainGrid->setChild(row, 2, new tinia::model::gui::Label("splats", true));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::Button("resetAllModels"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("useISTC"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("splatOutline"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::Button("reloadShader"));
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::CheckBox("useFragExt"));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("fragExtStatus", true)); // true) We get the text string connected to the element, false) name of element
        row++;
        mainGrid->setChild(row, 0, new tinia::model::gui::Label("consoleLog", false));
        mainGrid->setChild(row, 1, new tinia::model::gui::Label("consoleLog", true));
        // More elements...
    }

    // Setting up root consisting of canvas + mainGrid
    {
        tinia::model::gui::HorizontalLayout *rootLayout = new tinia::model::gui::HorizontalLayout();
        {
            tinia::model::gui::Canvas *canvas = new tinia::model::gui::Canvas("viewer", "renderlist", "boundingbox" );
            rootLayout->addChild(canvas);
        }
        rootLayout->addChild(mainGrid);
        m_model->setGUILayout(rootLayout, tinia::model::gui::DESKTOP);
    }

    return true;
}


APCJob::~APCJob()
{
}

void APCJob::stateElementModified(tinia::model::StateElement *stateElement)
{
}

bool APCJob::renderFrame(const std::string &session, const std::string &key, unsigned int fbo, const size_t width, const size_t height)
{
    static bool firsttime = true;
    if (firsttime) {
        std::cout << "Setting up texture" << std::endl;
        glGenTextures(1, &m_tex);
        glBindTexture(GL_TEXTURE_2D, m_tex);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
        const int n=512;
        std::vector<unsigned char> texData(n*n*3, 255);
        for (int i=0; i<n; i+=(n/16)) {
            for (int j=0; j<n; j++) {
                texData[3*(i*n+j) + 0] = 0;
                texData[3*(i*n+j) + 1] = 0;
                texData[3*(i*n+j) + 2] = 0;
                texData[3*(j*n+i) + 0] = 0;
                texData[3*(j*n+i) + 1] = 0;
                texData[3*(j*n+i) + 2] = 0;
            }
        }

        glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP, GL_TRUE);

        glTexImage2D(GL_TEXTURE_2D,
                     0, // mipmap level
                     GL_RGB, n, n, 0, GL_RGB, GL_UNSIGNED_BYTE, &texData[0]);

        //glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        firsttime = false;
    }

    // Simulated high latency
    // usleep(200000);

    glEnable(GL_DEPTH_TEST);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

    glViewport(0, 0, width, height);
    tinia::model::Viewer viewer;
    m_model->getElementValue( key, viewer);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadMatrixf( viewer.projectionMatrix.data() );
    glMatrixMode(GL_MODELVIEW);
    glLoadMatrixf( viewer.modelviewMatrix.data() );

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    glBindTexture(GL_TEXTURE_2D, m_tex);

    glBegin(GL_POLYGON);
    glColor3f(   1.0,  0.0, 0.0 );
    glTexCoord2f(1, 1);
    glVertex3f(  0.5, -0.5, -0.5 );
    glTexCoord2f(1, 0);
    glVertex3f(  0.5,  0.5, -0.5 );
    glTexCoord2f(0, 0);
    glVertex3f( -0.5,  0.5, -0.5 );
    glTexCoord2f(0, 1);
    glVertex3f( -0.5, -0.5, -0.5 );
    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(  .0,  0.0,  1.0 );
    glTexCoord2f(1, 1);
    glVertex3f( 0.5, -0.5, -0.5 );
    glTexCoord2f(1, 0);
    glVertex3f( 0.5,  0.5, -0.5 );
    glTexCoord2f(0, 0);
    glVertex3f( 0.5,  0.5,  0.5 );
    glTexCoord2f(0, 1);
    glVertex3f( 0.5, -0.5,  0.5 );
    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(   1.0,  0.5,  0.0 );
    glTexCoord2f(1, 1);
    glVertex3f( -0.5, -0.5,  0.5 );
    glColor3f(   0.0,  1.0,  0.5 );
    glTexCoord2f(1, 0);
    glVertex3f( -0.5,  0.5,  0.5 );
    glColor3f(   1.0,  0.0,  0.5 );
    glTexCoord2f(0, 0);
    glVertex3f( -0.5,  0.5, -0.5 );
    glColor3f(   0.5,  0.0,  0.5 );
    glTexCoord2f(0, 1);
    glVertex3f( -0.5, -0.5, -0.5 );
    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(   0.0,  1.0, 0.0 );
    glTexCoord2f(1, 1);
    glVertex3f(  0.5, -0.5, 0.5 );
    glTexCoord2f(1, 0);
    glVertex3f(  0.5,  0.5, 0.5 );
    glTexCoord2f(0, 0);
    glVertex3f( -0.5,  0.5, 0.5 );
    glTexCoord2f(0, 1);
    glVertex3f( -0.5, -0.5, 0.5 );
    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(   1.0,  0.0,  1.0 );
    glTexCoord2f(1, 1);
    glVertex3f(  0.5,  0.5,  0.5 );
    glTexCoord2f(1, 0);
    glVertex3f(  0.5,  0.5, -0.5 );
    glTexCoord2f(0, 0);
    glVertex3f( -0.5,  0.5, -0.5 );
    glTexCoord2f(0, 1);
    glVertex3f( -0.5,  0.5,  0.5 );
    glEnd();

    glBegin(GL_POLYGON);
    glColor3f(   0.0,  1.0,  1.0 );
    glTexCoord2f(1, 1);
    glVertex3f(  0.5, -0.5, -0.5 );
    glTexCoord2f(1, 0);
    glVertex3f(  0.5, -0.5,  0.5 );
    glTexCoord2f(0, 0);
    glVertex3f( -0.5, -0.5,  0.5 );
    glTexCoord2f(0, 1);
    glVertex3f( -0.5, -0.5, -0.5 );
    glEnd();

    glDisable(GL_TEXTURE_2D);

    CHECK_GL;

    return true;
}

const tinia::renderlist::DataBase*
APCJob::getRenderList( const std::string& session, const std::string& key )
{
    return &m_renderlist_db;
}
}
}
