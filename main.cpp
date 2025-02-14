#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION
#define LOG(argument) std::cout << argument << '\n'
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum AppStatus { RUNNING, TERMINATED };

constexpr int WINDOW_WIDTH  = 640 * 2,
              WINDOW_HEIGHT = 480 * 2;

constexpr float BG_RED     = 1.0f,
                BG_GREEN   = 1.0f,
                BG_BLUE    = 1.0f,
                BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
               F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr GLint NUMBER_OF_TEXTURES = 1,
                LEVEL_OF_DETAIL    = 0,
                TEXTURE_BORDER     = 0;

constexpr char HELI_FILEPATH[]    = "helicopter.png",
               CAR_FILEPATH[] = "car.png";

constexpr glm::vec3 INIT_SCALE       = glm::vec3(5.0f, 5.98f, 0.0f),
                    INIT_POS_HELI    = glm::vec3(2.0f, 0.0f, 0.0f),
                    INIT_POS_CAR = glm::vec3(-2.0f, 0.0f, 0.0f);

constexpr float ROT_INCREMENT = 1.0f;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;
ShaderProgram g_shader_program = ShaderProgram();

glm::mat4 g_view_matrix,
          g_heli_matrix,
          g_car_matrix,
          g_projection_matrix;

glm::vec3 g_heli_translation  = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_car_translation  = glm::vec3(0.0f, 0.0f, 0.0f);

float g_previous_ticks = 0.0f;

glm::vec3 g_rotation_heli    = glm::vec3(0.0f, 0.0f, 0.0f),
          g_rotation_car = glm::vec3(0.0f, 0.0f, 0.0f);

GLuint g_heli_texture_id,
       g_car_texture_id;


GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    stbi_image_free(image);

    return textureID;
}


void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);

    g_display_window = SDL_CreateWindow("Helicopter and Car",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        SDL_Quit();
        exit(1);
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_heli_matrix       = glm::mat4(1.0f);
    g_car_matrix     = glm::mat4(1.0f);
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    g_heli_texture_id    = load_texture(HELI_FILEPATH);
    g_car_texture_id = load_texture(CAR_FILEPATH);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}


void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
    }
}


void update()
{
    float ticks = (float) SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_rotation_car.y += -1.0f * delta_time;
    
    g_heli_translation.x += 1.0f * delta_time;
    g_heli_translation.y += 1.0f * delta_time;
    g_car_translation.x += 1.3f * delta_time;
    g_car_translation.y += 1.3f * delta_time;

    // Change scale of helicopter
    float scale_factor = 1.0f + 0.5f * sin(ticks * 2.0f);

    // Reset
    g_heli_matrix    = glm::mat4(1.0f);
    g_car_matrix    = glm::mat4(1.0f);

    // Move in circles
    g_heli_matrix = glm::translate(g_heli_matrix, glm::vec3(cos(g_heli_translation.x), sin(g_heli_translation.y), 0.0f));
    
    // Move in circles around the helicopter
    g_car_matrix = glm::translate(g_heli_matrix, glm::vec3(cos(g_car_translation.x), sin(g_car_translation.y), 0.0f));
    
    // Helicopter getting bigger and smaller
    g_heli_matrix = glm::scale(g_heli_matrix, glm::vec3(scale_factor, scale_factor, 1.0f));

    // Rotation
    g_car_matrix = glm::rotate(g_car_matrix, g_rotation_car.y, glm::vec3(1.0f, 1.0f, 0.0f));
}


void draw_object(glm::mat4 &object_g_model_matrix, GLuint &object_texture_id)
{
    g_shader_program.set_model_matrix(object_g_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so use 6, not 3
}


void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    // Vertices
    float vertices[] =
    {
        -0.5f, -0.5f, 0.5f, -0.5f, 0.5f, 0.5f,  // triangle 1
        -0.5f, -0.5f, 0.5f, 0.5f, -0.5f, 0.5f   // triangle 2
    };

    // Textures
    float texture_coordinates[] =
    {
        0.0f, 1.0f, 1.0f, 1.0f, 1.0f, 0.0f,     // triangle 1
        0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f,     // triangle 2
    };

    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false,
                          0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());

    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT,
                          false, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    // Bind texture
    draw_object(g_heli_matrix, g_heli_texture_id);
    draw_object(g_car_matrix, g_car_texture_id);

    // We disable two attribute arrays now
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown() { SDL_Quit(); }


int main(int argc, char* argv[])
{
    initialise();

    while (g_app_status == RUNNING)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
