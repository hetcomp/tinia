#pragma once

#include <QObject>
#include <QMutex>
#include <QWaitCondition>
#include <QTextStream>
#include "tinia/jobcontroller.hpp"
#include "tinia/qtcontroller/ImageSource.hpp"

namespace tinia {
namespace qtcontroller {
namespace impl {

class OpenGLServerGrabber : public QObject
{
    Q_OBJECT
public:
    explicit OpenGLServerGrabber(QObject *parent);

    ~OpenGLServerGrabber();

    /** Mutex that governs exclusive access to this object. */
    QMutex*
    exclusiveAccessMutex()
    { return &m_mainMutex; }

    /** Returns a pointer to the grabbed image.
     *
     * \note \ref exclusiveAccessMutex must be held before invocation.
     */
    const unsigned char*
    imageBuffer() const
    { return m_buffer; }

    /** Grabs an image of a view
     *
     * \note Must be invoked in the thread that holds the OpenGL context,
     *       usually the main/GUI-thread.
     * \note \ref exclusiveAccessMutex must be held before invocation.
     */
    void
    grabRGB( tinia::jobcontroller::OpenGLJob* job,
             unsigned int width,
             unsigned int height,
             const std::string &key);

    /** Grabs the depth buffer of a view
     *
     * \note The buffer is transformed to an RGB image with depth values encoded as fixed point numbers.
     * \note Must be invoked in the thread that holds the OpenGL context,
     *       usually the main/GUI-thread.
     * \note \ref exclusiveAccessMutex must be held before invocation.
     */
    void
    grabDepth( tinia::jobcontroller::OpenGLJob* job,
               unsigned int width,
               unsigned int height,
               const std::string &key);

private:
    void setupOpenGL();
    void resize(unsigned int width, unsigned int height);

    QMutex          m_mainMutex;
    unsigned char*  m_buffer;
    size_t          m_buffer_size;
    bool            m_openglIsReady;
    unsigned int    m_fbo;
    unsigned int    m_renderbufferRGBA;
    unsigned int    m_renderbufferDepth;
    unsigned int    m_width;
    unsigned int    m_height;
};

} // namespace impl
} // namespace qtcontroller
} // namespace tinia

