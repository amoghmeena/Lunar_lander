#include <vector>
#include <GL/glew.h>

enum Animation { MOVE_STRAIGHT,EXPLODE};

class Entity
{
private:
    std::vector<GLuint> m_texture_ids;
    std::vector<std::vector<int>> m_animations;

    glm::vec3 m_movement;
    glm::vec3 m_position;
    glm::vec3 m_scale;

    glm::mat4 m_model_matrix;
    float m_speed;

    int m_animation_cols;
    int m_animation_frames, m_animation_index, m_animation_rows;

    Animation m_current_animation;
    int* m_animation_indices = nullptr;
    float m_animation_time = 0.0f;
   

    glm::vec3 m_acceleration; // Acceleration vector (for movement)
    glm::vec3 m_velocity; // Velocity vector
    float m_gravity = -0.5f;  // Gravity constant
    float m_drag = 0.50f; // Drag factor (reduces movement over time)
    float m_fuel = 100.0f; // starting fuel

public:
    static constexpr int SECONDS_PER_FRAME = 6;

    Entity();
    Entity(std::vector<GLuint> texture_ids, float speed,
        std::vector<std::vector<int>> animations, float animation_time,
        int animation_frames, int animation_index, int animation_cols,
        int animation_rows, Animation animation);
    ~Entity();

    void draw_sprite_from_texture_atlas(ShaderProgram* program);
    void update(float delta_time);
    void render(ShaderProgram* program);

    void set_animation_state(Animation new_animation);
    void normalise_movement() { m_movement = glm::normalize(m_movement); };

    bool check_collision(Entity* other);

    glm::vec3 const get_acceleration() const { return m_acceleration; }
    void set_acceleration(glm::vec3 new_acceleration) { m_acceleration = new_acceleration; }

    glm::vec3 const get_velocity() const { return m_velocity; }
    void set_velocity(glm::vec3 new_velocity) { m_velocity = new_velocity; }

    float get_fuel() const { return m_fuel; }
    void set_fuel(float fuel) { m_fuel = fuel; }
    void consume_fuel(float amount) {
        m_fuel -= amount;
        if (m_fuel < 0.0f)
            m_fuel = 0.0f;
    }

    void move_straight();

    glm::vec3 const get_position() const { return m_position; }
    glm::vec3 const get_movement() const { return m_movement; }
    glm::vec3 const get_scale() const { return m_scale; }
    float const get_speed() const { return m_speed; }

    void const set_position(glm::vec3 new_position) { m_position = new_position; }
    void const set_movement(glm::vec3 new_movement) { m_movement = new_movement; }
    void const set_scale(glm::vec3 new_scale) { m_scale = new_scale; }
    void const set_speed(float new_speed) { m_speed = new_speed; }

    void draw_text(ShaderProgram* program, GLuint font_texture_id, std::string text, float font_size, float spacing, glm::vec3 position);

    void display_fuel(ShaderProgram* program, GLuint font_texture_id, float font_size, float spacing);
};

