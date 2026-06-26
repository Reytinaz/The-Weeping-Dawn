#ifndef CHARACTER_H
#define CHARACTER_H

#include "object3d.hpp"
#include "camera.hpp"
#include "physics.hpp"

// Состояния персонажа
enum class CharacterState {
    IDLE,
    MOVING,
    JUMPING,
    FALLING,
    CROUCHING,
};
enum class CharacterInput{
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT,
    JUMP,
    CROUCH,
    RUN,
};

class Character : public Object3D {
private:
    CharacterState m_state;

    struct {
        Vector3 standing = Vector3(0.5f, 1.0f, 0.5f);
        Vector3 crouching = Vector3(0.5f, 0.5f, 0.5f);
        Vector3 current = Vector3(0.5f, 1.0f, 0.5f);
    } m_size;

    struct {
        bool forward = false;
        bool backward = false;
        bool left = false;
        bool right = false;
        bool jump = false;
        bool run = false;
        bool crouch = false;
    } m_input;

    struct {
        Vector3 firstPerson = Vector3(0, 0.75f, 0);
        Vector3 thirdPerson = Vector3(0, 2.0f, -5.0f);
        Vector3 current = Vector3(0, 0.5f, 0);
    } m_cameraOffset;

    std::function<void()> onLandCallback;
    std::function<void()> onJumpCallback;
    std::function<void(CharacterState)> onStateChangeCallback;

    void updateMovement(float deltaTime);
    void updateState();
    void calculateTargetVelocity();
    void smoothVelocity(float deltaTime);
    void updateSize();
public:
    Character(const std::string& name = "Character");

    std::shared_ptr<Camera> m_camera;
    Vector3 cameraOffset;
    Physics* physics;
    ChunkManager* chunkManager;

    struct {
        Vector3 velocity;
        Vector3 targetVelocity;
        Vector3 moveDirection;
        bool isGrounded = false;
        float verticalVelocity = 0.0f;
        float currentSpeed = 0.0f;
    } m_movement;
    struct {
        float walkSpeed = 4.0f;
        float runSpeed = 6.0f;
        float crouchSpeed = 4.0f;
        float jumpForce = 10.0f;
        float acceleration = 80.0f;
        float deceleration = 180.0f;
        float airControl = 0.3f;
        float gravity = 5.f;
    } m_params;

    void update(float deltaTime);
    void setState(CharacterState newState);
    CharacterState getState() const { return m_state; }
    void handleInput(const CharacterInput& input, bool release);
    std::string characterStateToString();
    bool checkGrounded() const;
    Vector3 getVelocity() const { return m_movement.velocity; }

    void applyForce(const Vector3& force);
    void jump();
    void crouch();
    void stand();
};

#endif