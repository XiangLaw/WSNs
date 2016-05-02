#pragma once

#include <wsn/geometry/Point.h>

namespace voronoi {

    class VoronoiSite {
    public:
        VoronoiSite();

        VoronoiSite(const geometry::Point &position);

        VoronoiSite(double x, double y);

        VoronoiSite(int id, double x, double y);

        const geometry::Point &position() const;

        const int &id() const { return _id; };

    protected:
        geometry::Point _position;
        int _id;
    };

} //end namespace voronoi

