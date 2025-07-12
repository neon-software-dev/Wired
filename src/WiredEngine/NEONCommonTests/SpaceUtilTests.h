/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMONTESTS_SPACEUTILTESTS_H
#define WIREDENGINE_NEONCOMMONTESTS_SPACEUTILTESTS_H

#include <gtest/gtest.h>

#include <NEON/Common/Space/SpaceUtil.h>

namespace NCommon
{
    struct Test2DPoint : public NCommon::Point2DReal {
        using NCommon::Point2DReal::Point2DReal;
    };
    struct Test3DPoint : public NCommon::Point3DReal {
        using NCommon::Point3DReal::Point3DReal;
    };
    struct TestSurface : public NCommon::Surface {
        using NCommon::Surface::Surface;
    };

    TEST(SpaceUtilTests, Map2DPointToSurfaceCenterOrigin1)
    {
        Test2DPoint p(0, 0);
        TestSurface s(1000, 1000);

        const auto result = Map2DPointToSurfaceSpaceCenterOrigin(p, s);

        EXPECT_FLOAT_EQ(result.x, 500.0f);
        EXPECT_FLOAT_EQ(result.y, 500.0f);
    }

    TEST(SpaceUtilTests, Map2DPointToSurfaceCenterOrigin2)
    {
        Test2DPoint p(-500, 0);
        TestSurface s(1000, 1000);

        const auto result = Map2DPointToSurfaceSpaceCenterOrigin(p, s);

        EXPECT_FLOAT_EQ(result.x, 0.0f);
        EXPECT_FLOAT_EQ(result.y, 500.0f);
    }

    TEST(SpaceUtilTests, Map2DPointToSurfaceCenterOrigin3)
    {
        Test2DPoint p(-500, 500);
        TestSurface s(1000, 1000);

        const auto result = Map2DPointToSurfaceSpaceCenterOrigin(p, s);

        EXPECT_FLOAT_EQ(result.x, 0.0f);
        EXPECT_FLOAT_EQ(result.y, 0.0f);
    }

    TEST(SpaceUtilTests, Map2DPointToSurfaceCenterOrigin4)
    {
        Test2DPoint p(500, -500);
        TestSurface s(1000, 1000);

        const auto result = Map2DPointToSurfaceSpaceCenterOrigin(p, s);

        EXPECT_FLOAT_EQ(result.x, 1000.0f);
        EXPECT_FLOAT_EQ(result.y, 1000.0f);
    }

    TEST(SpaceUtilTests, Map2DPointToSurfaceCenterOrigin5)
    {
        Test2DPoint p(-1000, 1000);
        TestSurface s(1000, 1000);

        const auto result = Map2DPointToSurfaceSpaceCenterOrigin(p, s);

        EXPECT_FLOAT_EQ(result.x, -500.0f);
        EXPECT_FLOAT_EQ(result.y, -500.0f);
    }

    TEST(SpaceUtilTests, MapSizeBetweenSurfaces1)
    {
        Size2DUInt s(100, 100);
        TestSurface s0(500, 500);
        TestSurface s1(1000, 1000);

        const auto result = MapSizeBetweenSurfaces<Size2DUInt, Size2DReal>(s, s0, s1);

        EXPECT_FLOAT_EQ(result.w, 200.0f);
        EXPECT_FLOAT_EQ(result.h, 200.0f);
    }

    TEST(SpaceUtilTests, MapSizeBetweenSurfaces2)
    {
        Size2DReal s(100.25f, 100.25f);
        TestSurface s0(500, 500);
        TestSurface s1(1000, 1000);

        const auto result = MapSizeBetweenSurfaces<Size2DReal, Size2DReal>(s, s0, s1);

        EXPECT_FLOAT_EQ(result.w, 200.50f);
        EXPECT_FLOAT_EQ(result.h, 200.50f);
    }

    TEST(SpaceUtilTests, Map2DPointBetweenSurfaces1)
    {
        Test2DPoint p(100, 100);
        TestSurface s0(500, 500);
        TestSurface s1(1000, 1000);

        const auto result = Map2DPointBetweenSurfaces<Test2DPoint, Test2DPoint>(p, s0, s1);

        EXPECT_FLOAT_EQ(result.x, 200.0f);
        EXPECT_FLOAT_EQ(result.y, 200.0f);
    }

    TEST(SpaceUtilTests, Map3DPointBetweenSurfaces1)
    {
        Test3DPoint p(100, 100, 2);
        TestSurface s0(500, 500);
        TestSurface s1(1000, 1000);

        const auto result = Map3DPointBetweenSurfaces<Test3DPoint, Test3DPoint>(p, s0, s1);

        EXPECT_FLOAT_EQ(result.x, 200.0f);
        EXPECT_FLOAT_EQ(result.y, 200.0f);
        EXPECT_FLOAT_EQ(result.z, 2.0f);
    }

    TEST(SpaceUtilTests, MapSurfacePointToPointSpaceCenterOrigin1)
    {
        Test2DPoint p(0, 0);
        TestSurface s(500, 500);

        const auto result = MapSurfacePointToPointSpaceCenterOrigin<TestSurface, Test2DPoint, Test3DPoint>(p, s);

        EXPECT_FLOAT_EQ(result.x, -250.0f);
        EXPECT_FLOAT_EQ(result.y, 250.0f);
        EXPECT_FLOAT_EQ(result.z, 0.0f);
    }

    TEST(SpaceUtilTests, MapSurfacePointToPointSpaceCenterOrigin2)
    {
        Test2DPoint p(500, 500);
        TestSurface s(500, 500);

        const auto result = MapSurfacePointToPointSpaceCenterOrigin<TestSurface, Test2DPoint, Test3DPoint>(p, s);

        EXPECT_FLOAT_EQ(result.x, 250.0f);
        EXPECT_FLOAT_EQ(result.y, -250.0f);
        EXPECT_FLOAT_EQ(result.z, 0.0f);
    }

    TEST(SpaceUtilTests, MapSurfacePointToPointSpaceCenterOrigin3)
    {
        Test2DPoint p(250, 0);
        TestSurface s(500, 500);

        const auto result = MapSurfacePointToPointSpaceCenterOrigin<TestSurface, Test2DPoint, Test3DPoint>(p, s);

        EXPECT_FLOAT_EQ(result.x, 0.0f);
        EXPECT_FLOAT_EQ(result.y, 250.0f);
        EXPECT_FLOAT_EQ(result.z, 0.0f);
    }
}

#endif //WIREDENGINE_NEONCOMMONTESTS_SPACEUTILTESTS_H
