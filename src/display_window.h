#pragma once

#include <string>
#include <functional>
#include <memory>

#include <GL/glew.h>

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "types.h"
#include "gl_shader.h"

using namespace ppu_types;

class nes_ppu;

class display_window 
{
public:
	display_window(bool keep_ppu_aspect);
	virtual ~display_window() {}

	void render();
	void create(const std::string& title, int w, int h);

	void update_bitmap(const display_bitmap_t& image);
	void update_window_size(int w, int h);

	bool window_should_close();
	void set_window_should_close(bool value);

	void swap_buffers();
	void poll_events();

	typedef std::function<void(int sdl2_key, bool press)> key_event_func;

	// Callback mechanism for key events.
	void register_key_event_handler(const key_event_func& handler);

	void key_event(int sdl2_key, bool press);

private:
	std::string m_quad_vs_code;
	std::string m_quad_fs_code;

	void init_display_quad();

	int m_width, m_height;
	gl_shader m_sha;

	//GLFWwindow* m_window;
	SDL_Window* m_window;
	SDL_GLContext m_sdl_context;

	GLuint m_bitmap_tex_id;
	GLuint m_quad_vao, m_quad_vbo;
	GLuint m_tex_sampler_unif;

	bool m_initialised;
	bool m_keep_ppu_aspect;
	bool m_should_quit;

	key_event_func m_key_event_func;
};

