#include "MeshViewWidget.h"

MeshViewWidget::MeshViewWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
}

void MeshViewWidget::setMeshes(const std::vector<OpenCVTools::Mesh>& m)
{
    meshes = m;
    glMeshes.clear();

    makeCurrent();
    for (const auto& mesh : meshes)
        glMeshes.push_back(uploadMesh(mesh));
    doneCurrent();

    updateBounds();
    frameScene();
    update();
}

void MeshViewWidget::toggleWireframe()
{
    wireframe = !wireframe;
    update();
}

// ----------------------------------------------------------
// OpenGL initialization
// ----------------------------------------------------------
void MeshViewWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // Simple Phong shader
    program.addShaderFromSourceCode(QOpenGLShader::Vertex,
                                    R"(
                                    attribute vec3 aPos;
                                    attribute vec3 aNormal;

                                    uniform mat4 uMVP;
                                    uniform mat4 uModel;
                                    uniform mat3 uNormalMatrix;

                                    varying vec3 vNormal;
                                    varying vec3 vPos;

                                    void main() {
                                        vNormal = normalize(uNormalMatrix * aNormal);
                                        vPos = vec3(uModel * vec4(aPos,1.0));
                                        gl_Position = uMVP * vec4(aPos,1.0);
                                    }
                                    )"
                                    );

    program.addShaderFromSourceCode(QOpenGLShader::Fragment,
                                    R"(
                                    varying vec3 vNormal;
                                    varying vec3 vPos;

                                    uniform vec3 uLightPos;
                                    uniform vec3 uColor;

                                    void main() {
                                        vec3 N = normalize(vNormal);
                                        vec3 L = normalize(uLightPos - vPos);
                                        float diff = max(dot(N, L), 0.0);
                                        vec3 color = uColor * (0.2 + 0.8 * diff);
                                        gl_FragColor = vec4(color, 1.0);
                                    }
                                    )"
                                    );

    program.link();
}

// ----------------------------------------------------------
// Resize
// ----------------------------------------------------------
void MeshViewWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);
}

// ----------------------------------------------------------
// Main render
// ----------------------------------------------------------
void MeshViewWidget::paintGL()
{
    glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (meshes.empty())
        return;

    program.bind();

    QMatrix4x4 proj;
    float aspect = float(width())/float(height());
    float nearPlane = 0.1f;
    float farPlane = 10000.0f;
    // float fovY = 45.0f / zoomFactor;
    // proj.perspective(fovY, aspect, nearPlane, farPlane);
    float halfH = height() * 0.5f * zoomFactor;
    float halfW = halfH * aspect;
    float left   = -halfW;
    float right  =  halfW;
    float bottom = -halfH;
    float top    =  halfH;
    proj.ortho(left, right, bottom, top, nearPlane, farPlane);

    QMatrix4x4 view;
    view.translate(0,0,-distance);
    view.rotate(pitch, 1,0,0);
    view.rotate(yaw,   0,1,0);
    view.rotate(roll,  0,0,1);
    view.translate(-center.x, -center.y, -center.z);

    QMatrix4x4 model;
    QMatrix4x4 mvp = proj * view * model;

    program.setUniformValue("uMVP", mvp);
    program.setUniformValue("uModel", model);
    program.setUniformValue("uNormalMatrix", model.normalMatrix());
    program.setUniformValue("uLightPos", QVector3D(center.x, center.y + radius, center.z + radius));
    program.setUniformValue("uColor", QVector3D(0.8f, 0.8f, 0.9f));

    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    for (const auto& gm : glMeshes)
    {
        glBindVertexArray(gm.vao);
        glDrawElements(GL_TRIANGLES, gm.indexCount, GL_UNSIGNED_INT, 0);
    }
    glBindVertexArray(0);


    if (wireframe)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    program.release();
}

// ----------------------------------------------------------
// Mouse interaction
// ----------------------------------------------------------
void MeshViewWidget::mousePressEvent(QMouseEvent* e)
{
    lastPos = e->pos();
}

