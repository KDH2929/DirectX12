#pragma once

#include "GameObject.h"

class BoxObject : public GameObject {
public:
    BoxObject();
    virtual bool Initialize(Renderer* renderer) override;
    virtual void Update(float deltaTime) override;
    virtual void Render(Renderer* renderer) override;

private:
    std::shared_ptr<Mesh> cubeMesh;
};
