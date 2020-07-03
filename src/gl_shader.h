#pragma once

#include <iostream>
#include <string>
#include <fstream>
#include <streambuf>
#include <vector>

#include "types.h"

#include "gl_platform.h"

struct gl_shader_var
{
    GLuint loc;         // The attribute location retrieved from GL.
    std::string name;   // The name of the attribute.
};

typedef std::vector<gl_shader_var> gl_shader_var_v;

class gl_shader 
{
public:
    gl_shader();
    ~gl_shader();

    // Initialise shader program from files.
    void init_from_file(const std::string& vs_file_path, const std::string& fs_file_path);

    // Initialise shader program from strings.
    void init_from_str(const std::string& vs_code, const std::string& fs_code);

    bool use();
    void stop_using();
    void clean_up();

    void add_attribute(const std::string& name);
    void add_uniform(const std::string& name);
    GLuint get_attribute(const std::string& name);
    GLuint get_uniform(const std::string& name);

private:

    bool compile(GLuint sha_id);
    bool link_prog(GLuint pro_id);

    gl_shader_var_v m_sha_attrib;
    gl_shader_var_v m_sha_unif;
    GLuint m_vs_id, m_fs_id;
    GLuint m_pro_id;

    bool m_shader_initialised;
};
