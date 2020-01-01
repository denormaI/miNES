#include "display_window.h"
#include "nes_ppu.h"
#include "mines_sys.h"

#ifdef _WIN32
#include <windows.h>
#endif

display_window::display_window(bool keep_ppu_aspect) :
		m_initialised(false),
		m_keep_ppu_aspect(keep_ppu_aspect),
		m_should_quit(false)
{
	m_quad_vs_code =
		"#version 440 core	\n"

		"in vec3 position;"
		"in vec2 tex_coord;"
		"out vec2 tex_coord_through;"

		"void main(void)"
		"{"
		"	gl_Position = vec4(position, 1.0);"
		"	tex_coord_through = tex_coord;"
		"}";

	m_quad_fs_code =
		"#version 440 core	\n"

		"in vec2 tex_coord_through;"
		"out vec4 color_out;"
		"uniform sampler2D tex_sampler;"

		"void main(void)"
		"{"
		"	color_out = texture(tex_sampler, tex_coord_through);"
		"}";

}

void display_window::register_key_event_handler(const key_event_func& handler)
{
	if (handler != nullptr)
		m_key_event_func = handler;
}

void display_window::key_event(int glfw3_key, bool press)
{
	if (m_key_event_func != nullptr)
		m_key_event_func(glfw3_key, press);
}

void display_window::update_window_size(int w, int h)
{
	m_width = w;
	m_height = h;
}

void display_window::create(const std::string& title, int w, int h)
{
	if (m_initialised)
		throw mines_exception("ERROR: Window already created.");

	if (SDL_Init(SDL_INIT_VIDEO) < 0)
		throw mines_exception("Failed to init SDL");

	//Use OpenGL 3.1 core
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 1 );
	SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

	m_window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		w, h, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);	
	if (!m_window)
		throw mines_exception("Unable to create window");

	m_sdl_context = SDL_GL_CreateContext(m_window);
	if(!m_sdl_context)
		throw mines_exception("OpenGL context could not be created>");

	// Turn on double buffering.
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

	glewExperimental = GL_TRUE;
	GLenum err = glewInit();
	if (err != GLEW_OK)
		throw mines_exception("ERROR: " + std::string((const char*)glewGetErrorString(err)));

	SDL_GL_SetSwapInterval(1);

	init_display_quad();

	m_width = w;
	m_height = h;

	m_initialised = true;
}

void display_window::init_display_quad()
{
	glGenTextures(1, &m_bitmap_tex_id);
	glBindTexture(GL_TEXTURE_2D, m_bitmap_tex_id);

	// Avoid some artifacts
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// no mipmap
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, ppu_types::VISIBLE_DOTS, ppu_types::VISIBLE_LINES, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

	float quad_vertices[] = {
		/*
		clock-wise order
		positions xyz,    texture coords st
		triangle 1
		*/
		-1.0f, -1.0f, 0.0f,   0.0f, 1.0f,   // bottom left
		-1.0f, 1.0f,  0.0f,   0.0f, 0.0f,    // top left
		1.0f,  1.0f,  0.0f,   1.0f, 0.0f,   // top right

		// triangle 2
		1.0f,  -1.0f, 0.0f,   1.0f, 1.0f,   // bottom right
		-1.0f, -1.0f, 0.0f,   0.0f, 1.0f,   // bottom left  
		1.0f,  1.0f,  0.0f,   1.0f, 0.0f,   // top right
	};

	// The fullscreen quad's VAO
	glGenVertexArrays(1, &m_quad_vao);
	glBindVertexArray(m_quad_vao);

	// The fullscreen quad's VBO
	glGenBuffers(1, &m_quad_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_quad_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quad_vertices), quad_vertices, GL_STATIC_DRAW);

	// Add attributes/uniforms and initialise the shader.
	m_sha.add_attribute("position");
	m_sha.add_attribute("tex_coord");
	m_sha.add_uniform("tex_sampler");
	m_sha.init_from_str(m_quad_vs_code, m_quad_fs_code);

	// After initialization the attribute/uniform locations can be retrieved.
	GLuint pos_attrib = m_sha.get_attribute("position");
	GLuint tex_coord_attrib = m_sha.get_attribute("tex_coord");
	m_tex_sampler_unif = m_sha.get_uniform("tex_sampler");

	// Position attribute.
	glVertexAttribPointer(pos_attrib, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)0);
	glEnableVertexAttribArray(pos_attrib);

	// Texture coordinate attribute.
	glVertexAttribPointer(tex_coord_attrib, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (GLvoid*)(3 * sizeof(float)));
	glEnableVertexAttribArray(tex_coord_attrib);
}

void display_window::update_bitmap(const ppu_types::display_bitmap_t& image)
{
	if (m_initialised)
	{
		glBindTexture(GL_TEXTURE_2D, m_bitmap_tex_id);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, ppu_types::VISIBLE_DOTS, ppu_types::VISIBLE_LINES, GL_RGBA, GL_UNSIGNED_BYTE, image.data());
		glBindTexture(GL_TEXTURE_2D, 0);
	}
}

bool display_window::window_should_close() 
{ 
	return m_should_quit;
}

void display_window::set_window_should_close(bool value)
{
	m_should_quit = value;
}

void display_window::swap_buffers()
{
	SDL_GL_SwapWindow(m_window);
}

void display_window::poll_events()
{
	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		if (event.type == SDL_QUIT)
			m_should_quit = true;

		if(event.type == SDL_WINDOWEVENT)
		{
			if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED)
			{
				m_width = event.window.data1;
				m_height = event.window.data2;
			}
		}

		if (event.type == SDL_KEYDOWN || event.type == SDL_KEYUP)
		{
			int key = event.key.keysym.sym;
			bool press = true;

			switch (event.type)
			{
			case SDL_KEYDOWN: press = true; break;
			case SDL_KEYUP: press = false; break;
			}

			if (key == SDLK_ESCAPE && press)
				set_window_should_close(true);

			key_event(key, press);		
		}
	}
}

void display_window::render()
{
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	GLsizei vpx = 0, vpy = 0;
	GLsizei vpw = m_width, vph = m_height;

	if (m_keep_ppu_aspect) 
	{
		// the requested ratio
		GLfloat req_ratio = (GLfloat)ppu_types::VISIBLE_DOTS / (GLfloat)ppu_types::VISIBLE_LINES;

		// actual (window) ratio
		GLfloat aspect = (GLfloat)m_width / (GLfloat)m_height;
		GLfloat aspect_scale;

		if (aspect > req_ratio) 
		{
			// scale by height
			aspect_scale = (GLfloat)m_height / (GLfloat)ppu_types::VISIBLE_LINES;
			vpy = 0;
			vpx = (GLsizei)((m_width / 2) - ((ppu_types::VISIBLE_DOTS * aspect_scale) / 2));
		}
		else 
		{
			// scale by width
			aspect_scale = (GLfloat)m_width / (GLfloat)ppu_types::VISIBLE_DOTS;
			vpx = 0;
			vpy = (GLsizei)((m_height / 2) - ((ppu_types::VISIBLE_LINES * aspect_scale) / 2));
		}

		vpw = (GLsizei)((GLfloat)ppu_types::VISIBLE_DOTS * aspect_scale);
		vph = (GLsizei)((GLfloat)ppu_types::VISIBLE_LINES * aspect_scale);
	}

	glViewport(vpx, vpy, vpw, vph);

	m_sha.use();
	
	glBindTexture(GL_TEXTURE_2D, m_bitmap_tex_id);
	glUniform1i(m_tex_sampler_unif, 0);

	glBindVertexArray(m_quad_vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	glFinish();
}