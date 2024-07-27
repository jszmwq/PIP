#include <GL/glew.h>
#include "RasterJoin.hpp"
#include "GLHandler.hpp"
#include "forexperiment.h"
#include <QOpenGLVertexArrayObject>
#include <QElapsedTimer>
#include <set>
#include <list>
#include <iostream>
#include <QFile>

RasterJoin::RasterJoin(GLHandler *handler) : GLFunction(handler)
{
}

RasterJoin::~RasterJoin() {}

void RasterJoin::initGL()
{
    // init shaders
    pointsShader.reset(new QOpenGLShaderProgram());

    pointsShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":shaders/points.vert");
    pointsShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":shaders/points.frag");
    pointsShader->link();

    polyShader.reset(new QOpenGLShaderProgram());
    polyShader->addShaderFromSourceFile(QOpenGLShader::Vertex, ":shaders/polygon.vert");
    polyShader->addShaderFromSourceFile(QOpenGLShader::Fragment, ":shaders/polygon.frag");
    polyShader->link();

    if (this->gvao == 0)
    {
        glGenVertexArrays(1, &this->gvao);
    }

    pbuffer = new GLBuffer();
    pbuffer->generate(GL_ARRAY_BUFFER, false);

    polyBuffer = new GLBuffer();
    polyBuffer->generate(GL_ARRAY_BUFFER, false);

    GLint dims;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &dims);
    this->maxRes = std::min(MAX_FBO_SIZE, dims);

    glGenQueries(1, &query);

#ifdef FULL_SUMMARY_GL
    qDebug() << "setup buffers and shaders for raster join";
#endif
}

void RasterJoin::updateBuffers()
{
    resx = maxRes;
    resy = maxRes;
    GLFunction::size = QSize(resx, resy);
    if (this->polyFbo.isNull() || this->polyFbo->size() != GLFunction::size)
    {
        this->polyFbo.clear();
        this->pointsFbo.clear();

        this->polyFbo.reset(new FBOObject(GLFunction::size, FBOObject::NoAttachment, GL_TEXTURE_2D, GL_RGBA32F));
        this->pointsFbo.reset(new FBOObject(GLFunction::size, FBOObject::NoAttachment, GL_TEXTURE_2D, GL_RGBA32F));
    }
}

inline GLuint64 getTime(GLuint query)
{
    int done = 0;
    while (!done)
    {
        glGetQueryObjectiv(query, GL_QUERY_RESULT_AVAILABLE, &done);
    }
    GLuint64 elapsed_time;
    glGetQueryObjectui64v(query, GL_QUERY_RESULT, &elapsed_time);
    return elapsed_time;
}

void RasterJoin::renderPoints()
{
    // 绑定帧缓冲对象
    this->pointsFbo->bind();

    // 设置清除颜色并清除帧缓冲
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 深度测试和混合模式设置
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // 绑定着色器和顶点数组对象
    pointsShader->bind();
    glBindVertexArray(this->gvao);
    // 绑定和配置顶点缓冲
    this->pbuffer->bind();

    pointsShader->setAttributeBuffer(0, GL_FLOAT, 0,2);
    pointsShader->enableAttributeArray(0);

    // 纹理和图像贴图设置
    // OpenGL允许你在着色器中直接修改纹理数据，写入texBuf.texId中返回查询ID
    glActiveTexture(GL_TEXTURE0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, this->polyFbo->texture());
    glBindImageTexture(0, texBuf.texId, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);

    // 绘制命令
    glDrawArrays(GL_POINTS, 0, pointsize);

    // 禁用顶点属性和恢复OpenGL状态
    glDisableVertexAttribArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    // 绑定默认帧缓冲对象
    FBOObject::bindDefault();
}

void RasterJoin::renderPolys()
{   
    // 绑定帧缓冲对象（FBO）
    this->polyFbo->bind();
    // 设置清除颜色并清除帧缓冲
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // 禁用深度测试
    glDisable(GL_DEPTH_TEST);
    // 启用颜色混合，用于实现透明效果
    glEnable(GL_BLEND);
    // 设置混合函数,忽略目标颜色值。
    glBlendFunc(GL_ONE, GL_ZERO);
       
    // 绑定着色器和顶点数组对象（VAO）
    this->polyShader->bind();
    glBindVertexArray(this->gvao);
    // 绑定并设置顶点属性缓冲区
    this->polyBuffer->bind();
    polyShader->setAttributeBuffer(0, GL_FLOAT, 0, 2);
    polyShader->setAttributeBuffer(1, GL_FLOAT, psize*sizeof(float)*2, 1);
    polyShader->enableAttributeArray(0);
    polyShader->enableAttributeArray(1);

    // 执行渲染命令
    glDrawArrays(GL_TRIANGLES, 0, psize);
    // 禁用顶点属性数组并恢复OpenGL设置
    glDisableVertexAttribArray(0);
    glDisableVertexAttribArray(1);
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

void RasterJoin::performJoin()
{
    timer.restart();
    this->setupPolygons();
    passpolygons_time += timer.nsecsElapsed();

    timer.restart();
    this->setupPoints();
    passpoints_time += timer.nsecsElapsed();

    timer.restart();
    this->renderPolys();
    renderpolygons_time += timer.nsecsElapsed();

    timer.restart();
    this->renderPoints();
    renderpoints_time += timer.nsecsElapsed();

    QVector<int> polyID;
    timer.restart();
    polyID = texBuf.getBuffer(pointsize);
    getresult_time += timer.nsecsElapsed();

    //for (uint32_t i=0;i<pointsize;i++) {
    //    std::cout << "The plan ID of point " << i << " is: " << polyID[i] << endl;
    //}

    QFile file("point_id_result.txt");
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << "Can't open the experimentPath" << endl;
    }
    QTextStream out(&file);
    for (uint32_t i = 0; i < pointsize; i++) {
        out <<  polyID[i] << endl;
    }
    file.close();

    texBuf.destroy();
}

QVector<int> RasterJoin::executeFunction()
{
    QElapsedTimer timer;
    timer.start();

    glViewport(0, 0, pointsFbo->width(), pointsFbo->height());

    performJoin();

    return result;
}
