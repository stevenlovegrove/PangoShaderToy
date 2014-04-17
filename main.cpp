#include <pangolin/pangolin.h>
#include <pangolin/glsl.h>
#include <pangolin/image_load.h>

const char* shadertoy_header =
        "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
        "uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
        "uniform float     iChannelTime[4];       // channel playback time (in seconds)\n"
        "uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n"
        "uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\n"
        "uniform sampler2D iChannel0;             // input channel. XX = 2D/Cube\n"
        "uniform vec4      iDate;                 // (year, month, day, time in s)\n";

struct ShaderToyHandler : public pangolin::Handler
{
    void Mouse(pangolin::View&, pangolin::MouseButton button, int x, int y, bool pressed, int button_state) {
        this->x = x;
        this->y = y;
        this->button_state = button_state;
    }

    void MouseMotion(pangolin::View&, int x, int y, int button_state) {
        this->x = x;
        this->y = y;
        this->button_state = button_state;
    }

    int button_state;
    float x,y;
};

void SetGlFormat(GLint& glchannels, GLenum& glformat, const pangolin::VideoPixelFormat& fmt)
{
    switch( fmt.channels) {
    case 1: glchannels = GL_LUMINANCE; break;
    case 3: glchannels = GL_RGB; break;
    case 4: glchannels = GL_RGBA; break;
    default: throw std::runtime_error("Unable to display video format");
    }

    switch (fmt.channel_bits[0]) {
    case 8: glformat = GL_UNSIGNED_BYTE; break;
    case 16: glformat = GL_UNSIGNED_SHORT; break;
    case 32: glformat = GL_FLOAT; break;
    default: throw std::runtime_error("Unknown channel format");
    }
}

int main( int argc, char** argv )
{  
    if(argc <= 1) {
        std::cout << "Usage: ShaderToy shader.frag" << std::endl;
        return -1;
    }

    pangolin::CreateWindowAndBind("Main",640,480);
    ShaderToyHandler handler;
    pangolin::DisplayBase().SetHandler(&handler);

    // Load GlSl from file, adding header
    std::ifstream glsl_file(argv[1]);
    std::stringstream glsl_buffer;
    glsl_buffer << shadertoy_header;
    glsl_buffer << glsl_file.rdbuf();
    
    // Compile / link shader
    pangolin::GlSlProgram prog;
    prog.AddShader( pangolin::GlSlFragmentShader, glsl_buffer.str() );
    prog.Link();

    pangolin::GlTexture iChannel0;
    if(argc > 2) {
        pangolin::TypedImage img = pangolin::LoadImage(argv[2]);
        GLint channels;
        GLenum format;
        SetGlFormat(channels,format,img.fmt);
        iChannel0.Reinitialise(img.w, img.h, channels, true,0,channels,format,img.ptr);
        img.Dealloc();
    }

    GLfloat sq_vert[] = { -1,-1,  1,-1,  1, 1,  -1, 1 };
    GLfloat sq_tex[]  = { 0,0,  1,0,  1,1,  0,1  };

    float iGlobalTime = 0;
    while( !pangolin::ShouldQuit() )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glColor3f(1.0f,1.0f,1.0f);
        prog.Bind();
        prog.SetUniform("iResolution", (float)pangolin::DisplayBase().v.w, (float)pangolin::DisplayBase().v.h, 1.0);
        prog.SetUniform("iGlobalTime", iGlobalTime );
        prog.SetUniform("iMouse", handler.x, handler.y, 0.0f, 0.0f);

        iChannel0.Bind();
        glEnableClientState(GL_VERTEX_ARRAY);
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, sq_vert);
        glTexCoordPointer(2, GL_FLOAT, 0, sq_tex);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDisableClientState(GL_VERTEX_ARRAY);
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
        prog.Unbind();
        
        pangolin::FinishFrame();
        iGlobalTime += 1.0f/60.0f;
    }
    
    return 0;
}
