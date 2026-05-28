#include "character.hpp"

Character::Character(const std::string& name)
    : Object3D(name)
    , m_state(CharacterState::IDLE) {
    cameraOffset = m_cameraOffset.current;
    scale = m_size.standing;
    color[0] = color[1] = color[2] = 1.f;
    chunkManager = nullptr;
    physics = nullptr;
}

void Character::update(float deltaTime) {
    updateMovement(deltaTime);
    updateState();
    if (physics && chunkManager) m_movement.isGrounded = checkGrounded();
}

void Character::updateMovement(float deltaTime) {
    calculateTargetVelocity();
    smoothVelocity(deltaTime);

    velocity.x = -m_movement.velocity.x;
    velocity.z = -m_movement.velocity.z;

    if (m_input.jump) {
        jump();
        m_input.jump = false;
    }

    if (m_input.crouch) {
        crouch();
    }
    else {
        stand();
    }

    m_movement.currentSpeed = Vector3(
        velocity.x,
        0,
        velocity.z
    ).length();
    cameraOffset = m_cameraOffset.current;
}

void Character::calculateTargetVelocity() {
    if (!m_camera) return;

    float speed = m_input.run ? m_params.runSpeed : m_params.walkSpeed;
    if (m_state == CharacterState::CROUCHING) {
        speed = m_params.crouchSpeed;
    }

    Vector3 cameraForward(sin(m_camera->rotation.y), 0, cos(m_camera->rotation.y));
    Vector3 cameraRight(cos(m_camera->rotation.y), 0, -sin(m_camera->rotation.y));

    cameraForward = cameraForward.normalized();
    cameraRight = cameraRight.normalized();

    Vector3 moveDir(0, 0, 0);
    if (m_input.forward) moveDir += cameraForward;
    if (m_input.backward) moveDir -= cameraForward;
    if (m_input.left) moveDir += cameraRight;
    if (m_input.right) moveDir -= cameraRight;

    if (moveDir.length() > 0) {
        moveDir = moveDir.normalized();
        m_movement.moveDirection = moveDir;
        m_movement.targetVelocity = moveDir * speed;
    }
    else {
        m_movement.moveDirection = Vector3(0, 0, 0);
        m_movement.targetVelocity = Vector3(0, 0, 0);
    }
}

void Character::smoothVelocity(float deltaTime) {
    float acceleration = m_movement.isGrounded ?
        m_params.acceleration :
        m_params.acceleration * m_params.airControl;

    float deceleration = m_movement.isGrounded ?
        m_params.deceleration :
        m_params.deceleration * m_params.airControl;

    Vector3 currentHorizontal(m_movement.velocity.x, 0, m_movement.velocity.z);
    Vector3 targetHorizontal(m_movement.targetVelocity.x, 0, m_movement.targetVelocity.z);

    if (targetHorizontal.length() > 0) {
        Vector3 delta = targetHorizontal - currentHorizontal;
        if (delta.length() > acceleration * deltaTime) {
            delta = delta.normalized() * acceleration * deltaTime;
        }
        currentHorizontal += delta;
    }
    else {
        float speed = currentHorizontal.length();
        if (speed > 0) {
            currentHorizontal = currentHorizontal.normalized() * 
                std::max(0.0f, speed - deceleration * deltaTime);
        }
    }

    m_movement.velocity.x = currentHorizontal.x;
    m_movement.velocity.z = currentHorizontal.z;
}

void Character::updateState() {
    CharacterState newState = m_state;

    if (m_input.crouch) {
        newState = CharacterState::CROUCHING;
    }
    else if (m_movement.currentSpeed > 0.1f) {
        newState = CharacterState::MOVING;
    }
    else {
        newState = CharacterState::IDLE;
    }
    if (!m_movement.isGrounded) newState = CharacterState::FALLING;
    if (m_state != CharacterState::FALLING && m_input.jump) newState = CharacterState::JUMPING;

    if (newState != m_state) {
        setState(newState);
    }
}
void Character::setState(CharacterState newState) {
    m_state = newState;
    //updateSize();
}

void Character::handleInput(const CharacterInput& input, bool release) {
    if (!release) {
        switch (input) {
        case CharacterInput::JUMP: m_input.jump = true; break;
        case CharacterInput::CROUCH: m_input.crouch = true; break;
        case CharacterInput::FORWARD: m_input.forward = true; break;
        case CharacterInput::BACKWARD: m_input.backward = true; break;
        case CharacterInput::LEFT: m_input.left = true; break;
        case CharacterInput::RIGHT: m_input.right = true; break;
        case CharacterInput::RUN: m_input.run = true; break;
        }
    }
    else {
        switch (input) {
        case CharacterInput::JUMP: m_input.jump = false; break;
        case CharacterInput::CROUCH: m_input.crouch = false; break;
        case CharacterInput::FORWARD: m_input.forward = false; break;
        case CharacterInput::BACKWARD: m_input.backward = false; break;
        case CharacterInput::LEFT: m_input.left = false; break;
        case CharacterInput::RIGHT: m_input.right = false; break;
        case CharacterInput::RUN: m_input.run = false; break;
        }
    }
}

std::string Character::characterStateToString() {
    switch (m_state) {
    case CharacterState::IDLE:      return "IDLE";
    case CharacterState::MOVING:    return "MOVING";
    case CharacterState::JUMPING:   return "JUMPING";
    case CharacterState::FALLING:   return "FALLING";
    case CharacterState::CROUCHING: return "CROUCHING";
    default: return "UNKNOWN";
    }
}

bool Character::checkGrounded() const {
    float bottomY = position.y - scale.y * 0.5f;
    Vector3 rayOrigin = Vector3(position.x, bottomY + 0.1f, position.z);
    Vector3 rayDir = Vector3(0, -1, 0);
    float rayLength = 0.2f;
    Vector3 hitPoint;
    std::shared_ptr<Object3D> hitObj;
    if (physics->raycast(rayOrigin, rayDir, rayLength, hitPoint, hitObj)) {
        return true;
    }
    if (chunkManager) {
        float terrainY = chunkManager->getHeightAt(position.x, position.z);
        if (std::abs(bottomY - terrainY) < 0.2f) {
            return true;
        }
    }
    return false;
}

void Character::updateSize() {
    switch (m_state) {
    case CharacterState::CROUCHING:
        scale = m_size.crouching;
        break;
    default:
        scale = m_size.standing;
        break;
    }
}

void Character::jump() {
    if (m_state != CharacterState::FALLING) {
        velocity.y = m_params.jumpForce / 2.f;
        m_input.jump = false;
        setState(CharacterState::JUMPING);
    }
}

void Character::crouch() {
    if (m_state != CharacterState::CROUCHING) {
        setState(CharacterState::CROUCHING);
    }
}

void Character::stand() {
    if (m_state == CharacterState::CROUCHING) {
        setState(CharacterState::IDLE);
    }
}

void Character::applyForce(const Vector3& force) {
    if (isStatic) return;
    m_movement.velocity += force / mass * 0.1f;
}