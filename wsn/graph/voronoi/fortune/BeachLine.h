#pragma once

#include <wsn/graph/voronoi/VoronoiEdge.h>
#include <wsn/graph/voronoi/fortune/Arc.h>

namespace voronoi {
    namespace fortune {

        class BeachLine {
        public:
            BeachLine();

            Arc *createArc(VoronoiSite *site);

            bool isEmpty() const;

            void insert(Arc *arc);

            void insertAfter(Arc *newArc, Arc *after);

            void splitArcWith(Arc *arc, Arc *newArc);

            Arc *arcAbove(const geometry::Point &point) const;

            void replaceArc(Arc *arc, VoronoiEdge *edge);

            Arc *lastElement() const;

        private:
            Arc *_firstElement;
            Arc *_lastElement;

            void linkArcs(Arc *arc1, Arc *arc2);

            void unlinkArc(Arc *arc);
        };

    } //end namespace fortune
} //end namespace voronoi

