#include "DataHandler.hpp"
#include "forexperiment.h"

#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QString>
#include <QTextStream>
#include <omp.h>
#include <iostream>
DataHandler::DataHandler() {

}

void DataHandler::readPolygons(QString polyPath) {
    QFile fi(polyPath);
    if(!fi.open( QIODevice::ReadOnly | QIODevice::Text )) {
        qDebug() << "Could not open file" << polyPath;
        return;
    }

    QTextStream input(&fi);
    int polysize;

    input >> polysize;
    complexPolys.resize(polysize);

    int currentID = 0;
    int nvertex = 0;
    int totalPolygons = 0;
    QString next;

    while(!input.atEnd()) {
        input >> next;
        if (next == "next") {
            input >> currentID;
            // Start of a new complex polygon
            complexPolys[currentID].push_back(ComplexPolygon());
        }
        else if (next == "sep") {
            // Start of a new polygon within a complex polygon
            input >> nvertex;
            std::vector<Point> polygon;
            Point v;
            while (nvertex--) {
                input >> v.x >> v.y;
                polygon.push_back(v);
            }
            complexPolys[currentID].back().push_back(polygon);
            totalPolygons++;
        }
    }
    this->totalPolygons = totalPolygons;
    fi.close();
}

void DataHandler::getTriangulation(std::vector<float> &verts, std::vector<int> &ids) {
    verts.clear();
    ids.clear();

    int mts = omp_get_max_threads();
    std::vector<std::vector<float>> tverts(mts), tids(mts);

#pragma omp parallel for
    for (int i = 0; i < static_cast<int>(complexPolys.size()); i++) {
        for (int j = 0; j < static_cast<int>(complexPolys[i].size()); j++) {
            int id = omp_get_thread_num();
            ComplexPolygon inputComplexPolygon;
            std::vector<Point> outputTriangles; // Every 3 points is a triangle
            std::vector<Point> boundingPolygon;
            inputComplexPolygon = complexPolys[i][j];
            clip2tri clip2tri;
            clip2tri.triangulate(inputComplexPolygon, outputTriangles, boundingPolygon);

            for (size_t j = 0; j < outputTriangles.size(); j++) {
                double x = double(outputTriangles[j].x);
                double y = double(outputTriangles[j].y);
                tverts[id].push_back(x);
                tverts[id].push_back(y);
                tids[id].push_back(i);
            }
        }
    }
    for(int i = 0;i < mts;i ++) {
        verts.insert(verts.end(),tverts[i].begin(),tverts[i].end());
        ids.insert(ids.end(),tids[i].begin(),tids[i].end());
    }


//    float precision = 0.001;
//    for(size_t i = 0; i < verts.size(); i++) {
//        if (verts[i] - 0 <= precision) {
//            verts[i] = 0.0;
//        } else if (1 - verts[i] <= precision) {
//            verts[i] = 1.0;
//        }
//    }
}

int DataHandler::getNoPolys() {
    return this->totalPolygons;
}

void DataHandler::readPoints(QString pointPath) {
    QFile fi(pointPath);
    if(!fi.open( QIODevice::ReadOnly | QIODevice::Text )) {
        qDebug() << "Could not open file" << pointPath;
        return;
    }
    QTextStream input(&fi);
    float point_x, point_y;
    while(!input.atEnd()) {
        QString line = input.readLine();
        if(line.trimmed().length() == 0) {
            continue;
        }
        QStringList list = line.split(" ");
        point_x = list[0].toFloat();
        point_y = list[1].toFloat();
        point.push_back(point_x);
        point.push_back(point_y);
    }
    fi.close();
}

void DataHandler::initData(QString polyPath, QString pointPath) {
    timer.start();
    this->readPolygons(polyPath);
    this->getTriangulation(verts, ids);
    initpolygons_time = timer.nsecsElapsed();

    timer.restart();
    this->readPoints(pointPath);
    initpoints_time = timer.nsecsElapsed();
}
