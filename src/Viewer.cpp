#include "Viewer.h"
#include <algorithm>
#include <nanogui/slider.h>


Viewer::Viewer() : nanogui::Screen(Eigen::Vector2i(1024, 768), "CSC 5870 Assignment 1") {

    m_window = new Window(this, "Controls");
    m_window->setPosition(Vector2i(15, 15));
    m_window->setLayout(new GroupLayout());
    m_sliderLabel = new Label(m_window, "0.0");
    m_sliderLabel2 = new Label(m_window, "0.0");
    m_sliderLabel3 = new Label(m_window, "0.0");
    auto *xSlider = new Slider(m_window);
    auto *ySlider = new Slider(m_window);
    auto *zSlider = new Slider(m_window);


    auto *b = new Button(m_window, "Open obj file ");
    b->setCallback([this]() {
        std::string filename = nanogui::file_dialog({{"obj", "Wavefront OBJ"}}, false);

        if (!filename.empty()) {
            mProcessEvents = false;
            m_mesh = new Mesh(filename);
            this->refresh_mesh();
            this->refresh_trackball_center();
            mProcessEvents = true;
        }
    });

    auto *wireframeButton = new Button(m_window, "Wireframe");
    wireframeButton->setCallback([this]() {
        m_wireframe = !m_wireframe;
    });

    auto *horizontal = new Button(m_window, "Horizontal view of the camera ");
    horizontal->setCallback([this]() {
        m_camera.setHorizontalFOV(60.0f);

    });

    auto *vertical = new Button(m_window, "Vertical view of the camera ");
    vertical->setCallback([this]() {
        m_camera.setVerticalFOV(45.0f);
    });

    auto *resetViewButton = new Button(m_window, "Reset View");
    resetViewButton->setCallback([&]() {
        resetCamera();
    });

    xSlider->setRange(std::make_pair(-15.0f, 15.0f)); //change value here for slider
    xSlider->setValue(0.0f);
    xSlider->setCallback([this](float value) {
        m_camera.modelTranslation.x() = value;
        m_sliderLabel->setCaption("X Translation :" + std::to_string(value));
    });

    ySlider->setRange(std::make_pair(-15.0f, 15.0f)); //change value here for slider
    ySlider->setValue(0.0f);
    ySlider->setCallback([this](float value) {
        m_camera.modelTranslation.y() = value;
        m_sliderLabel2->setCaption("Y Translation :" + std::to_string(value));
    });

    zSlider->setRange(std::make_pair(-15.0f, 15.0f)); //change value here for slider
    zSlider->setValue(0.0f);
    zSlider->setCallback([this](float value) {
        m_camera.modelTranslation.z() = value;
        m_sliderLabel3->setCaption("Z Translation :" + std::to_string(value));
    });


    auto nearSlider = new Slider(m_window);
    nearSlider->setFixedWidth(100);
    nearSlider->setValue(0.5f);
    auto nearLabel = new Label(m_window, "Near Clipping :" + std::to_string(nearSlider->value()), "sans-bold", 15);
    nearSlider->setCallback([this, nearLabel](float value) {
        m_camera.dnear = 0.1f + value * 9.9f;
        nearLabel->setCaption("Near Clipping :" + std::to_string(value));
    });

    auto farSlider = new Slider(m_window);
    farSlider->setFixedWidth(100);
    farSlider->setValue(0.5f);
    auto farLabel = new Label(m_window, "Far Clipping :" + std::to_string(farSlider->value()), "sans-bold", 15);
    farSlider->setCallback([this, farLabel](float value) {
        m_camera.dfar = 10.0f + value * 90.0f;
        farLabel->setCaption("Far Clipping :" + std::to_string(value));
    });


    performLayout();
    initShaders();

    m_mesh = new Mesh("cow.obj");
    this->refresh_mesh();
    this->refresh_trackball_center();
}

bool Viewer::keyboardEvent(int key, int scancode, int action, int modifiers) {
    if (Screen::keyboardEvent(key, scancode, action, modifiers)) {
        return true;
    }
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        setVisible(false);
        return true;
    }
    return false;
}

void Viewer::draw(NVGcontext *ctx) {
    /* Draw the user interface */
    Screen::draw(ctx);
}


void Viewer::drawContents() {

    using namespace nanogui;

    if (m_wireframe) {
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }
    else {
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }


    if (m_mesh == nullptr)
        return;

    /* Draw the window contents using OpenGL */
    m_basic_shader.bind();

    Eigen::Matrix4f model, view, proj;
    computeCameraMatrices(model, view, proj);

    Matrix4f mv = view * model;
    Matrix4f p = proj;

    /* MVP uniforms */
    m_basic_shader.setUniform("MV", mv);
    m_basic_shader.setUniform("P", p);

    // Setup OpenGL (making sure the GUI doesn't disable these
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    m_basic_shader.drawIndexed(GL_TRIANGLES, 0, m_mesh->get_number_of_face());
}

bool Viewer::scrollEvent(const Vector2i &p, const Vector2f &rel) {
    if (!Screen::scrollEvent(p, rel)) {
        m_camera.zoom = std::max(0.1, m_camera.zoom * (rel.y() > 0 ? 1.1 : 0.9));
    }
    return true;
}

