#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
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
#include "Entity.h"
#include <ctime>
#include "cmath"

// ————— CONSTANTS ————— //
constexpr int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 480;

constexpr float BG_RED = 0.1765625f,
BG_GREEN = 0.17265625f,
BG_BLUE = 0.1609375f,
BG_OPACITY = 1.0f;

constexpr int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr float MILLISECONDS_IN_SECOND = 1000.0;

constexpr glm::vec3 PLANE_IDLE_SCALE = glm::vec3(1.0f, 1.0f, 0.0f);
constexpr glm::vec3 PLANE_IDLE_LOCATION = glm::vec3(-1.0f, 0.0f, 0.0f);

constexpr GLint NUMBER_OF_TEXTURES = 1,
LEVEL_OF_DETAIL = 0,
TEXTURE_BORDER = 0;

// ————— STRUCTS AND ENUMS —————//
enum AppStatus { RUNNING, TERMINATED };
enum FilterType { NEAREST, LINEAR };

struct GameState{
    Entity * spaceship;
    std::vector<Entity*> asteroids; // obstacles
    bool game_over = false;         // collision/game over flag
    bool game_won = false;          // win flag
};


// ————— VARIABLES ————— //
GameState g_game_state;

SDL_Window* g_display_window;
AppStatus g_app_status = RUNNING;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;


GLuint g_background_texture;
GLuint g_game_over_texture;
GLuint g_win_texture;
GLuint g_font_texture_id;

void initialise();
void process_input();
void update();
void render();
void shutdown();


// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath, FilterType filterType)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components,
        STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER,
        GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
        filterType == NEAREST ? GL_NEAREST : GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
        filterType == NEAREST ? GL_NEAREST : GL_LINEAR);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Let's play Lunar-lander!",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

    if (g_display_window == nullptr)
    {
        std::cerr << "Error: SDL window could not be created.\n";
        shutdown();
    }

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // Load textures
    g_background_texture = load_texture("assets/Lunar_bg.png", NEAREST);
    GLuint asteroid_texture = load_texture("assets/asteroid.png", NEAREST);
    g_game_over_texture = load_texture("assets/over.png", NEAREST);
    g_win_texture = load_texture("assets/win.png", NEAREST);
    g_font_texture_id = load_texture("assets/font1.png", NEAREST);



    // Spaceship setup  
    std::vector<GLuint> game_textures_ids = { load_texture("assets/spaceship.png", NEAREST) };
    std::vector<std::vector<int>> ship_animations = { {0} };

    g_game_state.spaceship = new Entity(
        game_textures_ids,
        3.0f,  // Non-zero speed
        ship_animations,
        0.0f,
        1,
        0,
        1,
        1,
        MOVE_STRAIGHT
    );

    g_game_state.spaceship->set_scale(glm::vec3(0.5f, 0.5f, 1.0f));
    g_game_state.spaceship->set_position(glm::vec3(0.0f, 2.0f, 0.0f));

    // Create multiple asteroid obstacles
    for (int i = 0; i < 5; i++) {
        Entity* asteroid = new Entity(
            { asteroid_texture },
            0.0f, // Asteroids do not move
            { {0} },
            0.0f,
            1,
            0,
            1,
            1,
            MOVE_STRAIGHT
        );

        // Randomly position asteroids
        float x = -4.0f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 8.0f)); // Keep within the screen width
        float y = -2.5f + static_cast<float>(rand()) / (static_cast<float>(RAND_MAX / 4.0f)); // Keep within the screen height

        asteroid->set_position(glm::vec3(x, y, 0.0f));
        asteroid->set_scale(glm::vec3(1.0f, 1.0f, 1.0f));

        g_game_state.asteroids.push_back(asteroid);
    }

    Entity* asteroid = new Entity(
        { asteroid_texture },
        2.0f, 
        { {0} },
        0.0f,
        1,
        0,
        1,
        1,
        MOVE_STRAIGHT
    );

    asteroid->set_position(glm::vec3(3, 0, 0.0f));
    asteroid->set_scale(glm::vec3(1.0f, 1.0f, 1.0f));

    g_game_state.asteroids.push_back(asteroid);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}
