#ifndef DATAHANDLER_HPP
#define DATAHANDLER_HPP

#include <QVector>
#include <QPolygonF>
#include <QString>
#include <vector>
#include "clipper/clipper.hpp"
#include "poly2tri/poly2tri.h"
#include "clip2tri/clip2tri.h"

using namespace c2t;

// A complex polygon is a vector of Polygons
typedef std::vector<std::vector<Point>> ComplexPolygon;

class DataHandler
{
public:
    DataHandler();
    ~DataHandler();

    void initData(QString polyPath, QString pointPath);

public:
    vector<vector<ComplexPolygon>> complexPolys;
    vector<float> verts;
    vector<int> ids; // to store the result of trianglulate
    vector<float> point;
    int totalPolygons;

public:
    void readPolygons(QString polyPath);
    void getTriangulation(vector<float> &verts, vector<int> &ids);
    void readPoints(QString pointPath);
    int getNoPolys();
};

#endif // DATAHANDLER_HPP
