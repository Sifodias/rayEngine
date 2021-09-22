#include <iostream>
#include "rigidBody.h"
#include "math.h"
#include "float.h"
#include "bullet.h"
#include "definitions.h"

Quadtree RigidBody::quad;
std::map<int, RigidBody*> RigidBody::pool;

using json = nlohmann::json;
using namespace std;

//TODO: handle case when out of box quad

RigidBody::RigidBody(Rectangle surface, bool solid) : surface_(surface), solid_(solid) {
    if (pool.size())
        pool_id_ = (--pool.end())->first + 1;
    else
        pool_id_ = 0;
    pool[pool_id_] = this;
    quad.add(quadNode{ pool_id_, surface_ });
}

RigidBody::RigidBody(json obj, GObject* father) {
    Rectangle rect{ 0.0f };
    speed_ = { 0 };
    solid_ = true;
    acceleration_ = 0;
    curve_ = 0;
    father_ = father;

    if (!obj.contains("body")) {
        cout << "ERROR: no rigidbody in json" << endl;
        return;
    }
    obj = obj["body"];

    if (obj.contains("x"))
        rect.x = obj["x"];

    if (obj.contains("y"))
        rect.y = obj["y"];

    if (obj.contains("w"))
        rect.width = obj["w"];

    if (obj.contains("h"))
        rect.height = obj["h"];

    surface_ = rect;
    if (obj.contains("solid"))
        solid_ = obj["solid"];

    if (obj.contains("acceleration"))
        acceleration_ = obj["acceleration"];

    if (obj.contains("curve"))
        curve_ = obj["curve"];

    if (obj.contains("speed")) {
        if (obj["speed"].contains("x")) {
            speed_.x = obj["speed"]["x"];
        }
        if (obj["speed"].contains("y")) {
            speed_.y = obj["speed"]["y"];
        }
    }

    if (pool.size())
        pool_id_ = (--pool.end())->first + 1;
    else
        pool_id_ = 0;
    pool[pool_id_] = this;

    if (t(*father_) != t(Bullet)) {
        quad.add(quadNode{ pool_id_, surface_ });
    }
}

RigidBody::~RigidBody() {
    pool.erase(pool_id_);
    if (t(*father_) != t(Bullet)) {
        quad.remove(quadNode{ pool_id_, surface_ });
    }
}

void RigidBody::setSolid(bool solid) {
    solid_ = solid;
}
void RigidBody::setCurve(double curve) {
    curve_ = curve;
}

std::vector<RigidBody*> RigidBody::query(Rectangle rect, bool force_solid) {
    auto queryVec = quad.query(rect);
    std::vector<RigidBody*> ret;

    for (auto node : queryVec) {
        if ((pool[node.id]->solid_ && force_solid) || !force_solid) {
            ret.push_back(pool[node.id]);
        }
    }
    return ret;
}

Vector2 RigidBody::getCoord() {
    return Vector2{ surface_.x, surface_.y };
}

void RigidBody::setCoord(Vector2 pos) {
    if (t(*father_) != t(Bullet)) {
        quad.remove({ pool_id_, surface_ });
    }
    surface_.x = pos.x;
    surface_.y = pos.y;
    if (t(*father_) != t(Bullet)) {
        quad.add({ pool_id_, surface_ });
    }
    clock_.getLap();
}

void RigidBody::setSpeed(Vector2 speed) {
    speed_ = speed;
}
Vector2 RigidBody::getSpeed() {
    return speed_;
}
void RigidBody::fixSpeed() {
    if (solid_) {
        Rectangle fixSpeed = surface_;
        if (speed_.x > 0)
            fixSpeed.width += 1;
        if (speed_.x < 0)
            fixSpeed.x -= 1;
        for (RigidBody* body : query(fixSpeed))
            if (body->solid_ && (body->pool_id_ != pool_id_))
                speed_.x = 0;

        fixSpeed = surface_;
        if (speed_.y > 0)
            fixSpeed.height += 1;
        if (speed_.y < 0)
            fixSpeed.y -= 1;
        for (RigidBody* body : query(fixSpeed))
            if (body->solid_ && (body->pool_id_ != pool_id_))
                speed_.y = 0;
    }
}

void RigidBody::routine() {
    float delta = (float)clock_.getLap();
    if (abs(speed_.x) < FLT_EPSILON && abs(speed_.y) < FLT_EPSILON)
        return;

    float tempX = cos(curve_ * delta) * speed_.x - sin(curve_ * delta) * speed_.y;
    float tempY = sin(curve_ * delta) * speed_.x + cos(curve_ * delta) * speed_.y;
    speed_ = { tempX, tempY };

    fixSpeed();

    if (t(*father_) != t(Bullet)) {
        quad.remove({ pool_id_, surface_ });
    }

    float speedNorm = sqrt(pow(speed_.x * delta, 2) + pow(speed_.y * delta, 2));
    Vector2 unitSpeed = { speed_.x * delta / speedNorm, speed_.y * delta / speedNorm };

    while (speedNorm > 0) {
        Rectangle temp = surface_;
        temp.x += unitSpeed.x * speedNorm;
        temp.y += unitSpeed.y * speedNorm;
        bool solid_collide = false;
        for (RigidBody* body : query(temp))
            if (body->solid_ && solid_ && (body->pool_id_ != pool_id_))
                solid_collide = true;

        if (!solid_collide) {
            surface_ = temp;
            break;
        }
        else
            speedNorm -= 0.1;
    }
    if (t(*father_) != t(Bullet)) {
        quad.add({ pool_id_, surface_ });
    }
    speed_.x += acceleration_ * delta * speed_.x;
    speed_.y += acceleration_ * delta * speed_.y;
}

std::vector<RigidBody*> RigidBody::getCollisions(bool with_solid) {
    return query(surface_, with_solid);
}

bool RigidBody::isSolid() {
    return solid_;
}