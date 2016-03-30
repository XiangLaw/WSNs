#pragma once

#include <vector>
#include <queue>

#include <wsn/graph/voronoi/VoronoiDiagram.h>
#include <wsn/graph/voronoi/fortune/Event.h>
#include <wsn/graph/voronoi/fortune/BeachLine.h>

namespace voronoi {
    namespace fortune {

        class Fortune {
        public:
            Fortune();

            void operator()(VoronoiDiagram &diagram);

        private:
            VoronoiDiagram *diagram;

            std::priority_queue<Event *, std::vector<Event *>, EventComparator> eventQueue;
            BeachLine beachLine;
            double sweepLineY;

            void calculate();

            void addEventsFor(const std::vector<VoronoiSite *> &sites);

            void addEvent(Event *event);

            Event *nextEvent();

            void processEvent(Event *event);

            void handleSiteEvent(SiteEvent *event);

            void handleCircleEvent(CircleEvent *event);

            void checkForCircleEvent(Arc *arc);
        };

    } //end namespace fortune
} //end namespace voronoi

