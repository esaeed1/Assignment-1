#ifndef TINYOBJVIEWER_VIEWER_H
#define TINYOBJVIEWER_VIEWER_H

#include <nanogui/opengl.h>
#include <nanogui/glutil.h>
#include <nanogui/screen.h>
#include <nanogui/window.h>
#include <nanogui/layout.h>
#include <nanogui/popupbutton.h>
#include <nanogui/label.h>
#include <nanogui/button.h>
#include <nanogui/checkbox.h>
#include <nanogui/textbox.h>
#include <nanogui/tabwidget.h>
#include <nanogui/combobox.h>

#include "Mesh.h"

using namespace nanogui;

class Viewer : public nanogui::Screen {
public:

    Viewer();

    ~Viewer() override;

    void draw(NVGcontext *ctx) override;

    void drawContents() override;

    void refresh_mesh();

    void refresh_trackball_center();

    bool keyboardEvent(int key, int scancode, int action, int modifiers) override;

    bool scrollEvent(const Vector2i &p, const Vector2f &rel) override;

    bool mouseMotionEvent(const Vector2i &p, const Vector2i &rel, int button, int modifiers) override;

    bool mouseButtonEvent(const Vector2i &p, int button, bool down, int modifiers) override;

private:
    void initShaders();

    void resetCamera();

    void computeCameraMatrices(Eigen::Matrix4f &model,
                               Eigen::Matrix4f &view,
                               Eigen::Matrix4f &proj);

    bool m_wireframe = false; // add this line


    struct CameraParameters {
        nanogui::Arcball arcball;
        float zoom = 1.0f, viewAngle = 45.0f;
        float dnear = 0.05f, dfar = 100.0f;
        Eigen::Vector3f eye = Eigen::Vector3f(0.0f, 0.0f, 5.0f);
        Eigen::Vector3f center = Eigen::Vector3f(0.0f, 0.0f, 0.0f);
        Eigen::Vector3f up = Eigen::Vector3f(0.0f, 1.0f, 0.0f);
        Eigen::Vector3f modelTranslation = Eigen::Vector3f::Zero();
        Eigen::Vector3f modelTranslation_start = Eigen::Vector3f::Zero();
        float modelZoom = 1.0f;


        void setHorizontalFOV(float angle) {
            viewAngle = angle;
        }

        // calculate the new zoom value based on the new angle and the original aspect ratio
        void setVerticalFOV(float angle) {
            float newZoom = 1.0f / std::tan(angle * 0.5f * (float) M_PI / 180.0f);
            zoom *= newZoom / zoom;
            viewAngle = angle;
        }

    };

    CameraParameters m_camera;
    bool m_translate = false;
    Vector2i m_translateStart = Vector2i(0, 0);

    // Variables for the viewer
    nanogui::GLShader m_basic_shader;
    nanogui::Window *m_window;
    Label *m_sliderLabel;
    Label *m_sliderLabel2;
    Label *m_sliderLabel3;
    Mesh *m_mesh;

};

#endif //TINYOBJVIEWER_VIEWER_H