bool Viewer::mouseMotionEvent(const Vector2i &p, const Vector2i &rel,
                              int button, int modifiers) {
    if (!Screen::mouseMotionEvent(p, rel, button, modifiers)) {
        if (m_camera.arcball.motion(p)) {
            //
        } else if (m_translate) {
            Eigen::Matrix4f model, view, proj;
            computeCameraMatrices(model, view, proj);
            Eigen::Vector3f mesh_center = m_mesh->get_mesh_center();
            float zval = nanogui::project(Vector3f(mesh_center.x(),
                                                   mesh_center.y(),
                                                   mesh_center.z()),
                                          view * model, proj, mSize).z();
            Eigen::Vector3f pos1 = nanogui::unproject(
                    Eigen::Vector3f(p.x(), mSize.y() - p.y(), zval),
                    view * model, proj, mSize);
            Eigen::Vector3f pos0 = nanogui::unproject(
                    Eigen::Vector3f(m_translateStart.x(), mSize.y() -
                                                          m_translateStart.y(), zval), view * model, proj, mSize);
            m_camera.modelTranslation = m_camera.modelTranslation_start + (pos1 - pos0);
        }
    }
    return true;
}

void Viewer::resetCamera() {
    m_camera.arcball = Arcball();
    m_camera.zoom = 1.0f;
    m_camera.modelZoom = .5f;
    m_camera.modelTranslation = Eigen::Vector3f::Zero();
    m_camera.modelTranslation_start = Eigen::Vector3f::Zero();
    m_camera.setHorizontalFOV(45.0f);
    m_camera.arcball.setSize(mSize);

    // Reset translation
    m_camera.modelTranslation.x() = 0;
    m_camera.modelTranslation.y() = 0;
    m_camera.modelTranslation.z() = 0;

    // Reset clipping values to default
    m_camera.dnear = 0.1f;
    m_camera.dfar = 100.0f;
}

bool Viewer::mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) {
    if (!Screen::mouseButtonEvent(p, button, down, modifiers)) {
        if (button == GLFW_MOUSE_BUTTON_1 && modifiers == 0) {
            m_camera.arcball.button(p, down);
        } else if (button == GLFW_MOUSE_BUTTON_2 ||
                   (button == GLFW_MOUSE_BUTTON_1 && modifiers == GLFW_MOD_SHIFT)) {
            m_camera.modelTranslation_start = m_camera.modelTranslation;
            m_translate = true;
            m_translateStart = p;
        }
    }
    if (button == GLFW_MOUSE_BUTTON_1 && !down) {
        m_camera.arcball.button(p, false);
    }
    if (!down) {
        m_translate = false;
    }
    return true;
}

void Viewer::initShaders() {
    // Shaders
    m_basic_shader.init(
            "a_basic_shader",

            /* Vertex shader */
            "#version 330\n"
            "uniform mat4 MV;\n"
            "uniform mat4 P;\n"

            "in vec3 position;\n"
            "in vec3 normal;\n"

            "out vec3 fcolor;\n"

            "void main() {\n"
            "    gl_Position = P * MV * vec4(position, 1.0);\n"
            "    fcolor = vec3(0.5, 0.5, 0.5);\n"
            "}",

            /* Fragment shader */
            "#version 330\n"

            "in vec3 fcolor;\n"

            "out vec4 color;\n"

            "void main() {\n"
            "    color = vec4(fcolor, 1.0);\n"
            "}"
    );
}


void Viewer::refresh_trackball_center() {
    // Re-center the mesh
    Eigen::Vector3f mesh_center = m_mesh->get_mesh_center();
    m_camera.arcball = Arcball();
    m_camera.arcball.setSize(mSize);
    m_camera.modelZoom = 2 / m_mesh->get_dist_max();
    m_camera.modelTranslation = -Vector3f(mesh_center.x(), mesh_center.y(), mesh_center.z());
}

void Viewer::refresh_mesh() {
    m_basic_shader.bind();
    m_basic_shader.uploadIndices(*(m_mesh->get_indices()));
    m_basic_shader.uploadAttrib("position", *(m_mesh->get_points()));
    m_basic_shader.uploadAttrib("normal", *(m_mesh->get_normals()));
}

void Viewer::computeCameraMatrices(Eigen::Matrix4f &model,
                                   Eigen::Matrix4f &view,
                                   Eigen::Matrix4f &proj) {

    view = nanogui::lookAt(m_camera.eye, m_camera.center, m_camera.up);

    float fH = std::tan(m_camera.viewAngle / 360.0f * M_PI) * m_camera.dnear;
    float fW = fH * (float) mSize.x() / (float) mSize.y();

    proj = nanogui::frustum(-fW, fW, -fH, fH, m_camera.dnear, m_camera.dfar);
    model = m_camera.arcball.matrix();

    // model = nanogui::scale(model, CEigen::Vector3f::Constant(m_camera.zoom * m_camera.modelZoom));
    // model = nanogui::translate(model, m_camera.modelTranslation);

    model = model * nanogui::scale(Eigen::Vector3f::Constant(m_camera.zoom * m_camera.modelZoom));
    model = model * nanogui::translate(m_camera.modelTranslation);
}

Viewer::~Viewer() {
    m_basic_shader.free();
    delete m_mesh;
}