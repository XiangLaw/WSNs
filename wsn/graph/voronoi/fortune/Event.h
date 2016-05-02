#pragma once

#include <wsn/graph/voronoi/VoronoiSite.h>
#include <wsn/graph/voronoi/fortune/Arc.h>
#include <wsn/geometry/Circle.h>

namespace voronoi {
    namespace fortune {

        class SiteEvent;

        class CircleEvent;

        class Event {
        public:
            virtual ~Event();

            virtual bool isSiteEvent() const;

            virtual bool isCircleEvent() const;

            SiteEvent *asSiteEvent() const;

            CircleEvent *asCircleEvent() const;

            virtual geometry::Point position() const = 0;

            bool operator<(const Event &event) const;
        };

        class SiteEvent : public Event {
        public:
            SiteEvent(VoronoiSite *site);

            virtual bool isSiteEvent() const;

            virtual geometry::Point position() const;

            VoronoiSite *site() const;

        private:
            VoronoiSite *_site;
        };

        class CircleEvent : public Event {
        public:
            CircleEvent(Arc *arc, geometry::Circle circle);

            virtual bool isCircleEvent() const;

            virtual geometry::Point position() const;

            bool isValid() const;

            void invalidate();

            Arc *arc() const;

            const geometry::Circle &circle() const;

        private:
            bool valid;
            Arc *_arc;
            geometry::Circle _circle;
        };

        class EventComparator {
        public:
            bool operator()(Event *event1, Event *event2);
        };

    } //end namespace fortune
} //end namespace voronoi

