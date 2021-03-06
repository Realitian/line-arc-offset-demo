#include "GeometryCGAL.h"
#include "../GeometryQt.h"

// #include <CGAL/Boolean_set_operations_2/Gps_polygon_validation.h> // for has_valid_orientation_polygon()

namespace LineArcOffsetDemo {

Polygon_2 ContourToPolygon(const LineArcGeometry::Contour &contour)
{
    Traits_2 traits;
    Polygon_2 result;
    for (std::list<LineArcGeometry::Segment>::const_iterator it = contour.segments.begin(); it != contour.segments.end(); ++it)
    {
        std::list<CGAL::Object> objects;
        if (it->isArc)
        {
            // qDebug() << "cen: " << it->center.x << "," << it->center.y << "; p1: " << it->line.p1.x << "," << it->line.p1.y << "; p2: " << it->line.p2.x << it->line.p2.y;
#if 0
            // NOTE: this method doesn't work in practice because the source
            // and endpoints must be exactly the same distance from the center
            // point, which due to roundoff error isn't the case usually
            const Point_2 cen(it->center.x, it->center.y);
            const Point_2 pt2_kern(it->line.p2.x, it->line.p2.y);
            const Traits_2::Point_2 cen_alt(it->center.x, it->center.y);
            // http://cgal-discuss.949826.n4.nabble.com/Some-quot-Curve-2-quot-constructors-don-t-compile-for-me-td4661665.html#a4661667
            const Traits_2::Point_2 pt1(it->line.p1.x, it->line.p1.y);
            const Traits_2::Point_2 pt2(it->line.p2.x, it->line.p2.y);
            const Kernel::Orientation orientation = (it->orientation == LineArcGeometry::Segment::Clockwise) ? CGAL::CLOCKWISE : CGAL::COUNTERCLOCKWISE;
            const Kernel::FT radius_squared = CGAL::squared_distance(cen, pt2_kern);
            // const Kernel::FT radius_squared = CGAL::squared_distance(cen_alt, pt2);
            const Circle_2 circle(cen, radius_squared, orientation);
            Curve_2 curve(circle, pt1, pt2);
#else
            const LineArcGeometry::Point midPoint = it->midPoint();
            const Point_2 pt1(it->line.p1.x, it->line.p1.y);
            const Point_2 pt2(it->line.p2.x, it->line.p2.y);
            const Point_2 pt3(midPoint.x, midPoint.y);
            Curve_2 curve(pt1, pt3, pt2);
#endif
            traits.make_x_monotone_2_object() (curve, std::back_inserter(objects));
        }
        else // it's a line
        {
            // NOTE: CGAL doesn't like zero-length lines!
            if (it->line.p1 != it->line.p2)
            {
                Point_2 pt1(it->line.p1.x, it->line.p1.y);
                Point_2 pt2(it->line.p2.x, it->line.p2.y);
                Curve_2 curve(pt1, pt2);
                traits.make_x_monotone_2_object() (curve, std::back_inserter(objects));
            }
        }
        // Construct the polygon.
        X_monotone_curve_2 arc;
        std::list<CGAL::Object>::iterator iter;
        for (iter = objects.begin(); iter != objects.end(); ++iter)
        {
            CGAL::assign(arc, *iter);
            result.push_back(arc);
        }
    }
    // if (!has_valid_orientation_polygon(result, traits))
    // if (result.orientation() == CGAL::CLOCKWISE)
    if (result.orientation() == CGAL::NEGATIVE)
    {
        result.reverse_orientation();
    }
    return result;
}

Polygon_with_holes_2 ShapeToPolygonWithHoles(const LineArcGeometry::Shape &shape)
{
    Polygon_with_holes_2 result(ContourToPolygon(shape.boundary));
    for (std::list<LineArcGeometry::Contour>::const_iterator it = shape.holes.begin(); it != shape.holes.end(); ++it)
    {
        std::list<Polygon_with_holes_2> res;
        Polygon_2 hole(ContourToPolygon(*it));
        CGAL::difference(result, hole, std::back_inserter(res));
        result = res.front();
    }
    return result;
}

std::list<Polygon_with_holes_2> MultiShapeToPolygonWithHolesList(const LineArcGeometry::MultiShape &multiShape)
{
    std::list<Polygon_with_holes_2> result;
    for (std::list<LineArcGeometry::Shape>::const_iterator it = multiShape.shapes.begin(); it != multiShape.shapes.end(); ++it)
    {
        Polygon_with_holes_2 polygon = ShapeToPolygonWithHoles(*it);
        result.push_back(polygon);
    }
    return result;
}

LineArcGeometry::Contour PolygonToContour(const Polygon_2 &polygon)
{
    LineArcGeometry::Contour result;
    for (auto curve_it = polygon.curves_begin(); curve_it != polygon.curves_end(); ++curve_it)
    {
        auto curve = *curve_it;
        if (curve.is_circular())
        {
            auto circle = curve.supporting_circle();
            // const double radius = sqrt(CGAL::to_double(circle.squared_radius()));
            const LineArcGeometry::Point circleCenter(CGAL::to_double(circle.center().x()), CGAL::to_double(circle.center().y()));
            // const QRectF  circleRect(circleCenter.x() - radius, circleCenter.y() - radius, radius*2.0, radius*2.0);
            const LineArcGeometry::Segment::Orientation orientation = (curve.orientation() == CGAL::CLOCKWISE) ? LineArcGeometry::Segment::Clockwise : LineArcGeometry::Segment::CounterClockwise;
            // if (curve.orientation() != CGAL::CLOCKWISE)
            // {
                // qSwap(startPoint, endPoint);
            // }
            QPointF  startPoint(CGAL::to_double(curve.source().x()), CGAL::to_double(curve.source().y()));
            QPointF    endPoint(CGAL::to_double(curve.target().x()), CGAL::to_double(curve.target().y()));
            const double lineLength = QLineF(startPoint, endPoint).length();
            if (lineLength < 0.0001) // HACK, this is to fix some sort of rounding errors with .angle()...
            {
                const LineArcGeometry::Point p1(CGAL::to_double(curve.source().x()), CGAL::to_double(curve.source().y()));
                const LineArcGeometry::Point p2(CGAL::to_double(curve.target().x()), CGAL::to_double(curve.target().y()));
                result.segments.push_back(LineArcGeometry::Line(p1, p2));
            }
            else
            {
                const LineArcGeometry::Point p1(CGAL::to_double(curve.source().x()), CGAL::to_double(curve.source().y()));
                const LineArcGeometry::Point p2(CGAL::to_double(curve.target().x()), CGAL::to_double(curve.target().y()));
                result.segments.push_back(LineArcGeometry::Segment(LineArcGeometry::Line(p1, p2), circleCenter, orientation));
            }
        }
        else
        {
            const LineArcGeometry::Point p1(CGAL::to_double(curve.source().x()), CGAL::to_double(curve.source().y()));
            const LineArcGeometry::Point p2(CGAL::to_double(curve.target().x()), CGAL::to_double(curve.target().y()));
            result.segments.push_back(LineArcGeometry::Line(p1, p2));
        }
    }
    if (!result.isValid())
    {
        qDebug() << "ERROR: generated invalid Contour from Polygon_2!" << result;
    }
    return result;
}

LineArcGeometry::Shape PolygonWithHolesToShape(const Polygon_with_holes_2 &polygon)
{
    LineArcGeometry::Shape result(PolygonToContour(polygon.outer_boundary()));
    for (Polygon_with_holes_2::Hole_const_iterator hole_it = polygon.holes_begin(); hole_it != polygon.holes_end(); ++hole_it)
    {
        result.holes.push_back(PolygonToContour(*hole_it));
    }
    return result;
}

LineArcGeometry::MultiShape PolygonWithHolesListToMultiShape(const std::list<Polygon_with_holes_2> &polygons)
{
    LineArcGeometry::MultiShape result;
    for (std::list<Polygon_with_holes_2>::const_iterator it = polygons.begin(); it != polygons.end(); ++it)
    {
        result.shapes.push_back(PolygonWithHolesToShape(*it));
    }
    return result;
}

} // namespace LineArcOffsetDemo
