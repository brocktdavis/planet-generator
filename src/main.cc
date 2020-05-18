#include <GL/glew.h>

#include "config.h"
#include "gui.h"
#include "mesh.h"
#include "render_pass.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/io.hpp>
#include <debuggl.h>

#include <QuickHull.hpp>

int window_width = 800, window_height = 600;
const std::string window_title = "Planet Generator";

// Shader includes
const char* vertex_shader =
#include "shaders/default.vert"
;

const char* planet_vertex_shader =
#include "shaders/planet.vert"
;

const char* hull_lines_fragment_shader =
#include "shaders/hull_lines.frag"
;

const char* hull_fragment_shader =
#include "shaders/hull.frag"
;

const char* voronoi_lines_fragment_shader =
#include "shaders/voronoi_lines.frag"
;

const char* planet_fragment_shader =
#include "shaders/planet.frag"
;

const char* floor_fragment_shader =
#include "shaders/floor.frag"
;

void ErrorCallback(int error, const char* description) {
	std::cerr << "GLFW Error: " << description << "\n";
}

GLFWwindow* init_glefw()
{
	if (!glfwInit())
		exit(EXIT_FAILURE);
	glfwSetErrorCallback(ErrorCallback);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4);
	auto ret = glfwCreateWindow(window_width, window_height, window_title.data(), nullptr, nullptr);
	CHECK_SUCCESS(ret != nullptr);
	glfwMakeContextCurrent(ret);
	glewExperimental = GL_TRUE;
	CHECK_SUCCESS(glewInit() == GLEW_OK);
	glGetError();  // clear GLEW's error for it
	glfwSwapInterval(1);
	const GLubyte* renderer = glGetString(GL_RENDERER);  // get renderer string
	const GLubyte* version = glGetString(GL_VERSION);    // version as a string
	std::cout << "Renderer: " << renderer << "\n";
	std::cout << "OpenGL version supported:" << version << "\n";

	return ret;
}

void create_floor(std::vector<glm::vec3>& floor_vertices, std::vector<glm::uvec3>& floor_faces)
{
	floor_vertices.push_back(glm::vec3(kFloorXMin, kFloorY, kFloorZMax));
	floor_vertices.push_back(glm::vec3(kFloorXMax, kFloorY, kFloorZMax));
	floor_vertices.push_back(glm::vec3(kFloorXMax, kFloorY, kFloorZMin));
	floor_vertices.push_back(glm::vec3(kFloorXMin, kFloorY, kFloorZMin));
	floor_faces.push_back(glm::uvec3(0, 1, 2));
	floor_faces.push_back(glm::uvec3(2, 3, 0));
}

glm::vec3 parseHexCode(std::string hexstring)
{
	unsigned hexval;
	std::stringstream ss;
	ss << std::hex << hexstring;
	ss >> hexval;

	float r(((hexval >> 16) & 0xFF) / 255.0f);
	float g(((hexval >> 8) & 0xFF) / 255.0f);
	float b((hexval & 0xFF) / 255.0f);

	return glm::vec3(r, g, b);
}