void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_app_status = TERMINATED;
            break;

        case SDL_KEYDOWN:
            if (event.key.keysym.sym == SDLK_q)
                g_app_status = TERMINATED;
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    glm::vec3 acceleration = g_game_state.spaceship->get_acceleration();

    // Apply acceleration when pressing movement keys
    if (g_game_state.spaceship->get_fuel() > 0.0f)
    {
        if (key_state[SDL_SCANCODE_A]) {
            acceleration.x = -1.5f; // Move left
        }
        else if (key_state[SDL_SCANCODE_D]) {
            acceleration.x = 1.5f;  // Move right
        }
        else {
            acceleration.x = 0.0f;  // No horizontal thrust
        }

        // For vertical thrust, if desired:
        if (key_state[SDL_SCANCODE_W]) {
            acceleration.y = 1.5f; // For example, thrust upward
        }
        else {
            // Let gravity do its work (or set a baseline, if needed)
            acceleration.y = -0.5f;
        }
    }
    else {
        // No fuel left: ensure no acceleration is applied
        acceleration = glm::vec3(0.0f);
        acceleration.y = -0.5f;
    }

    g_game_state.spaceship->set_acceleration(acceleration);
}


void update()
{
    if (g_game_state.game_over) return; // Stop updating if the game is over

    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;

    g_game_state.spaceship->update(delta_time);

    // Update each asteroid so their model matrices are recalculated
    for (Entity* asteroid : g_game_state.asteroids) {
        asteroid->update(delta_time);
    }

    glm::vec3 accel = g_game_state.spaceship->get_acceleration();
    if ((fabs(accel.x) > 0.01f || fabs(accel.y) > 0.01f) && g_game_state.spaceship->get_fuel() > 0.0f)
    {
        // Consume fuel at a rate of 10 units per second (adjust as needed)
        g_game_state.spaceship->consume_fuel(5.0f * delta_time);
    }

    if (g_game_state.spaceship->get_position().x >= 5.0f) {
        g_game_state.game_won = true;
        return;
    }

    // Check for collisions with asteroids
    for (Entity* asteroid : g_game_state.asteroids) {
        if (g_game_state.spaceship->check_collision(asteroid)) {
            g_game_state.game_over = true;
            return;
        }
    }
}

void render_background()
{
    glUseProgram(g_shader_program.get_program_id()); // Ensure shader is active

    glm::mat4 identity_matrix = glm::mat4(1.0f);
    g_shader_program.set_model_matrix(identity_matrix); // Reset transformations

    glBindTexture(GL_TEXTURE_2D, g_background_texture);

    float vertices[] = {
        -5.0f, -3.75f,   // Bottom-left
         5.0f, -3.75f,   // Bottom-right
         5.0f,  3.75f,   // Top-right
        -5.0f,  3.75f    // Top-left
    };

    float tex_coords[] = {
        0.0f, 1.0f,  // Bottom-left
        1.0f, 1.0f,  // Bottom-right
        1.0f, 0.0f,  // Top-right
        0.0f, 0.0f   // Top-left
    };

    GLuint indices[] = { 0, 1, 2, 0, 2, 3 };  // Two triangles

    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);

    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
}

void render_end_screen(GLuint texture)
{
    // Bind the given texture (win or lose)
    glBindTexture(GL_TEXTURE_2D, texture);

    // Define a quad that covers the desired area.
    // Adjust these values as needed.
    float vertices[] = {
        -2.5f, -2.0f,  // Bottom-left
         2.5f, -2.0f,  // Bottom-right
         2.5f,  2.0f,  // Top-right
        -2.5f,  2.0f   // Top-left
    };

    float tex_coords[] = {
        0.0f, 1.0f,  // Bottom-left
        1.0f, 1.0f,  // Bottom-right
        1.0f, 0.0f,  // Top-right
        0.0f, 0.0f   // Top-left
    };

    GLuint indices[] = { 0, 1, 2, 0, 2, 3 };

    // Enable the position attribute and pass in the vertices
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);

    // Enable the texture coordinate attribute and pass in the texture coordinates
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);

    // Draw the quad using indices
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, indices);

    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
}

void render()
{
    glClear(GL_COLOR_BUFFER_BIT);

    render_background();

    if (g_game_state.game_won) {
        render_end_screen(g_win_texture);
    }
    else if (g_game_state.game_over) {
        render_end_screen(g_game_over_texture);
    }
    else {
        // Render the normal game objects
        for (Entity* asteroid : g_game_state.asteroids) {
            glUseProgram(g_shader_program.get_program_id());
            asteroid->render(&g_shader_program);
        }
        g_game_state.spaceship->render(&g_shader_program);
    }

    g_game_state.spaceship->display_fuel(&g_shader_program, g_font_texture_id, 0.5f, 0.05f);

    SDL_GL_SwapWindow(g_display_window);
}


void shutdown()
{
    SDL_Quit();
    delete   g_game_state.spaceship;
}


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
