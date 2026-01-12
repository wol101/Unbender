#ifndef MESHVIEWWIDGET_H
#define MESHVIEWWIDGET_H

#include "OpenCVTools.h"

#include <QOpenGLWidget>
#include <QOpenGLFunctions_3_3_Core>
#include <QOpenGLShaderProgram>
#include <QMatrix4x4>
#include <QMouseEvent>
#include <vector>

class MeshViewWidget : public QOpenGLWidget, protected QOpenGLFunctions_3_3_Core
{
    Q_OBJECT

public:
    struct GLMesh {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        int indexCount = 0;
    };

    explicit MeshViewWidget(QWidget* parent = nullptr);

    void setMeshes(const std::vector<OpenCVTools::Mesh>& m);

    void toggleWireframe();


protected:
    // Rendering state
    std::vector<OpenCVTools::Mesh> meshes;
    QOpenGLShaderProgram program;
    bool wireframe = false;
    std::vector<GLMesh> glMeshes;

    // Camera state
    float distance = 5.0f;
    float yaw = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    QPoint lastPos;

    // Scene bounds
    cv::Point3f minB, maxB, center;
    float radius = 1.0f;

    // ----------------------------------------------------------
    // OpenGL initialization
    // ----------------------------------------------------------
    void initializeGL() override;

    // ----------------------------------------------------------
    // Resize
    // ----------------------------------------------------------
    void resizeGL(int w, int h) override;

    // ----------------------------------------------------------
    // Main render
    // ----------------------------------------------------------
    void paintGL() override;

    // ----------------------------------------------------------
    // Mouse interaction
    // ----------------------------------------------------------
    void mousePressEvent(QMouseEvent* e) override;

    void mouseMoveEvent(QMouseEvent* e) override;

    void wheelEvent(QWheelEvent* e) override;

    void mouseDoubleClickEvent(QMouseEvent*) override;

    // ----------------------------------------------------------
    // Scene bounds
    // ----------------------------------------------------------
    void updateBounds();

    void frameScene();

    GLMesh uploadMesh(const OpenCVTools::Mesh& mesh);

};

#endif // MESHVIEWWIDGET_H
