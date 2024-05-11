#pragma once

#include "GameObject.h"
#include "GameTimer.h"
#include "Camera.h"

class Scene
{
public:
    Scene();
    ~Scene();

    void Update();
    void KeyInput();

    void InitScene();
    void LoadScene();
    void BuildScene();

    void AddGameObject(GameObject* gameObject);
    void AddGameObjects(GameObject** gameObjects, int numGameObjects);

    void DeleteGameObject(GameObject* gameObject);
    void DeleteGameObjects(GameObject** gameObject, int numGameObject);
    void DeleteAllGameObjects();

private:
    GameObject* mAllGameObjects;

};

