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
    // ��֡�������
    this->pointsFbo->bind();

    // ���������ɫ�����֡����
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // ��Ȳ��Ժͻ��ģʽ����
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    // ����ɫ���Ͷ����������
    pointsShader->bind();
    glBindVertexArray(this->gvao);
    // �󶨺����ö��㻺��
    this->pbuffer->bind();

    pointsShader->setAttributeBuffer(0, GL_FLOAT, 0,2);
    pointsShader->enableAttributeArray(0);

    // ������ͼ����ͼ����
    // OpenGL����������ɫ����ֱ���޸��������ݣ�д��texBuf.texId�з��ز�ѯID
    glActiveTexture(GL_TEXTURE0);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindTexture(GL_TEXTURE_2D, this->polyFbo->texture());
    glBindImageTexture(0, texBuf.texId, 0, GL_FALSE, 0, GL_READ_WRITE, GL_R32I);

    // ��������
    glDrawArrays(GL_POINTS, 0, pointsize);

    // ���ö������Ժͻָ�OpenGL״̬
    glDisableVertexAttribArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    // ��Ĭ��֡�������
    FBOObject::bindDefault();
}

void RasterJoin::renderPolys()
{   
    // ��֡�������FBO��
    this->polyFbo->bind();
    // ���������ɫ�����֡����
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    // ������Ȳ���
    glDisable(GL_DEPTH_TEST);
    // ������ɫ��ϣ�����ʵ��͸��Ч��
    glEnable(GL_BLEND);
    // ���û�Ϻ���,����Ŀ����ɫֵ��
    glBlendFunc(GL_ONE, GL_ZERO);
       
    // ����ɫ���Ͷ����������VAO��
    this->polyShader->bind();
    glBindVertexArray(this->gvao);
    // �󶨲����ö������Ի�����
    this->polyBuffer->bind();
    polyShader->setAttributeBuffer(0, GL_FLOAT, 0, 2);
    polyShader->setAttributeBuffer(1, GL_FLOAT, psize*sizeof(float)*2, 1);
    polyShader->enableAttributeArray(0);
    polyShader->enableAttributeArray(1);

    // ִ����Ⱦ����
    glDrawArrays(GL_TRIANGLES, 0, psize);
    // ���ö����������鲢�ָ�OpenGL����
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