#include "../view.hpp"
#include "ui/canvas.hpp"

#include "imgui.h"
#include "features/features.hpp"
#include "input/input_service.hpp"
#include "utility/unreal_engine_utility.hpp"

namespace big
{
    struct star_node
    {
        float x, y;
    };

    struct zodiac_shape_t
    {
        std::vector<star_node> nodes;
        std::vector<std::pair<int, int>> edges;
    };

    static zodiac_shape_t zodiac_complex_1 =
    {
        {
            { -40, -60 }, { -10, -30 }, { 20, -20 }, { 50, 10 },
            { 30, 40 }, { 0, 60 }, { -30, 40 }, { -50, 10 },

            // branches
            { -10, -30 }, { -20, -10 }, { -35, -5 },
            { 20, -20 }, { 35, -35 }, { 45, -50 }
        },
        {
            // main spine
            {0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},{7,0},

            // inner cross
            {1,4},{2,6},

            // branches kiri
            {1,8},{8,9},{9,10},

            // branches kanan
            {2,11},{11,12},{12,13}
        }
    };

    static zodiac_shape_t zodiac_complex_2 =
    {
        {
            { -50, 0 }, { -30, -20 }, { -10, -30 }, { 10, -20 },
            { 30, 0 }, { 40, 25 }, { 20, 50 }, { -10, 60 },

            // tail detail
            { 20, 50 }, { 35, 60 }, { 50, 70 },

            // inner detail
            { -10, -30 }, { -5, -50 }
        },
        {
            {0,1},{1,2},{2,3},{3,4},{4,5},{5,6},{6,7},

            // tail
            {6,8},{8,9},{9,10},

            // inner
            {2,11},{11,12}
        }
    };

    static zodiac_shape_t zodiac_3 =
    {
        {
            // main spine (atas → bawah)
            { 0, -50 },    // 0
            { 15, -30 },   // 1
            { 25, -10 },   // 2
            { 20, 15 },    // 3
            { 5, 40 },     // 4
            { -10, 60 },   // 5

            // kiri branch (atas)
            { -15, -25 },  // 6
            { -30, -10 },  // 7

            // kanan branch (tengah)
            { 40, 5 },     // 8
            { 55, 15 },    // 9

            // small inner detail
            { 5, -5 },     // 10
        },
        {
            // main chain
            {0,1},{1,2},{2,3},{3,4},{4,5},

            // left branch
            {1,6},{6,7},

            // right branch
            {2,8},{8,9},

            // inner detail
            {2,10},{10,3}
        }
    };

