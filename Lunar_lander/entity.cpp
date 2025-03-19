#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

constexpr int FONTBANK_SIZE = 16;

Entity::Entity()
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(0.0f), m_animation_cols(0), m_animation_frames(0), m_animation_index(0),
    m_animation_rows(0), m_animation_indices(nullptr), m_animation_time(0.0f),
    m_current_animation(MOVE_STRAIGHT)
{
}

Entity::Entity(std::vector<GLuint> texture_ids, float speed,
    std::vector<std::vector<int>> animations, float animation_time,
    int animation_frames, int animation_index, int animation_cols,
    int animation_rows, Animation animation)
    : m_position(0.0f), m_movement(0.0f), m_scale(1.0f, 1.0f, 0.0f), m_model_matrix(1.0f),
    m_speed(speed), m_texture_ids(texture_ids), m_animations(animations),
    m_animation_cols(animation_cols), m_animation_frames(animation_frames),
    m_animation_index(animation_index), m_animation_rows(animation_rows),
    m_animation_time(animation_time), m_current_animation(animation)
{
    set_animation_state(m_current_animation);
}

Entity::~Entity() {}

void Entity::set_animation_state(Animation new_animation)
{
    m_current_animation = new_animation;
    m_animation_indices = m_animations[m_current_animation].data();

    switch (new_animation)
    {
   
    case MOVE_STRAIGHT:
        m_animation_frames = 1;
        m_animation_rows = 1;
        m_scale = glm::vec3(2.0f, 2.0f, 0.0f);
        break;
    default:
        m_animation_frames = 1;
        m_animation_rows = 1;
        m_scale = glm::vec3(2.0f, 2.0f, 0.0f);
        break;
    }
}

bool Entity::check_collision(Entity* other)
{
    glm::vec3 posA = this->m_position;
    glm::vec3 posB = other->m_position;

    float ax = this->m_scale.x / 2.0f;
    float ay = this->m_scale.y / 2.0f;
    float bx = other->m_scale.x / 2.0f;
    float by = other->m_scale.y / 2.0f;

    return (fabs(posA.x - posB.x) < (ax + bx)) && (fabs(posA.y - posB.y) < (ay + by));
}


void Entity::draw_sprite_from_texture_atlas(ShaderProgram* program)
{
    GLuint current_texture = m_texture_ids[m_current_animation];

    // Ensure m_animation_cols and m_animation_rows are not zero
    if (m_animation_cols == 0 || m_animation_rows == 0) {
        std::cerr << "Error: Animation columns or rows are zero." << std::endl;
        return;
    }

    float u_coord = (float)(m_animation_index % m_animation_cols) / (float)m_animation_cols;
    float v_coord = (float)(m_animation_index / m_animation_cols) / (float)m_animation_rows;

    float width = 1.0f / (float)m_animation_cols;
    float height = 1.0f / (float)m_animation_rows;

    float tex_coords[] =
    {
        u_coord, v_coord + height, u_coord + width, v_coord + height, u_coord + width,
        v_coord, u_coord, v_coord + height, u_coord + width, v_coord, u_coord, v_coord
    };

    float vertices[] =
    {
        -0.5, -0.5, 0.5, -0.5,  0.5, 0.5,
        -0.5, -0.5, 0.5,  0.5, -0.5, 0.5
    };

    glBindTexture(GL_TEXTURE_2D, current_texture);

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->get_position_attribute());

    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void Entity::update(float delta_time)
{
    // Apply gravity
    m_acceleration.y += m_gravity * delta_time;

    // Update velocity using acceleration
    m_movement += m_acceleration * delta_time;

    // Update position
    m_position += m_movement * m_speed * delta_time;

    // Slowly reduce acceleration (simulate drag/friction)
    m_acceleration *= 0.98f;

    // Update model matrix
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);
    m_model_matrix = glm::scale(m_model_matrix, m_scale);
}
void Entity::render(ShaderProgram* program)
{
    program->set_model_matrix(m_model_matrix);

    if (m_animation_indices != nullptr) draw_sprite_from_texture_atlas(program);
}




void Entity::move_straight()
{
    if (m_current_animation != MOVE_STRAIGHT)
    {
        set_animation_state(MOVE_STRAIGHT);
    }
}


void Entity::draw_text(ShaderProgram* program, GLuint font_texture_id, std::string text, float font_size, float spacing, glm::vec3 position) {
    // Scale the size of the fontbank in the UV-plane
    // We will use this for spacing and positioning
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    // Instead of having a single pair of arrays, we'll have a series of pairs—one for
    // each character. Don't forget to include <vector>!
    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    // For every character...
    for (int i = 0; i < text.size(); i++) {
        // 1. Get their index in the spritesheet, as well as their offset (i.e. their
        //    position relative to the whole sentence)
        int spritesheet_index = (int)text[i];  // ascii value of character
        float offset = (font_size + spacing) * i;

        // 2. Using the spritesheet index, we can calculate our U- and V-coordinates
        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        // 3. Inset the current pair in both vectors
        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size), 0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate, v_coordinate,
            u_coordinate, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate + width, v_coordinate + height,
            u_coordinate + width, v_coordinate,
            u_coordinate, v_coordinate + height,
            });
    }

    // 4. And render all of them using the pairs
    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    program->set_model_matrix(model_matrix);
    glUseProgram(program->get_program_id());

    glVertexAttribPointer(program->get_position_attribute(), 2, GL_FLOAT, false, 0,
        vertices.data());
    glEnableVertexAttribArray(program->get_position_attribute());
    glVertexAttribPointer(program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
        texture_coordinates.data());
    glEnableVertexAttribArray(program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(program->get_position_attribute());
    glDisableVertexAttribArray(program->get_tex_coordinate_attribute());
}

void Entity::display_fuel(ShaderProgram* program, GLuint font_texture_id, float font_size, float spacing) {
    std::string fuel_text = "Fuel: " + std::to_string((int)m_fuel);

    glm::vec3 top_left(-4.5f, 3.4f, 0.0f);

    draw_text(program, font_texture_id, fuel_text, font_size, spacing, top_left);
}