#include <pangolin/pangolin.h>
#include <pangolin/glsl.h>
#include <pangolin/image_load.h>
#include <memory>

#ifdef HAVE_OCULUS
#include <OVR.h>
#endif

namespace std{ template<typename T, typename ...Args>
std::unique_ptr<T> make_unique( Args&& ...args ) {
    return std::unique_ptr<T>( new T( std::forward<Args>(args)... ) );
}}

const char* shadertoy_header =
        "#version 120\n"
        "uniform vec3      iResolution;           // viewport resolution (in pixels)\n"
        "uniform float     iGlobalTime;           // shader playback time (in seconds)\n"
        "uniform vec4      iMouse;                // mouse pixel coords. xy: current (if MLB down), zw: click\n"
        "uniform mat4      iT_bh;                 // Camera to World transform\n"
        "uniform vec4      iDate;                 // (year, month, day, time in s)\n"
        "uniform float     iChannelTime[4];       // channel playback time (in seconds)\n"
        "uniform vec3      iChannelResolution[4]; // channel resolution (in pixels)\n";

struct ShaderToyHandler : public pangolin::Handler
{
    void Mouse(pangolin::View& v, pangolin::MouseButton button, int x, int y, bool pressed, int button_state) {
        this->x = x; this->y = y;
        this->button_state = button_state;
    }

    void MouseMotion(pangolin::View&, int x, int y, int button_state) {
        this->x = x; this->y = y;
        this->button_state = button_state;
    }

    void Special(pangolin::View&, pangolin::InputSpecial inType, float x, float y, float p1, float p2, float p3, float p4, int button_state)
    {
        this->x = x; this->y = y;
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
    pangolin::View view;
    pangolin::DisplayBase().AddDisplay(view);
    ShaderToyHandler handler;
    view.SetHandler(&handler);

    // GLSL Source, starting with header
    std::stringstream glsl_buffer;
    glsl_buffer << shadertoy_header;
    
    // Load input textures
    pangolin::GlTexture iChannel[4];
    GLfloat sq_vert[] = { -1,-1,  1,-1,  1, 1,  -1, 1 };
    for(int i=0; i<4; ++i) {
        if(argc > 2+i) {
            pangolin::TypedImage img = pangolin::LoadImage(argv[2+i]);
            GLint channels;
            GLenum format;
            SetGlFormat(channels,format,img.fmt);
            iChannel[i].Reinitialise(img.w, img.h, channels, true, 0, channels, format, img.ptr);
            img.Dealloc();

            glActiveTexture(GL_TEXTURE0+i);
            iChannel[i].Bind();
            glsl_buffer << "uniform sampler2D iChannel" << i << ";" << std::endl;
        }
    }

    // Load ShaderToy body from file
    std::ifstream glsl_file(argv[1]);
    glsl_buffer << glsl_file.rdbuf();

    // Compile / link shader
    pangolin::GlSlProgram prog;
    prog.AddShader( pangolin::GlSlFragmentShader, glsl_buffer.str() );
    prog.Link();

#ifdef HAVE_OCULUS
    OVR::System::Init();
    OVR::Ptr<OVR::DeviceManager> pManager = *OVR::DeviceManager::Create();
    std::unique_ptr<OVR::SensorFusion> pFusionResult;
    OVR::Ptr<OVR::HMDDevice> pHMD;
    OVR::Ptr<OVR::SensorDevice> pSensor;
    if(pManager) {
        pHMD = *pManager->EnumerateDevices<OVR::HMDDevice>().CreateDevice();
        if (pHMD) {
            pSensor = *pHMD->GetSensor();
            if (pSensor) {
                pFusionResult = std::make_unique<OVR::SensorFusion>();
                pFusionResult->AttachToSensor(pSensor);
            }
        }
    }
#endif

    // head to body transform
    pangolin::OpenGlMatrix T_bh;
    T_bh.SetIdentity();

    float iGlobalTime = 0;
    while( !pangolin::ShouldQuit() )
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#ifdef HAVE_OCULUS
        if(pFusionResult) {
            T_bh = OVR::Matrix4f(pFusionResult->GetOrientation());
        }
#endif

        glColor3f(1.0f,1.0f,1.0f);
        prog.Bind();
        prog.SetUniform("iResolution", (float)pangolin::DisplayBase().v.w, (float)pangolin::DisplayBase().v.h, 1.0);
        prog.SetUniform("iGlobalTime", iGlobalTime );
        prog.SetUniform("iMouse", handler.x, handler.y, (float)handler.button_state, 0.0f);
        prog.SetUniform("iT_bh", T_bh);

        glEnableClientState(GL_VERTEX_ARRAY);
        glVertexPointer(2, GL_FLOAT, 0, sq_vert);
        glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        glDisableClientState(GL_VERTEX_ARRAY);
        prog.Unbind();
        
        pangolin::FinishFrame();
        iGlobalTime += 1.0f/60.0f;
    }
    
    return 0;
}