    static zodiac_shape_t zodiac_4 =
    {
        {
            { -30, 10 },   // 0 left base
            { -10, -20 },  // 1 top left
            { 20, -25 },   // 2 top right
            { 40, 0 },     // 3 right mid
            { 20, 30 },    // 4 bottom right
            { -10, 35 },   // 5 bottom left

            { -45, -5 },   // 6
            { -55, 15 },   // 7

            { 55, -5 },    // 8
            { 70, 5 },     // 9
            { 80, -10 },   // 10
            { 95, -20 },   // 11
            { 90, 0 },     // 12
        },
        {
            // body loop
            {0,1},{1,2},{2,3},{3,4},{4,5},{5,0},

            // handle
            {0,6},{6,7},

            // spout
            {3,8},{8,9},

            // arrow
            {8,10},{10,11},{10,12}
        }
    };
    static void draw_orbit(float cx, float cy)
    {
        float time = ImGui::GetTime();

        float radius = 80.f;
        float speed = 2.0f;

        int count = 12;

        for (int i = 0; i < count; i++)
        {
            float angle = time * speed + (i * (2 * M_PI / count));

            float x = cx + cosf(angle) * radius;
            float y = cy + sinf(angle) * radius;

            canvas::draw_circle(x, y, 3.f, Color(255, 255, 255, 200), 12);
        }
    }
    static void draw_rotating_ring(float cx, float cy)
    {
        float time = ImGui::GetTime();

        float base_radius = 100.f;
        float rotation = time * 1.5f;

        int segments = 64;

        for (int i = 0; i < segments; i++)
        {
            float a1 = (i / (float)segments) * 2 * M_PI + rotation;
            float a2 = ((i + 1) / (float)segments) * 2 * M_PI + rotation;

            float x1 = cx + cosf(a1) * base_radius;
            float y1 = cy + sinf(a1) * base_radius;

            float x2 = cx + cosf(a2) * base_radius;
            float y2 = cy + sinf(a2) * base_radius;

            canvas::draw_line(x1, y1, x2, y2, Color(180, 180, 255, 120), 1.f);
        }
    }
    static void draw_pulse(float cx, float cy)
    {
        float time = ImGui::GetTime();

        float pulse = (sinf(time * 3.f) + 1.f) * 0.5f; // 0 → 1
        float radius = 40.f + pulse * 20.f;

        canvas::draw_circle(
            cx,
            cy,
            radius,
            Color(255, 255, 255, (int)(pulse * 200)),
            64
        );
    }
    static void draw_zodiac(zodiac_shape_t const& shape, float cx, float cy, float scale, float speed, float time_offset)
    {
        float time = ImGui::GetTime() + time_offset;

        float angle = time * speed;
        float pulse = 1.0f + sinf(time * 2.f) * 0.05f;

        std::vector<SDK::FVector2D> transformed;
        transformed.reserve(shape.nodes.size());

        for (size_t i = 0; i < shape.nodes.size(); i++)
        {
            const auto& p = shape.nodes[i];

            float x = p.x * scale * pulse;
            float y = p.y * scale * pulse;

            float rx = x * cosf(angle) - y * sinf(angle);
            float ry = x * sinf(angle) + y * cosf(angle);

            transformed.push_back({ cx + rx, cy + ry });
        }

        for (auto& [a, b] : shape.edges)
        {
            const auto& p1 = transformed[a];
            const auto& p2 = transformed[b];

            canvas::draw_line(p1.X, p1.Y, p2.X, p2.Y, Color(100, 100, 255, 40), 3.f);

            canvas::draw_line(p1.X, p1.Y, p2.X, p2.Y, Color(200, 200, 255, 180), 1.f);
        }

        for (size_t i = 0; i < transformed.size(); i++)
        {
            const auto& p = transformed[i];

            float twinkle = (sinf(time * 3.f + i) + 1.f) * 0.5f;

            float size = 2.0f + (i % 3);
            int alpha = (int)(150 + twinkle * 100);

            // base star
            canvas::draw_circle(
                p.X,
                p.Y,
                size,
                Color(255, 255, 255, alpha),
                8
            );

            if (i % 4 == 0)
            {
                canvas::draw_circle(
                    p.X,
                    p.Y,
                    size + 2.0f,
                    Color(255, 255, 255, 220),
                    12
                );
            }
        }
    }

	void view::draw_animation()
	{
        using namespace features;

        if (!_draw_anim.get_state())
            return;

        float width = static_cast<float>(g_pointers->m_resolution->x / 2);
        float height = static_cast<float>(g_pointers->m_resolution->y / 2);

        float offset_x = 200.f;
        float offset_y = 150.f;

        float cx = width;
        float cy = height;

        draw_orbit(cx, cy);
        draw_rotating_ring(cx, cy);
        draw_pulse(cx, cy);

        draw_zodiac(
            zodiac_complex_1,
            width - offset_x,
            height - offset_y,
            1.0f,
            0.4f,
            0.0f
        );

        // kanan atas
        draw_zodiac(
            zodiac_complex_2,
            width + offset_x,
            height - offset_y,
            1.0f,
            -0.5f,
            1.5f
        );

        // kiri bawah
        draw_zodiac(
            zodiac_3,
            width - offset_x,
            height + offset_y,
            1.1f,
            0.3f,
            3.0f
        );

        // kanan bawah
        draw_zodiac(
            zodiac_4,
            width + offset_x,
            height + offset_y,
            1.1f,
            -0.4f,
            5.0f
        );
	}
}