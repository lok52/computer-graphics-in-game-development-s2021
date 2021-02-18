#define CATCH_CONFIG_MAIN

#include "renderer/raytracer/raytracer.h"
#include "resource.h"

#include <catch.hpp>


SCENARIO("Raytracer with shadow rays")
{
	GIVEN("2 raytracers, vertex buffer, render target, lights")
	{
		cg::renderer::raytracer<cg::vertex, cg::unsigned_color> raytracer;
		raytracer.set_viewport(5, 5);
		cg::renderer::raytracer<cg::vertex, cg::unsigned_color> shadow_raytracer;

		auto render_target = std::make_shared<cg::resource<cg::unsigned_color>>(5, 5);
		raytracer.set_render_target(render_target);

		std::vector<std::shared_ptr<cg::resource<cg::vertex>>> vertex_buffer;
		vertex_buffer.push_back(std::make_shared<cg::resource<cg::vertex>>(6));
		vertex_buffer[0]->item(0).x = -0.5f;
		vertex_buffer[0]->item(0).y = -0.5f;
		vertex_buffer[0]->item(0).z = -1.f;
		vertex_buffer[0]->item(0).nx = 0.f;
		vertex_buffer[0]->item(0).ny = 0.f;
		vertex_buffer[0]->item(0).nz = 1.f;
		vertex_buffer[0]->item(1).x = 0.5f;
		vertex_buffer[0]->item(1).y = -0.5f;
		vertex_buffer[0]->item(1).z = -1.f;
		vertex_buffer[0]->item(1).nx = 0.f;
		vertex_buffer[0]->item(1).ny = 0.f;
		vertex_buffer[0]->item(1).nz = 1.f;
		vertex_buffer[0]->item(2).x = 0.f;
		vertex_buffer[0]->item(2).y = 0.5f;
		vertex_buffer[0]->item(2).z = -1.f;
		vertex_buffer[0]->item(2).nx = 0.f;
		vertex_buffer[0]->item(2).ny = 0.f;
		vertex_buffer[0]->item(2).nz = 1.f;

		vertex_buffer[0]->item(3).x = 0.5f;
		vertex_buffer[0]->item(3).y = 0.4f;
		vertex_buffer[0]->item(3).z = 0.9f;
		vertex_buffer[0]->item(3).nx = 0.f;
		vertex_buffer[0]->item(3).ny = 0.f;
		vertex_buffer[0]->item(3).nz = 1.f;
		vertex_buffer[0]->item(4).x = 0.4f;
		vertex_buffer[0]->item(4).y = 0.6f;
		vertex_buffer[0]->item(4).z = 0.9f;
		vertex_buffer[0]->item(4).nx = 0.f;
		vertex_buffer[0]->item(4).ny = 0.f;
		vertex_buffer[0]->item(4).nz = 1.f;
		vertex_buffer[0]->item(5).x = 0.6f;
		vertex_buffer[0]->item(5).y = 0.6f;
		vertex_buffer[0]->item(5).z = 0.9f;
		vertex_buffer[0]->item(5).nx = 0.f;
		vertex_buffer[0]->item(5).ny = 0.f;
		vertex_buffer[0]->item(5).nz = 1.f;

		raytracer.set_per_shape_vertex_buffer(vertex_buffer);
		raytracer.build_acceleration_structure();
		shadow_raytracer.acceleration_structures = raytracer.acceleration_structures;

		std::vector<cg::renderer::light> lights;
		lights.push_back({ float3{ 0.5f, 0.5f, 1.f }, float3{ 1.f, 0.f, 0.f } });
		lights.push_back({ float3{ -0.5f, 0.5f, 1.f }, float3{ 0.f, 1.f, 0.f } });

		WHEN("Setup miss, hit shaders and generate rays")
		{
			raytracer.clear_render_target({ 0, 0, 0 });

			shadow_raytracer.miss_shader = [](const cg::renderer::ray& ray) {
				cg::renderer::payload payload = {};
				payload.t = -1.f;
				return payload;
			};
			shadow_raytracer.any_hit_shader =
				[](const cg::renderer::ray& ray, cg::renderer::payload& payload,
				   const cg::renderer::triangle<cg::vertex>& triangle) {
					return payload;
				};


			raytracer.miss_shader = [](const cg::renderer::ray& ray) {
				cg::renderer::payload payload = {};
				payload.t = -1.f;
				payload.color = { 0.f, 0.f, 0.f };
				return payload;
			};

			raytracer.closest_hit_shader =
				[&](const cg::renderer::ray& ray, cg::renderer::payload& payload,
					const cg::renderer::triangle<cg::vertex>& triangle, size_t depth) {
					float3 result_color = float3{ 0.f, 0.f, 0.f };

					float3 point = ray.position + ray.direction * payload.t;
					float3 normal = payload.bary.x * triangle.na +
									payload.bary.y * triangle.nb +
									payload.bary.z * triangle.nc;

					for (auto& light : lights)
					{
						cg::renderer::ray to_light(point, light.position - point);

						auto shadow_payload = shadow_raytracer.trace_ray(
							to_light, 1, length(light.position - point));

						if (shadow_payload.t == -1.f)
						{

							result_color +=
								light.color *
								std::max(dot(normal, to_light.direction), 0.f);
						}
					}
					payload.color = cg::color::from_float3(result_color);
					return payload;
				};

			raytracer.ray_generation(
				float3{ 0.f, 0.f, 1.f }, float3{ 0.f, 0.f, -1.f },
				float3{ 1.f, 0.f, 0.f }, float3{ 0.f, 1.f, 0.f });

			THEN("Make sure that image is correct")
			{
				for (size_t x = 0; x < 5; x++)
				{
					for (size_t y = 0; y < 5; y++)
					{
						if (x == 2 && y == 2)
						{
							REQUIRE(render_target->item(x, y).r == 0);
							REQUIRE(render_target->item(x, y).g == 240);
							REQUIRE(render_target->item(x, y).b == 0);
						}
						else
						{
							REQUIRE(render_target->item(x, y).r == 0);
							REQUIRE(render_target->item(x, y).g == 0);
							REQUIRE(render_target->item(x, y).b == 0);
						}
					}
				}
			};
		}
	}
}
