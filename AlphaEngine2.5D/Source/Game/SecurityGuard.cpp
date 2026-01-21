#include "SecurityGuard.hpp"
#include "Engine/Core/GameObject.hpp"
#include "Engine/Core/Time.hpp"
#include <cmath>
#include <vector>
#include <queue>
#include <cstdlib>
#include <ctime>

namespace Game {

    bool SecurityGuard::seeded = false;

    void SecurityGuard::Initialize()
    {
       //random generateed once used for path ltr
        if (!seeded) {
            std::srand((unsigned)std::time(nullptr));
            seeded = true;
        }
        BuildPathBFS();
    }

    void SecurityGuard::Update()
    {
        if (!tileMap) {
            return;
        }
        if (path.empty() || pathIndex >= (int)path.size()) {
            BuildPathBFS();
            return; //if not path createed or walk finish generate new path
        }
        //Creates a reference to the original value, with the type automatically deduced.
        auto& currentPos = gameObject->transform->position;
        Engine::Vector2 target = path[pathIndex];

        //calculate dist
        Engine::Vector2 distance(target.x - currentPos.x, target.y - currentPos.y);
        float distanceSquare = (distance.x * distance.x) + (distance.y * distance.y);
        if (distanceSquare <= arriveDistance * arriveDistance) {
            pathIndex++; //move to the next tile
            if (pathIndex >= (int)path.size()) {
                BuildPathBFS();
            }
            return;
        }


        float dist = std::sqrt(distanceSquare);
        if (dist <= 0.00001f) return;
        Engine::Vector2 direction(distance.x / dist, distance.y / dist);
        currentPos.x += direction.x * speed * Engine::Time::deltaTime;
        currentPos.y += direction.y * speed * Engine::Time::deltaTime;
        /*
        point A to B simple walking
        if (!transform) return;

        Engine::Vector2 target;
        if (goingtonextPoint) {
            target = pointB;
        }
        else {
            target = pointA;
        }
        Engine::Vector2 delta;
        delta.x = target.x - transform->position.x;
        delta.y = target.y - transform->position.y;
        float distanceSq = delta.x * delta.x + delta.y * delta.y;
        if (distanceSq <= arriveDistance * arriveDistance)
        {
            goingtonextPoint = !goingtonextPoint;
            return;
        }
        float length = std::sqrt(distanceSq);
        Engine::Vector2 dir(delta.x / length, delta.y / length);
        transform->position.x += dir.x * speed * Engine::Time::deltaTime;
        transform->position.y += dir.y * speed * Engine::Time::deltaTime;*/
    }

    /*
        pseudo code
        pick random tile
        breadth first search
        convert bfs search into path
        guard follow it
        when reach pick new dest
        repeat bfs agan


    */
    void SecurityGuard::BuildPathBFS() {
        path.clear();
        pathIndex = 0;
        if (!tileMap) {
            return;
        }
        //convert position 
        int startX = (int)gameObject->transform->position.x;
        int startY = (int)gameObject->transform->position.y;
        //pick random goal tile
        //check if tile walkable
        if (!tileMap->IsWalkable(startX + 0.5f, startY + 0.5f)) {
            return;
        }
        int goalX = startX;
        int goalY = startY;

        //try 300 times;
        for (int tries = 0; tries < 300; ++tries) {
            //randomindex
            int randomX = std::rand() % tileMap->width;
            int randomY = std::rand() % tileMap->height;
            //if can walk then set as goal
            if (tileMap->IsWalkable(randomX + 0.5f, randomY + 0.5f)) {
                goalX = randomX;
                goalY = randomY;
                break;
            }
        }
        //setup bfs
        int start = iD(startX, startY);
        int goal = iD(goalX, goalY);
        //store parent bfs -1 is for unvisited
        std::vector<int> parent(tileMap->width * tileMap->height, -1);
        //bfs queue
        std::queue<int> q;

        parent[start] = start;
        q.push(start);

        //4 direction movement
        /*
        k	dx[k]	dy[k]	Meaning	New tile
        0	1	0	Right	(cx+1, cy)
        1	-1	0	Left	(cx-1, cy)
        2	0	1	Down	(cx, cy+1)
        3	0	-1	Up	    (cx, cy-1)
        */
        const int dX[4] = { 1,-1,0,0 };
        const int dY[4] = { 0,0,1,-1 };
        //bfs loop
        bool found = false;
        while (!q.empty()) {
            int current = q.front();
            q.pop();
            if (current == goal) {
                //stop when reach
                found = true;
                break;
            }

            //convert id to x n y
            int convertX = current % tileMap->width;
            int convertY = current / tileMap->width;

            //explore neighbour
            for (int k = 0; k < 4; ++k) {
                int neighbourX = convertX + dX[k];
                int neighbourY = convertY + dY[k];
                if (neighbourX < 0 || neighbourX >= tileMap->width || neighbourY < 0 || neighbourY >= tileMap->height) {
                    continue;//bounds checl
                }
                if (!tileMap->IsWalkable(neighbourX + 0.5f, neighbourY + 0.5f) ){
                    //if is wall skip
                    continue;
                }
                int neighbourID = iD(neighbourX, neighbourY);
                if (parent[neighbourID] != -1) {
                    continue; //visit alr skip
                }
                parent[neighbourID]= current;
                q.push(neighbourID);

            }

           
        }
        //construct the path
        if (!found) {
            return; //no path stop
        }
        std::vector<int> tiles;
        int current = goal;//backtrack from goal;
        while (current != start) {
            tiles.push_back(current);
            current = parent[current];
        }
        tiles.push_back(start);//collect path goal n start
        //convert to world space
        for (int i = (int)tiles.size() - 1; i >= 0; --i) {
            int id = tiles[i];
            int x = id % tileMap->width;
            int y = id / tileMap->width;

            path.push_back(WorldFromTile(x, y));
        }
    }

}

