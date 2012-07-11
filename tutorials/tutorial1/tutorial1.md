Tutorial 1: Hello World {#tut_tutorial1}
===

In this tutorial we demonstrate how to create a simple OpenGL Hello World-application
in Tinia. We will draw a simple triangle and allow trackball rotation.

Familiarity with basic OpenGL and C++ with object orientation is assumed.

The program created in this tutorial will run both as a desktop program and as a
server/client program.

The program consists of two files: The Job class definition and the main file. One
main file will be created for the desktop program, and one for the web program.

\tableofcontents

The Job class
---
The Job class defines the methods which will be called to interact with our program.

We subclass `tinia::jobcontroller::OpenGLJob` as we're going to make an OpenGL program.

Tinia provides the convenience header `tinia/tinia.hpp` which includes everything we need. We also include
the glew header since we're going to do OpenGL rendering. The user is free to choose whatever OpenGL wrangler
he wants.
\snippet Tutorial1_Job.hpp headers

Notice our class, `tinia::tutorial::Tutorial1Job` is a subclass of `tinia::jobcontroller::OpenGLJob`,
as we're going to do OpenGL rendering.
To utilize the superclass, we need to override `tinia::jobcontroller::OpenGLJob::renderFrame`. We're
not using proxy geometry for this tutorial, so we don't need to reimplement `tinia::jobcontroller::OpenGLJob::getRenderList`.

\snippet Tutorial1_Job.hpp class

### The ExposedModel

Every subclass of `tinia::jobcontroller::Job` has an instance of `tinia::model::ExposedModel` named `m_model`,
hereby referenced to as the model. The model defines the variables which are exposed to the user interface.
Some variables in the model will not be directly visible to user, others will typically be visible through
GUI widgets such as textboxes and spinboxes.

In the constructor of `tinia::tutorial::Tutorial1Job` we add an element of type `tinia::model::Viewer`
to the model which we inherited from `tinia::jobcontroller::Job`. Objects of type `tinia::model::Viewer` contains the
necessary information to do OpenGL rendering. The method `tinia::model::ExposedModel::addElement` takes two parameters:
the key and the the value. The key is completely user defined; the user is free to choose any string as a key, as long as the
key is unique within the model. The key will later be used for looking up the value.

We also add a key with name "boundingbox". The default viewer in Tinia will look for an element with this name to find the
boundingbox for the geometry. The boundingbox is specified as a string with the lower left corner first, then the upper right corner.

\snippet Tutorial1_Job.hpp ctor

We're utilizing Tinia's ability to automatically generate a GUI based on the model, so we don't need
to specify a GUI.

### Rendering OpenGL

The method `tinia::jobcontroller::OpenGLJob::renderFrame` will be called whenever there's a change in the model.
This happens, for instance, when the user interacts with the mouse on the OpenGL canvas.

In `tinia::tutorial::Tutorial1Job::renderFrame` we first obtain the Viewer object we defined in the constructor.
\snippet Tutorial1_Job.hpp viewer

Objects of type `tinia::model::Viewer` contain the ModelView and Projection matrices. The matrices are stored as row-major in a `std::array<float, 16>`.
You may treat the `data()` any way you'd like. In this example, we hand them directly to the `glLoadMatrixf` function, though a typical use would be to use
them as uniform values to a shader.

\snippet Tutorial1_Job.hpp matrices

The rest of the `renderFrame` method is just simple OpenGL rendering. Any OpenGL call is allowed,
as long as the final framebuffer is rendered to the framebuffer specified by the `fbo` parameter.
\snippet Tutorial1_Job.hpp renderloop

Lastly we return `true` to signalize everything went OK:
\snippet Tutorial1_Job.hpp return

The full `renderFrame` thus becomes
\snippet Tutorial1_Job.hpp renderframe

The Desktop Main File
---
Every Tinia program is controlled by a subclass of `tinia::jobcontroller::Controller`. The controller is responsible
for creating a GUI and handling interactions with the user. For desktop programs, one should use `tinia::qtcontroller::QTController`.

First we include the the code for the Job we've written, then we include the desktop controller `tinia/qtcontroller/QTController.hpp`.
\snippet tutorial1_desktop.cpp headers

We create an instance of our `tinia::tutorial::Tutorial1Job` class
\snippet tutorial1_desktop.cpp job

We also create an instance of `tinia::qtcontroller::QTController`
\snippet tutorial1_desktop.cpp controller

Then we need to hand the job to the controller
\snippet tutorial1_desktop.cpp jobtocontroller

And lastly we run the program
\snippet tutorial1_desktop.cpp run

The whole desktop main is then
\include tutorial1_desktop.cpp

### Running the desktop program
Starting the program should show something similar to this:
\image html tutorial1_desktop.png "Screenshot of the desktop job from Tutorial1."
Notice how the first line displays our boundingbox. This is a caveat of the default GUI generated by
Tinia, as it will display all the elements in the model. See [Tutorial 2](@ref tut_tutorial2) for how to specify your own GUI.

The Web Main File
---
The main file for the web application is quite similar to the dekstop main file. We only show the main differences here.

For web programs we use the `tinia::trell::IPCGLJobController` as our controller.

First you need to include the `tinia/trell/IPCGLJobController.hpp` header file instead of the QTController header file:
\snippet tutorial1_web.cpp headers

Then we specify the controller:
\snippet tutorial1_web.cpp controller

The whole main file is then
\include tutorial1_web.cpp