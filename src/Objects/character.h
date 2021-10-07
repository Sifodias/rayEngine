#pragma once
#include <raylib.h>
#include <nlohmann/json.hpp>

#include "hObject.h"
#include "rigidBody.h"
#include "sprite.h"
#include "runes.h"

class Character : public HObject {
public:
    Character(nlohmann::json obj);
    void routine();
    void draw();
    void shoot();

private:
    RigidBody* body_;
    Sprite* sprite_;    
};