int main(int argc, char* argv[])
{
	// Planet parameters
	static unsigned num_regions;
	static int height_param;
	static float ocean_height;
	static unsigned noise_seed;

	std::string ocean_str, snow_str, coast_str, vegetation_str;

	static glm::vec3 ocean_c;
	static glm::vec3 snow_c;
	static glm::vec3 coast_c;
	static glm::vec3 vegetation_c;

	// Draw paramters
	bool draw_planet = true;
	bool draw_poly_lines = false;
	bool draw_hull = false;

	try {
		po::options_description desc("Allowed options");
		desc.add_options()
			("help,h", "Display help messagee")
			("regions,r", po::value<unsigned>(&num_regions)->default_value(10000), "Set the number of regions, between 500 and 100,000")
			("ocean_ht,o", po::value<int>(&height_param)->default_value(120), "Set the height of the ocean, between 0 (everything terrain) and 200 (everything underwater)")
			("seed,s", po::value<unsigned>(&noise_seed)->default_value(8675309), "Set the seed for the height noise function, between 0 and 4,294,967,295")
			("planet,p", "Don't render the planet. Not setting this flag renders the planet as is default behavior")
			("polygons,g", "Render the polygons on the terrain of the planet.")
			("hull,l", "Render the convex hull of the original points. Need to also hide the planet with --planet or -p")
			("ocean_color", po::value<std::string>(&ocean_str)->default_value("1a1a66"), "Set the color of the ocean in hexadecimal\n(000000 - ffffff)")
			("snow_color", po::value<std::string>(&snow_str)->default_value("ffffff"), "Set the color of the snow in hexadecimal\n(000000 - ffffff)")
			("coast_color", po::value<std::string>(&coast_str)->default_value("edd640"), "Set the color of the coast in hexadecimal\n(000000 - ffffff)")
			("vegetation_color", po::value<std::string>(&vegetation_str)->default_value("006600"), "Set the color of the vegetation in hexadecimal\n(000000 - ffffff)")
		;

		po::variables_map vm;
		po::store(po::parse_command_line(argc, argv, desc), vm);
		po::notify(vm);

		if (vm.count("help")) {
			std::cout << desc << "\n";
			return 0;
		}

		// Regions
		if (num_regions < 500 || num_regions > 100000) {
			std::cerr << "Invalid number of regions. Must be in range [500-100000]\n";
			return 1;
		}

		// Ocean height
		if (height_param < 0 || height_param > 200) {
			std::cerr << "Invalid ocean height parameter.\n";
			return 1;
		}
		ocean_height = 1.0f + ((height_param / 1000.0f) - 0.1f);

		// Seed, unneeded

		// Draw flags
		if (vm.count("planet")) draw_planet = false;
		if (vm.count("polygons")) draw_poly_lines = true;
		if (vm.count("hull")) draw_hull = true;

		// Colors
		ocean_c = parseHexCode(ocean_str);
		snow_c = parseHexCode(snow_str);
		coast_c = parseHexCode(coast_str);
		vegetation_c = parseHexCode(vegetation_str);
	} catch (std::exception& e) {
		std::cerr << "error: " << e.what() << "\n";
		return 1;
	} catch (...) {
		std::cerr << "Exception of unknown type!\n";
	}

	GLFWwindow *window = init_glefw();
	GUI gui(window);

	/** I. Build meshes **/
	std::vector<glm::vec3> floor_vertices;
	std::vector<glm::uvec3> floor_faces;
	create_floor(floor_vertices, floor_faces);

	Mesh planet(num_regions, noise_seed);

	/** II. Build Uniforms **/
	MatrixPointers mats;

	// Viewing data
	std::function<glm::mat4()> model_data = [&mats]() { return *mats.model; };
	std::function<glm::mat4()> view_data = [&mats]() { return *mats.view; };
	std::function<glm::mat4()> proj_data = [&mats]() { return *mats.projection; };

	// Elevation data
	std::function<float()> ocean_lvl = []() { return ocean_height; };
	std::function<float()> max_el = []() { return (1.0f / elevation_divisor) + (1.0f - ocean_height); };

	// Color data
	std::function<glm::vec3()> ocean_color = []() { return ocean_c; };
	std::function<glm::vec3()> snow_color = []() { return snow_c; };
	std::function<glm::vec3()> coast_color = []() { return coast_c; };
	std::function<glm::vec3()> vegetation_color = []() { return vegetation_c; };

	auto std_model = make_uniform("model", model_data);
	auto std_view = make_uniform("view" , view_data);
	auto std_proj = make_uniform("projection", proj_data);

	auto ocean = make_uniform("ocean_height", ocean_lvl);
	auto mx_el = make_uniform("max_elevation", max_el);

	auto oc_col = make_uniform("ocean_color", ocean_color);
	auto sn_col = make_uniform("snow_color", snow_color);
	auto cs_col = make_uniform("coast_color", coast_color);
	auto vg_col = make_uniform("vegetation_color", vegetation_color);

	/** III. Build RenderPass Objects from inside out **/
	RenderDataInput hull_lines_input;
	hull_lines_input.assign(0, "vertex_position", planet.hull_points.data(), planet.hull_points.size(), 3, GL_FLOAT);
	hull_lines_input.assignIndex(planet.hull_indices.data(), planet.hull_indices.size(), 2);
	RenderPass hull_lines_pass(-1,
			hull_lines_input,
			{ vertex_shader, nullptr, hull_lines_fragment_shader },
			{ std_model, std_view, std_proj },
			{ "fragment_color" }
			);


	RenderDataInput hull_input;
	hull_input.assign(0, "vertex_position", planet.hull_points.data(), planet.hull_points.size(), 3, GL_FLOAT);
	hull_input.assignIndex(planet.hull_faces.data(), planet.hull_faces.size(), 3);
	RenderPass hull_pass(-1,
			hull_input,
			{ vertex_shader, nullptr, hull_fragment_shader },
			{ std_model, std_view, std_proj },
			{ "fragment_color" }
			);

	RenderDataInput voronoi_lines_input;
	voronoi_lines_input.assign(0, "vertex_position", planet.vertices.data(), planet.vertices.size(), 3, GL_FLOAT);
	voronoi_lines_input.assignIndex(planet.lines.data(), planet.lines.size(), 2);
	RenderPass voronoi_lines_pass(-1,
			voronoi_lines_input,
			{ vertex_shader, nullptr, voronoi_lines_fragment_shader },
			{ std_model, std_view, std_proj },
			{ "fragment_color" }
			);

	RenderDataInput planet_input;
	planet_input.assign(0, "vertex_position", planet.vertices.data(), planet.vertices.size(), 3, GL_FLOAT);
	planet_input.assignIndex(planet.faces.data(), planet.faces.size(), 3);
	RenderPass planet_pass(-1,
			planet_input,
			{ planet_vertex_shader, nullptr, planet_fragment_shader },
			{ std_model, std_view, std_proj, ocean, mx_el, oc_col, sn_col, cs_col, vg_col },
			{ "fragment_color" }
			);

	RenderDataInput floor_input;
	floor_input.assign(0, "vertex_position", floor_vertices.data(), floor_vertices.size(), 3, GL_FLOAT);
	floor_input.assignIndex(floor_faces.data(), floor_faces.size(), 3);
	RenderPass floor_pass(-1,
			floor_input,
			{ vertex_shader, nullptr, floor_fragment_shader},
			{ std_model, std_view, std_proj },
			{ "fragment_color" }
			);

	while (!glfwWindowShouldClose(window)) {
		// Setup some basic window stuff.
		glfwGetFramebufferSize(window, &window_width, &window_height);
		glViewport(0, 0, window_width, window_height);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_MULTISAMPLE);
		glEnable(GL_BLEND);
		glEnable(GL_CULL_FACE);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDepthFunc(GL_LESS);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		glCullFace(GL_BACK);

		gui.updateMatrices();
		mats = gui.getMatrixPointers();

		if (draw_hull)
		{
			// Draw lines
			hull_lines_pass.setup();
			CHECK_GL_ERROR(glDrawElements(GL_LINES,
										  planet.hull_indices.size() * 2,
										  GL_UNSIGNED_INT,
										  0));
			// Then draw triangles
			hull_pass.setup();
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
										  planet.hull_faces.size() * 3,
										  GL_UNSIGNED_INT,
										  0));
		}

		if (draw_poly_lines)
		{
			voronoi_lines_pass.setup();
			CHECK_GL_ERROR(glDrawElements(GL_LINES,
										  planet.lines.size() * 2,
										  GL_UNSIGNED_INT,
										  0));
		}

		if (draw_planet)
		{
			planet_pass.setup();
			CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
										  planet.faces.size() * 3,
										  GL_UNSIGNED_INT,
										  0));
		}

		// Always draw floor
		floor_pass.setup();
		CHECK_GL_ERROR(glDrawElements(GL_TRIANGLES,
									  floor_faces.size() * 3,
									  GL_UNSIGNED_INT,
									  0));

		// Poll and swap.
		glfwPollEvents();
		glfwSwapBuffers(window);
	}
	glfwDestroyWindow(window);
	glfwTerminate();
	exit(EXIT_SUCCESS);
}
