#pragma once


#include <wsn/geometry/Point.h>
#include <wsn/geometry/Line.h>

#include <wsn/graph/voronoi/VoronoiSite.h>

namespace voronoi {

    class VoronoiHalfEdge;

    class VoronoiEdge {
    public:
        VoronoiEdge(VoronoiSite *left, VoronoiSite *right);

        ~VoronoiEdge();

        void addPoint(const geometry::Point &point);

        void adjustOrientation(const geometry::Point &awayPoint);

        VoronoiHalfEdge *halfEdgeFor(VoronoiSite *site);

        geometry::Line edge() { return line; }

        VoronoiSite leftSite() { return *left; }

        VoronoiSite rightSite() { return *right; }

    protected:
        VoronoiSite *left;
        VoronoiSite *right;

        geometry::Line line;

        VoronoiHalfEdge *halfEdge1;
        VoronoiHalfEdge *halfEdge2;
    };

    class VoronoiHalfEdge {
    public:
        VoronoiHalfEdge(VoronoiSite *site);

        VoronoiSite *site() const;

        VoronoiHalfEdge *opposite() const;

        VoronoiHalfEdge *next() const;

        VoronoiHalfEdge *prev() const;

        const geometry::Point &startPoint() const;

        bool hasStartPoint() const;

        void setStartPoint(const geometry::Point &point);

        void setOpposite(VoronoiHalfEdge *opposite);

        void setNext(VoronoiHalfEdge *next);

        void setPrev(VoronoiHalfEdge *prev);

        VoronoiHalfEdge *begin();

    protected:
        VoronoiSite *_site;
        VoronoiHalfEdge *_prev;
        VoronoiHalfEdge *_next;
        VoronoiHalfEdge *_opposite;
        bool _hasStart;
        geometry::Point _startPoint;
    };

} //end namespace voronoi