void MeshViewWidget::mouseMoveEvent(QMouseEvent* e)
{
    QPoint delta = e->pos() - lastPos;
    lastPos = e->pos();

    if (e->buttons() & Qt::LeftButton)
    {
        if (e->modifiers() & Qt::ShiftModifier)
            roll += delta.x() * 0.3f;
        else {
            yaw   += delta.x() * 0.3f;
            pitch += delta.y() * 0.3f;
        }
    }
    else if (e->buttons() & Qt::MiddleButton)
    {
        float panSpeed = radius * 0.002f;
        center.x -= delta.x() * panSpeed;
        center.y += delta.y() * panSpeed;
    }

    update();
}

void MeshViewWidget::wheelEvent(QWheelEvent* e)
{
    zoomFactor *= std::clamp(1.0f - e->angleDelta().y() * 0.001f, 0.5f, 2.0f);
    zoomFactor = std::clamp(zoomFactor, 0.01f, 100.0f);
    update();
}

void MeshViewWidget::mouseDoubleClickEvent(QMouseEvent*)
{
    frameScene();
    update();
}

// ----------------------------------------------------------
// Scene bounds
// ----------------------------------------------------------
void MeshViewWidget::updateBounds()
{
    if (meshes.empty()) {
        minB = maxB = cv::Point3f(0,0,0);
        return;
    }

    bool first = true;
    for (const auto& mesh : meshes)
    {
        for (const auto& v : mesh.vertices)
        {
            if (first) {
                minB = maxB = v;
                first = false;
            } else {
                minB.x = std::min(minB.x, v.x);
                minB.y = std::min(minB.y, v.y);
                minB.z = std::min(minB.z, v.z);
                maxB.x = std::max(maxB.x, v.x);
                maxB.y = std::max(maxB.y, v.y);
                maxB.z = std::max(maxB.z, v.z);
            }
        }
    }

    center = 0.5f * (minB + maxB);
    radius = std::sqrt(
                 (maxB.x - minB.x)*(maxB.x - minB.x) +
                 (maxB.y - minB.y)*(maxB.y - minB.y) +
                 (maxB.z - minB.z)*(maxB.z - minB.z)
                 ) * 0.5f;
}

void MeshViewWidget::frameScene()
{
    center = 0.5f * (minB + maxB);
    distance = radius * 2.5f;
    yaw = pitch = roll = 0.0f;
}

MeshViewWidget::GLMesh MeshViewWidget::uploadMesh(const OpenCVTools::Mesh& mesh)
{
    GLMesh gl;

    glGenVertexArrays(1, &gl.vao);
    glGenBuffers(1, &gl.vbo);
    glGenBuffers(1, &gl.ebo);

    glBindVertexArray(gl.vao);

    // Interleave positions + normals
    struct Vertex { float px, py, pz, nx, ny, nz; };
    std::vector<Vertex> verts;
    verts.reserve(mesh.vertices.size());

    for (size_t i = 0; i < mesh.vertices.size(); ++i)
    {
        const auto& p = mesh.vertices[i];
        const auto& n = mesh.normals[i];
        verts.push_back({p.x, p.y, p.z, n.x, n.y, n.z});
    }

    glBindBuffer(GL_ARRAY_BUFFER, gl.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(Vertex), verts.data(), GL_STATIC_DRAW);

    // Index buffer
    std::vector<unsigned int> indices;
    indices.reserve(mesh.triangles.size() * 3);
    for (auto& t : mesh.triangles) {
        indices.push_back(t[0]);
        indices.push_back(t[1]);
        indices.push_back(t[2]);
    }

    gl.indexCount = indices.size();

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Vertex attributes
    GLint posLoc = program.attributeLocation("aPos");
    GLint normLoc = program.attributeLocation("aNormal");

    glEnableVertexAttribArray(posLoc);
    glVertexAttribPointer(posLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);

    glEnableVertexAttribArray(normLoc);
    glVertexAttribPointer(normLoc, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);

    return gl;
}
