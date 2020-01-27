#include "mines_sys.h"
#include "gl_shader.h"

/*
 * TODO: Clean up the error handling. 
 */

static bool read_file_text(const std::string& path, std::string& text_out)
{
    std::ifstream file(path.c_str());
    if (!file.is_open())
        return false;
        
    /* 
	 * Read whole file in to string.
	 * Note the extra parentheses around the first argument to the string constructor. 
	 * They prevent the problem known as the "most vexing parse". 
	 */
    text_out = std::string(
        (std::istreambuf_iterator<char>(file)),
        std::istreambuf_iterator<char>());

    file.close();

    return true;
}

gl_shader::gl_shader()
{
    m_pro_id = 0;
    m_shader_initialised = false;
}

gl_shader::~gl_shader() 
{
}

void gl_shader::clean_up() 
{
    stop_using();
    glDeleteProgram(m_pro_id);
    glFlush();
}

void gl_shader::add_attribute(const std::string& name) 
{
    gl_shader_var sv = {0, name}; 
    m_sha_attrib.push_back(sv);
}

void gl_shader::add_uniform(const std::string& name) 
{
    gl_shader_var sv = {0, name};
    m_sha_unif.push_back(sv);
}

GLuint gl_shader::get_attribute(const std::string& name) 
{
    for (auto it = m_sha_attrib.begin(); it != m_sha_attrib.end(); ++it)
        if (it->name == name && it->loc != (GLuint)-1)
            return it->loc;
    return (GLuint)-1;
}

GLuint gl_shader::get_uniform(const std::string& name) 
{
    for (auto it = m_sha_unif.begin(); it != m_sha_unif.end(); ++it) 
        if (it->name == name && it->loc != (GLuint)-1)
            return it->loc;
    return (GLuint)-1;
}

void gl_shader::init_from_file(const std::string& vs_file_path, const std::string& fs_file_path)
{
    std::string vs_code;
    if (!read_file_text(vs_file_path, vs_code))
        throw mines_exception("Failed to open vertex shader file for reading");

    std::string fs_code;
    if (!read_file_text(fs_file_path, fs_code))
        throw mines_exception("Failed to open fragment shader file for reading");

    init_from_str(vs_code, fs_code);
}

void gl_shader::init_from_str(const std::string& vs_code, const std::string& fs_code)
{
    const GLchar* vs_code_cstr = vs_code.c_str();
    const GLchar* fs_code_cstr = fs_code.c_str();

    // Create a compile vertex shader.
    m_vs_id = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(m_vs_id, 1, &vs_code_cstr, NULL);

    if (!compile(m_vs_id))
        throw mines_exception("Shader compile error");

    // Create and compile fragment shader.
    m_fs_id = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(m_fs_id, 1, &fs_code_cstr, NULL);

    if (!compile(m_fs_id))
        throw mines_exception("Shader compile error");

    m_pro_id = glCreateProgram();

    glAttachShader(m_pro_id, m_vs_id);
    glAttachShader(m_pro_id, m_fs_id);
    
    // Store locations of attributes.
    GLuint loc = 0;
    for(auto it = m_sha_attrib.begin(); it != m_sha_attrib.end(); ++it)
	{
        it->loc = loc++;
        glBindAttribLocation(m_pro_id, it->loc, it->name.c_str());
    }

    if (!link_prog(m_pro_id))
        throw mines_exception("Shader link error");

    glDetachShader(m_pro_id, m_vs_id);
    glDeleteShader(m_vs_id);
    glDetachShader(m_pro_id, m_fs_id);
    glDeleteShader(m_fs_id);

    // After linking, we can get locations for uniforms.
    for (auto it = m_sha_unif.begin(); it != m_sha_unif.end(); ++it)
	{
        GLint glret = glGetUniformLocation(m_pro_id, it->name.c_str());
        if (glret == -1)
            throw mines_exception("Unused or unrecognized uniform");
        it->loc = glret;
    }

    m_shader_initialised = true;
}

bool gl_shader::compile(GLuint shader_id)
{
	glCompileShader(shader_id);

	GLint param = 0;
	glGetShaderiv(shader_id, GL_COMPILE_STATUS, &param);

	if (param == GL_FALSE)
	{
		glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &param);

 		if (param > 0) 
		{
            GLchar* info = new GLchar[param];
            int nchars = 0;
            glGetShaderInfoLog(shader_id, param, &nchars, info);
			std::cout << info << std::endl;
			delete [] info;
 		}
		return false;
	}
	return true;
}

bool gl_shader::link_prog(GLuint program_id) 
{
    glLinkProgram(program_id);

    GLint param = 0;
    glGetProgramiv(program_id, GL_LINK_STATUS, &param);

    if (param == GL_FALSE) 
	{
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &param);

        if (param > 0) 
		{
            GLchar* info = new GLchar[param];
            int nchars = 0;
            glGetProgramInfoLog(program_id, param, &nchars, info);
            delete [] info;
        }
        return false;
    }
    return true;
}

bool gl_shader::use() 
{
    if (!m_shader_initialised)
        return false;

    glUseProgram(m_pro_id);
    return true;
}

void gl_shader::stop_using() 
{
    if (!m_shader_initialised)
        return;

    glUseProgram(0);
}
