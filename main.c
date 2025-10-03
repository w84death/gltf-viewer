#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define WINDOW_WIDTH 800
#define WINDOW_HEIGHT 600

// Camera control settings
#define CAMERA_MOUSE_SENSITIVITY 0.003f
#define CAMERA_ZOOM_SPEED 0.1f
#define CAMERA_MIN_DISTANCE 1.0f
#define CAMERA_MAX_DISTANCE 100.0f

// Isometric camera settings
#define ISO_CAMERA_ANGLE 45.0f  // Angle in degrees
#define ISO_CAMERA_PAN_SPEED 10.0f
#define ISO_CAMERA_EDGE_SCROLL_ZONE 20  // Pixels from edge
#define ISO_CAMERA_EDGE_SCROLL_SPEED 8.0f
#define ISO_CAMERA_MIN_HEIGHT 5.0f
#define ISO_CAMERA_MAX_HEIGHT 50.0f
#define ISO_CAMERA_ZOOM_SPEED 2.0f
#define ISO_CAMERA_SMOOTHING 0.15f  // Smooth camera movement

// Unit settings
#define MAX_UNITS 100
#define UNIT_SIZE 0.3f
#define UNIT_SPEED 2.0f
#define UNIT_TURN_SPEED 3.0f
#define UNIT_DETECTION_RANGE 5.0f
#define UNIT_AVOIDANCE_DISTANCE 1.5f
#define UNIT_WANDER_RADIUS 10.0f
#define UNIT_HEIGHT_OFFSET 0.2f
#define UNIT_ARRIVAL_DISTANCE 0.5f

// Camera view modes
typedef enum {
    VIEW_MODE_ORBIT,
    VIEW_MODE_ISOMETRIC
} ViewMode;

// Structure to hold camera orbit data
typedef struct {
    float distance;
    float rotationH;  // Horizontal rotation (around Y axis)
    float rotationV;  // Vertical rotation (pitch)
    Vector3 target;
} OrbitCamera;

// Structure to hold isometric camera data
typedef struct {
    Vector3 position;
    Vector3 target;
    float height;
    float angle;  // Fixed angle for isometric view
    Vector3 targetPosition;  // For smooth movement
    Vector3 targetTarget;    // For smooth panning
    // Selection box
    bool selecting;
    Vector2 selectionStart;
    Vector2 selectionEnd;
} IsometricCamera;

// Structure for units
typedef struct {
    Vector3 position;
    Vector3 velocity;
    Vector3 targetPosition;
    Vector3 commandTarget;  // Target set by player command
    bool hasCommand;        // Whether unit has a player command
    float rotation;
    float moveTimer;
    float size;
    Color color;
    bool active;
    bool selected;
    int groupId;  // Control group ID (0 = no group, 1-9 = groups)
} Unit;

// Command marker structure
typedef struct {
    Vector3 position;
    float timer;
    bool active;
} CommandMarker;

// Control group structure
typedef struct {
    int unitIndices[MAX_UNITS];
    int unitCount;
    bool active;
} ControlGroup;

// Global unit array and command marker
Unit units[MAX_UNITS];
int unitCount = 0;
CommandMarker commandMarker = {0};

// Control groups (1-9)
ControlGroup controlGroups[10] = {0};  // Index 0 unused, 1-9 for groups

// Function to initialize a unit
void InitUnit(Unit* unit, Vector3 position) {
    unit->position = position;
    unit->velocity = (Vector3){0, 0, 0};
    unit->targetPosition = position;
    unit->commandTarget = position;
    unit->hasCommand = false;
    unit->rotation = GetRandomValue(0, 360) * DEG2RAD;
    unit->moveTimer = 0;
    unit->size = UNIT_SIZE;
    unit->color = WHITE;
    unit->active = true;
    unit->selected = false;
    unit->groupId = 0;
}

// Function to spawn a new unit at a random position
void SpawnUnit(Vector3 centerPos, float radius) {
    if (unitCount >= MAX_UNITS) return;
    
    float angle = GetRandomValue(0, 360) * DEG2RAD;
    float distance = (float)GetRandomValue(10, (int)(radius * 10)) / 10.0f;
    
    Vector3 spawnPos = {
        centerPos.x + cosf(angle) * distance,
        centerPos.y + UNIT_HEIGHT_OFFSET,
        centerPos.z + sinf(angle) * distance
    };
    
    InitUnit(&units[unitCount], spawnPos);
    unitCount++;
}

// Function to perform raycast collision check with actual mesh triangles
bool CheckCollisionWithModel(Vector3 origin, Vector3 direction, Model model, float maxDistance) {
    Ray ray = { origin, direction };
    
    for (int i = 0; i < model.meshCount; i++) {
        Mesh mesh = model.meshes[i];
        
        // First check bounding box for optimization
        BoundingBox bounds = GetMeshBoundingBox(mesh);
        RayCollision boxCollision = GetRayCollisionBox(ray, bounds);
        
        if (boxCollision.hit && boxCollision.distance < maxDistance) {
            // If bounding box is hit, check actual triangles
            Matrix transform = model.transform;
            
            // Check if mesh has indices
            if (mesh.indices != NULL) {
                // Indexed mesh
                for (int t = 0; t < mesh.triangleCount; t++) {
                    Vector3 v0, v1, v2;
                    
                    // Get triangle vertices
                    int idx0 = mesh.indices[t * 3 + 0];
                    int idx1 = mesh.indices[t * 3 + 1];
                    int idx2 = mesh.indices[t * 3 + 2];
                    
                    v0.x = mesh.vertices[idx0 * 3 + 0];
                    v0.y = mesh.vertices[idx0 * 3 + 1];
                    v0.z = mesh.vertices[idx0 * 3 + 2];
                    
                    v1.x = mesh.vertices[idx1 * 3 + 0];
                    v1.y = mesh.vertices[idx1 * 3 + 1];
                    v1.z = mesh.vertices[idx1 * 3 + 2];
                    
                    v2.x = mesh.vertices[idx2 * 3 + 0];
                    v2.y = mesh.vertices[idx2 * 3 + 1];
                    v2.z = mesh.vertices[idx2 * 3 + 2];
                    
                    // Transform vertices
                    v0 = Vector3Transform(v0, transform);
                    v1 = Vector3Transform(v1, transform);
                    v2 = Vector3Transform(v2, transform);
                    
                    // Check collision with triangle
                    RayCollision triCollision = GetRayCollisionTriangle(ray, v0, v1, v2);
                    if (triCollision.hit && triCollision.distance < maxDistance) {
                        return true;
                    }
                }
            } else {
                // Non-indexed mesh
                for (int t = 0; t < mesh.triangleCount; t++) {
                    Vector3 v0, v1, v2;
                    
                    v0.x = mesh.vertices[(t * 3 + 0) * 3 + 0];
                    v0.y = mesh.vertices[(t * 3 + 0) * 3 + 1];
                    v0.z = mesh.vertices[(t * 3 + 0) * 3 + 2];
                    
                    v1.x = mesh.vertices[(t * 3 + 1) * 3 + 0];
                    v1.y = mesh.vertices[(t * 3 + 1) * 3 + 1];
                    v1.z = mesh.vertices[(t * 3 + 1) * 3 + 2];
                    
                    v2.x = mesh.vertices[(t * 3 + 2) * 3 + 0];
                    v2.y = mesh.vertices[(t * 3 + 2) * 3 + 1];
                    v2.z = mesh.vertices[(t * 3 + 2) * 3 + 2];
                    
                    // Transform vertices
                    v0 = Vector3Transform(v0, transform);
                    v1 = Vector3Transform(v1, transform);
                    v2 = Vector3Transform(v2, transform);
                    
                    // Check collision with triangle
                    RayCollision triCollision = GetRayCollisionTriangle(ray, v0, v1, v2);
                    if (triCollision.hit && triCollision.distance < maxDistance) {
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}

// Function to get ground height at position (for terrain following)
float GetGroundHeight(Vector3 position, Model model) {
    // Cast ray downward from above the position
    Ray ray = { 
        (Vector3){position.x, position.y + 10.0f, position.z}, 
        (Vector3){0, -1, 0} 
    };
    
    float closestDistance = 10000.0f;
    float groundY = 0.0f;
    
    for (int i = 0; i < model.meshCount; i++) {
        Mesh mesh = model.meshes[i];
        
        // Check bounding box first
        BoundingBox bounds = GetMeshBoundingBox(mesh);
        RayCollision boxCollision = GetRayCollisionBox(ray, bounds);
        
        if (boxCollision.hit) {
            Matrix transform = model.transform;
            
            // Check triangles
            if (mesh.indices != NULL) {
                for (int t = 0; t < mesh.triangleCount; t++) {
                    Vector3 v0, v1, v2;
                    
                    int idx0 = mesh.indices[t * 3 + 0];
                    int idx1 = mesh.indices[t * 3 + 1];
                    int idx2 = mesh.indices[t * 3 + 2];
                    
                    v0.x = mesh.vertices[idx0 * 3 + 0];
                    v0.y = mesh.vertices[idx0 * 3 + 1];
                    v0.z = mesh.vertices[idx0 * 3 + 2];
                    
                    v1.x = mesh.vertices[idx1 * 3 + 0];
                    v1.y = mesh.vertices[idx1 * 3 + 1];
                    v1.z = mesh.vertices[idx1 * 3 + 2];
                    
                    v2.x = mesh.vertices[idx2 * 3 + 0];
                    v2.y = mesh.vertices[idx2 * 3 + 1];
                    v2.z = mesh.vertices[idx2 * 3 + 2];
                    
                    v0 = Vector3Transform(v0, transform);
                    v1 = Vector3Transform(v1, transform);
                    v2 = Vector3Transform(v2, transform);
                    
                    RayCollision triCollision = GetRayCollisionTriangle(ray, v0, v1, v2);
                    if (triCollision.hit && triCollision.distance < closestDistance) {
                        closestDistance = triCollision.distance;
                        groundY = triCollision.point.y;
                    }
                }
            } else {
                for (int t = 0; t < mesh.triangleCount; t++) {
                    Vector3 v0, v1, v2;
                    
                    v0.x = mesh.vertices[(t * 3 + 0) * 3 + 0];
                    v0.y = mesh.vertices[(t * 3 + 0) * 3 + 1];
                    v0.z = mesh.vertices[(t * 3 + 0) * 3 + 2];
                    
                    v1.x = mesh.vertices[(t * 3 + 1) * 3 + 0];
                    v1.y = mesh.vertices[(t * 3 + 1) * 3 + 1];
                    v1.z = mesh.vertices[(t * 3 + 1) * 3 + 2];
                    
                    v2.x = mesh.vertices[(t * 3 + 2) * 3 + 0];
                    v2.y = mesh.vertices[(t * 3 + 2) * 3 + 1];
                    v2.z = mesh.vertices[(t * 3 + 2) * 3 + 2];
                    
                    v0 = Vector3Transform(v0, transform);
                    v1 = Vector3Transform(v1, transform);
                    v2 = Vector3Transform(v2, transform);
                    
                    RayCollision triCollision = GetRayCollisionTriangle(ray, v0, v1, v2);
                    if (triCollision.hit && triCollision.distance < closestDistance) {
                        closestDistance = triCollision.distance;
                        groundY = triCollision.point.y;
                    }
                }
            }
        }
    }
    
    // If no ground found, return default height
    if (closestDistance >= 10000.0f) {
        return UNIT_HEIGHT_OFFSET;
    }
    
    return groundY + UNIT_HEIGHT_OFFSET;
}

// Function to get ground position from screen coordinates (terrain-aware)
Vector3 GetGroundPositionFromMouse(Vector2 mousePos, Camera3D camera, Model model) {
    // Create a ray from the camera through the mouse position
    Ray ray = GetMouseRay(mousePos, camera);
    
    float closestDistance = 10000.0f;
    Vector3 hitPoint = {0, 0, 0};
    bool hitFound = false;
    
    // Check for collision with model meshes
    for (int i = 0; i < model.meshCount; i++) {
        Mesh mesh = model.meshes[i];
        
        // Check bounding box first
        BoundingBox bounds = GetMeshBoundingBox(mesh);
        RayCollision boxCollision = GetRayCollisionBox(ray, bounds);
        
        if (boxCollision.hit) {
            Matrix transform = model.transform;
            
            // Check triangles
            if (mesh.indices != NULL) {
                for (int t = 0; t < mesh.triangleCount; t++) {
                    Vector3 v0, v1, v2;
                    
                    int idx0 = mesh.indices[t * 3 + 0];
                    int idx1 = mesh.indices[t * 3 + 1];
                    int idx2 = mesh.indices[t * 3 + 2];
                    
                    v0.x = mesh.vertices[idx0 * 3 + 0];
                    v0.y = mesh.vertices[idx0 * 3 + 1];
                    v0.z = mesh.vertices[idx0 * 3 + 2];
                    
                    v1.x = mesh.vertices[idx1 * 3 + 0];
                    v1.y = mesh.vertices[idx1 * 3 + 1];
                    v1.z = mesh.vertices[idx1 * 3 + 2];
                    
                    v2.x = mesh.vertices[idx2 * 3 + 0];
                    v2.y = mesh.vertices[idx2 * 3 + 1];
                    v2.z = mesh.vertices[idx2 * 3 + 2];
                    
                    v0 = Vector3Transform(v0, transform);
                    v1 = Vector3Transform(v1, transform);
                    v2 = Vector3Transform(v2, transform);
                    
                    RayCollision triCollision = GetRayCollisionTriangle(ray, v0, v1, v2);
                    if (triCollision.hit && triCollision.distance < closestDistance) {
                        closestDistance = triCollision.distance;
                        hitPoint = triCollision.point;
                        hitFound = true;
                    }
                }
            }
        }
    }
    
    // If hit terrain/model, return that position
    if (hitFound) {
        hitPoint.y += UNIT_HEIGHT_OFFSET;
        return hitPoint;
    }
    
    // Otherwise, intersect with ground plane (y = 0)
    float t = -ray.position.y / ray.direction.y;
    if (t > 0) {
        Vector3 groundPos = {
            ray.position.x + ray.direction.x * t,
            UNIT_HEIGHT_OFFSET,
            ray.position.z + ray.direction.z * t
        };
        return groundPos;
    }
    
    // Default fallback
    return (Vector3){0, UNIT_HEIGHT_OFFSET, 0};
}

// Function to command selected units to a position
void CommandUnitsToPosition(Vector3 targetPos, Model model) {
    int selectedCount = 0;
    
    // Count selected units
    for (int i = 0; i < unitCount; i++) {
        if (units[i].active && units[i].selected) {
            selectedCount++;
        }
    }
    
    if (selectedCount == 0) return;
    
    // Calculate formation positions (simple grid formation)
    int cols = (int)sqrtf((float)selectedCount);
    if (cols == 0) cols = 1;
    float spacing = UNIT_SIZE * 2.5f;
    
    int currentUnit = 0;
    for (int i = 0; i < unitCount; i++) {
        if (units[i].active && units[i].selected) {
            int row = currentUnit / cols;
            int col = currentUnit % cols;
            
            Vector3 formationOffset = {
                (col - cols/2.0f) * spacing,
                0,
                (row - selectedCount/cols/2.0f) * spacing
            };
            
            Vector3 finalTarget = Vector3Add(targetPos, formationOffset);
            // Adjust height based on terrain
            finalTarget.y = GetGroundHeight(finalTarget, model);
            
            units[i].commandTarget = finalTarget;
            units[i].hasCommand = true;
            units[i].moveTimer = 0;  // Reset wander timer
            
            currentUnit++;
        }
    }
    
    // Set command marker
    commandMarker.position = targetPos;
    commandMarker.timer = 1.0f;
    commandMarker.active = true;
}

// Function to update unit movement
void UpdateUnit(Unit* unit, Model model, float deltaTime) {
    if (!unit->active) return;
    
    Vector3 actualTarget;
    
    // Determine which target to use
    if (unit->hasCommand) {
        actualTarget = unit->commandTarget;
        
        // Check if arrived at command target
        float distToCommand = Vector3Distance(unit->position, unit->commandTarget);
        if (distToCommand < UNIT_ARRIVAL_DISTANCE) {
            unit->hasCommand = false;  // Command completed
            unit->moveTimer = 0;  // Start wandering again
        }
    } else {
        // Wander behavior when no command
        unit->moveTimer -= deltaTime;
        
        if (unit->moveTimer <= 0) {
            float angle = GetRandomValue(0, 360) * DEG2RAD;
            float distance = GetRandomValue(2, 8);
            
            unit->targetPosition = (Vector3){
                unit->position.x + cosf(angle) * distance,
                unit->position.y,
                unit->position.z + sinf(angle) * distance
            };
            
            unit->moveTimer = GetRandomValue(20, 50) / 10.0f; // 2-5 seconds
        }
        
        actualTarget = unit->targetPosition;
    }
    
    // Calculate direction to target
    Vector3 direction = Vector3Subtract(actualTarget, unit->position);
    float distanceToTarget = Vector3Length(direction);
    
    if (distanceToTarget > 0.1f) {
        direction = Vector3Normalize(direction);
        
        // Check for collision in movement direction
        bool willCollide = CheckCollisionWithModel(unit->position, direction, model, UNIT_AVOIDANCE_DISTANCE);
        
        // Check collision with other units
        for (int i = 0; i < unitCount; i++) {
            if (&units[i] != unit && units[i].active) {
                float dist = Vector3Distance(unit->position, units[i].position);
                if (dist < UNIT_SIZE * 3) {
                    willCollide = true;
                    break;
                }
            }
        }
        
        if (willCollide) {
            // Try to find alternative direction
            float avoidanceAngle = GetRandomValue(-90, 90) * DEG2RAD;
            float currentAngle = atan2f(direction.z, direction.x);
            float newAngle = currentAngle + avoidanceAngle;
            
            direction = (Vector3){
                cosf(newAngle),
                0,
                sinf(newAngle)
            };
            
            // If unit has a command, try to maintain progress toward it
            if (unit->hasCommand) {
                // Don't change the command target, just navigate around obstacle
                Vector3 movement = Vector3Scale(direction, UNIT_SPEED * deltaTime * 0.5f);
                unit->position = Vector3Add(unit->position, movement);
            } else {
                // Set new random target for wandering
                unit->targetPosition = Vector3Add(unit->position, Vector3Scale(direction, 3.0f));
                unit->moveTimer = 1.0f;
            }
        } else {
            // Move towards target
            Vector3 movement = Vector3Scale(direction, UNIT_SPEED * deltaTime);
            unit->position = Vector3Add(unit->position, movement);
            
            // Update rotation to face movement direction
            float targetRotation = atan2f(direction.z, direction.x);
            float rotationDiff = targetRotation - unit->rotation;
            
            // Normalize rotation difference
            while (rotationDiff > PI) rotationDiff -= 2 * PI;
            while (rotationDiff < -PI) rotationDiff += 2 * PI;
            
            unit->rotation += rotationDiff * UNIT_TURN_SPEED * deltaTime;
        }
    }
    
    // Keep units on ground level (terrain-aware)
    float groundHeight = GetGroundHeight(unit->position, model);
    unit->position.y = groundHeight;
}

// Function to draw a unit
void DrawUnit(Unit* unit, Camera3D camera) {
    if (!unit->active) return;
    
    // Change color based on state
    Color unitColor = unit->color;
    if (unit->selected) {
        unitColor = unit->hasCommand ? SKYBLUE : LIME;
    }
    
    // Draw cube body
    DrawCube(unit->position, unit->size, unit->size, unit->size, unitColor);
    DrawCubeWires(unit->position, unit->size, unit->size, unit->size, BLACK);
    
    // Draw direction indicator
    Vector3 front = {
        unit->position.x + cosf(unit->rotation) * unit->size,
        unit->position.y,
        unit->position.z + sinf(unit->rotation) * unit->size
    };
    DrawLine3D(unit->position, front, RED);
    
    // Draw selection indicator
    if (unit->selected) {
        DrawCubeWires(unit->position, unit->size * 1.5f, unit->size * 1.5f, unit->size * 1.5f, GREEN);
    }
    
    // Draw group number above unit
    if (unit->groupId > 0) {
        Vector2 screenPos = GetWorldToScreen(
            (Vector3){unit->position.x, unit->position.y + unit->size, unit->position.z}, 
            camera
        );
        DrawText(TextFormat("%d", unit->groupId), screenPos.x - 5, screenPos.y - 10, 10, YELLOW);
    }
    
    // Draw command target line for selected units
    if (unit->selected && unit->hasCommand) {
        DrawLine3D(unit->position, unit->commandTarget, Fade(GREEN, 0.3f));
    }
}

// Function to draw command marker
void DrawCommandMarker(float deltaTime) {
    if (!commandMarker.active) return;
    
    commandMarker.timer -= deltaTime;
    if (commandMarker.timer <= 0) {
        commandMarker.active = false;
        return;
    }
    
    float alpha = commandMarker.timer;
    float scale = 1.0f + (1.0f - commandMarker.timer) * 2.0f;
    
    // Draw expanding circle on ground
    DrawCircle3D(commandMarker.position, scale * 0.5f, (Vector3){1, 0, 0}, 90.0f, Fade(GREEN, alpha * 0.5f));
    
    // Draw vertical beam
    Vector3 beamTop = commandMarker.position;
    beamTop.y += 2.0f * alpha;
    DrawLine3D(commandMarker.position, beamTop, Fade(GREEN, alpha));
}

// Function to select units within a rectangle
void SelectUnitsInBox(Vector2 start, Vector2 end, Camera3D camera) {
    // Deselect all units first
    for (int i = 0; i < unitCount; i++) {
        units[i].selected = false;
    }
    
    // Get screen space bounding box
    float minX = fminf(start.x, end.x);
    float maxX = fmaxf(start.x, end.x);
    float minY = fminf(start.y, end.y);
    float maxY = fmaxf(start.y, end.y);
    
    // Check each unit
    for (int i = 0; i < unitCount; i++) {
        if (!units[i].active) continue;
        
        // Project unit position to screen space
        Vector2 screenPos = GetWorldToScreen(units[i].position, camera);
        
        // Check if within selection box
        if (screenPos.x >= minX && screenPos.x <= maxX &&
            screenPos.y >= minY && screenPos.y <= maxY) {
            units[i].selected = true;
        }
    }
}

// Function to assign selected units to a control group
void AssignControlGroup(int groupNum) {
    if (groupNum < 1 || groupNum > 9) return;
    
    // Clear old group assignments for selected units
    for (int i = 0; i < unitCount; i++) {
        if (units[i].active && units[i].selected) {
            // Remove from old group if any
            if (units[i].groupId > 0) {
                ControlGroup* oldGroup = &controlGroups[units[i].groupId];
                for (int j = 0; j < oldGroup->unitCount; j++) {
                    if (oldGroup->unitIndices[j] == i) {
                        // Shift remaining units
                        for (int k = j; k < oldGroup->unitCount - 1; k++) {
                            oldGroup->unitIndices[k] = oldGroup->unitIndices[k + 1];
                        }
                        oldGroup->unitCount--;
                        break;
                    }
                }
            }
            units[i].groupId = groupNum;
        }
    }
    
    // Build new group
    ControlGroup* group = &controlGroups[groupNum];
    group->unitCount = 0;
    group->active = true;
    
    for (int i = 0; i < unitCount; i++) {
        if (units[i].active && units[i].selected) {
            group->unitIndices[group->unitCount++] = i;
        }
    }
}

// Function to select units in a control group and center camera
Vector3 SelectControlGroup(int groupNum) {
    if (groupNum < 1 || groupNum > 9) return (Vector3){0, 0, 0};
    
    ControlGroup* group = &controlGroups[groupNum];
    if (!group->active || group->unitCount == 0) return (Vector3){0, 0, 0};
    
    // Deselect all units first
    for (int i = 0; i < unitCount; i++) {
        units[i].selected = false;
    }
    
    // Select units in group and calculate center
    Vector3 center = {0, 0, 0};
    int validCount = 0;
    
    for (int i = 0; i < group->unitCount; i++) {
        int unitIdx = group->unitIndices[i];
        if (unitIdx < unitCount && units[unitIdx].active) {
            units[unitIdx].selected = true;
            center.x += units[unitIdx].position.x;
            center.y += units[unitIdx].position.y;
            center.z += units[unitIdx].position.z;
            validCount++;
        }
    }
    
    if (validCount > 0) {
        center.x /= validCount;
        center.y /= validCount;
        center.z /= validCount;
        return center;
    }
    
    return (Vector3){0, 0, 0};
}

// Function to update orbit camera based on input
void UpdateOrbitCamera(Camera3D *camera, OrbitCamera *orbit) {
    // Mouse controls for rotation
    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 mouseDelta = GetMouseDelta();
        orbit->rotationH -= mouseDelta.x * CAMERA_MOUSE_SENSITIVITY;
        orbit->rotationV += mouseDelta.y * CAMERA_MOUSE_SENSITIVITY;
        
        // Clamp vertical rotation to avoid flipping
        if (orbit->rotationV > PI/2.0f - 0.1f) orbit->rotationV = PI/2.0f - 0.1f;
        if (orbit->rotationV < -PI/2.0f + 0.1f) orbit->rotationV = -PI/2.0f + 0.1f;
    }
    
    // Mouse wheel for zoom
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0) {
        orbit->distance -= wheelMove * orbit->distance * CAMERA_ZOOM_SPEED;
        if (orbit->distance < CAMERA_MIN_DISTANCE) orbit->distance = CAMERA_MIN_DISTANCE;
        if (orbit->distance > CAMERA_MAX_DISTANCE) orbit->distance = CAMERA_MAX_DISTANCE;
    }
    
    // Middle mouse button for panning
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 mouseDelta = GetMouseDelta();
        
        // Calculate right and up vectors based on camera orientation
        float cosH = cosf(orbit->rotationH);
        float sinH = sinf(orbit->rotationH);
        
        // Pan the target
        orbit->target.x += (mouseDelta.x * cosH - mouseDelta.y * 0) * orbit->distance * 0.001f;
        orbit->target.z += (mouseDelta.x * sinH) * orbit->distance * 0.001f;
        orbit->target.y += mouseDelta.y * orbit->distance * 0.001f;
    }
    
    // Reset camera with R key
    if (IsKeyPressed(KEY_R)) {
        orbit->distance = 5.0f;
        orbit->rotationH = PI * 0.25f;
        orbit->rotationV = PI * 0.15f;
        orbit->target = (Vector3){0.0f, 0.0f, 0.0f};
    }
    
    // Calculate camera position from orbit parameters
    float cosV = cosf(orbit->rotationV);
    float sinV = sinf(orbit->rotationV);
    float cosH = cosf(orbit->rotationH);
    float sinH = sinf(orbit->rotationH);
    
    camera->position.x = orbit->target.x + orbit->distance * cosV * sinH;
    camera->position.y = orbit->target.y + orbit->distance * sinV;
    camera->position.z = orbit->target.z + orbit->distance * cosV * cosH;
    
    camera->target = orbit->target;
}

// Function to update isometric camera
void UpdateIsometricCamera(Camera3D *camera, IsometricCamera *iso) {
    float deltaTime = GetFrameTime();
    Vector2 mousePos = GetMousePosition();
    
    // Calculate movement direction
    float moveX = 0.0f;
    float moveZ = 0.0f;
    
    // Keyboard controls (WASD and arrow keys)
    if (IsKeyDown(KEY_W) || IsKeyDown(KEY_UP)) moveZ -= 1.0f;
    if (IsKeyDown(KEY_S) || IsKeyDown(KEY_DOWN)) moveZ += 1.0f;
    if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT)) moveX -= 1.0f;
    if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) moveX += 1.0f;
    
    // Edge scrolling
    if (mousePos.x < ISO_CAMERA_EDGE_SCROLL_ZONE) moveX -= 1.0f;
    if (mousePos.x > WINDOW_WIDTH - ISO_CAMERA_EDGE_SCROLL_ZONE) moveX += 1.0f;
    if (mousePos.y < ISO_CAMERA_EDGE_SCROLL_ZONE) moveZ -= 1.0f;
    if (mousePos.y > WINDOW_HEIGHT - ISO_CAMERA_EDGE_SCROLL_ZONE) moveZ += 1.0f;
    
    // Apply movement with speed adjusted by zoom level
    float moveSpeed = ISO_CAMERA_PAN_SPEED * (iso->height / 20.0f);
    iso->targetPosition.x += moveX * moveSpeed * deltaTime;
    iso->targetPosition.z += moveZ * moveSpeed * deltaTime;
    iso->targetTarget.x += moveX * moveSpeed * deltaTime;
    iso->targetTarget.z += moveZ * moveSpeed * deltaTime;
    
    // Middle mouse button drag for panning
    if (IsMouseButtonDown(MOUSE_BUTTON_MIDDLE)) {
        Vector2 mouseDelta = GetMouseDelta();
        float panSpeed = iso->height * 0.002f;
        
        iso->targetPosition.x -= mouseDelta.x * panSpeed;
        iso->targetPosition.z -= mouseDelta.y * panSpeed;
        iso->targetTarget.x -= mouseDelta.x * panSpeed;
        iso->targetTarget.z -= mouseDelta.y * panSpeed;
    }
    
    // Mouse wheel zoom
    float wheelMove = GetMouseWheelMove();
    if (wheelMove != 0) {
        iso->height -= wheelMove * ISO_CAMERA_ZOOM_SPEED;
        iso->height = fmaxf(ISO_CAMERA_MIN_HEIGHT, fminf(ISO_CAMERA_MAX_HEIGHT, iso->height));
    }
    
    // Selection box handling
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        iso->selecting = true;
        iso->selectionStart = mousePos;
        iso->selectionEnd = mousePos;
    }
    
    if (iso->selecting) {
        iso->selectionEnd = mousePos;
        
        if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
            iso->selecting = false;
            // Handle selection for units
            SelectUnitsInBox(iso->selectionStart, iso->selectionEnd, *camera);
        }
    }
    
    // Right click to command units  
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        // Need to access model through isometric camera's parent context
        // For now, we'll need to pass model as parameter
        // This is handled in the main loop
    }
    
    // Reset camera with R key
    if (IsKeyPressed(KEY_R)) {
        iso->position = (Vector3){0.0f, 15.0f, 10.0f};
        iso->target = (Vector3){0.0f, 0.0f, 0.0f};
        iso->targetPosition = iso->position;
        iso->targetTarget = iso->target;
        iso->height = 15.0f;
    }
    
    // Smooth camera movement
    iso->position.x += (iso->targetPosition.x - iso->position.x) * ISO_CAMERA_SMOOTHING;
    iso->position.z += (iso->targetPosition.z - iso->position.z) * ISO_CAMERA_SMOOTHING;
    iso->target.x += (iso->targetTarget.x - iso->target.x) * ISO_CAMERA_SMOOTHING;
    iso->target.z += (iso->targetTarget.z - iso->target.z) * ISO_CAMERA_SMOOTHING;
    
    // Set camera position relative to target for isometric view
    camera->position.x = iso->target.x;
    camera->position.y = iso->target.y + iso->height;
    camera->position.z = iso->target.z + iso->height * 0.8f;  // Offset for isometric view
    
    camera->target = iso->target;
}

// Function to draw selection box
void DrawSelectionBox(IsometricCamera *iso) {
    if (!iso->selecting) return;
    
    float x = fminf(iso->selectionStart.x, iso->selectionEnd.x);
    float y = fminf(iso->selectionStart.y, iso->selectionEnd.y);
    float width = fabsf(iso->selectionEnd.x - iso->selectionStart.x);
    float height = fabsf(iso->selectionEnd.y - iso->selectionStart.y);
    
    DrawRectangleLinesEx((Rectangle){x, y, width, height}, 2.0f, GREEN);
    DrawRectangle(x, y, width, height, Fade(GREEN, 0.1f));
}

// Function to calculate model bounds
BoundingBox GetModelBounds(Model model) {
    BoundingBox bounds = {0};
    
    if (model.meshCount > 0) {
        bounds = GetMeshBoundingBox(model.meshes[0]);
        
        for (int i = 1; i < model.meshCount; i++) {
            BoundingBox meshBounds = GetMeshBoundingBox(model.meshes[i]);
            
            bounds.min.x = fminf(bounds.min.x, meshBounds.min.x);
            bounds.min.y = fminf(bounds.min.y, meshBounds.min.y);
            bounds.min.z = fminf(bounds.min.z, meshBounds.min.z);
            
            bounds.max.x = fmaxf(bounds.max.x, meshBounds.max.x);
            bounds.max.y = fmaxf(bounds.max.y, meshBounds.max.y);
            bounds.max.z = fmaxf(bounds.max.z, meshBounds.max.z);
        }
    }
    
    return bounds;
}

int main(int argc, char *argv[]) {
    // Get model filename from command line or use default
    const char *modelPath = "ibm-pc.glb";
    if (argc > 1) {
        modelPath = argv[1];
    }
    
    // Initialize window
    InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "GLTF Viewer - Strategy Camera");
    
    // Initialize camera
    Camera3D camera = {0};
    camera.position = (Vector3){5.0f, 5.0f, 5.0f};
    camera.target = (Vector3){0.0f, 0.0f, 0.0f};
    camera.up = (Vector3){0.0f, 1.0f, 0.0f};
    camera.fovy = 45.0f;
    camera.projection = CAMERA_PERSPECTIVE;
    
    // Camera view mode
    ViewMode viewMode = VIEW_MODE_ORBIT;
    
    // Initialize orbit camera controller
    OrbitCamera orbit = {0};
    orbit.distance = 5.0f;
    orbit.rotationH = PI * 0.25f;
    orbit.rotationV = PI * 0.15f;
    orbit.target = (Vector3){0.0f, 0.0f, 0.0f};
    
    // Initialize isometric camera controller
    IsometricCamera isometric = {0};
    isometric.position = (Vector3){0.0f, 15.0f, 10.0f};
    isometric.target = (Vector3){0.0f, 0.0f, 0.0f};
    isometric.targetPosition = isometric.position;
    isometric.targetTarget = isometric.target;
    isometric.height = 15.0f;
    isometric.angle = ISO_CAMERA_ANGLE;
    isometric.selecting = false;
    
    // Load model
    Model model = LoadModel(modelPath);
    
    // Check if model loaded successfully
    if (model.meshCount == 0) {
        printf("Failed to load model: %s\n", modelPath);
        CloseWindow();
        return 1;
    }
    
    // Ensure model transform is identity matrix for proper rendering
    model.transform = MatrixIdentity();
    
    // Get model bounds and center camera target
    BoundingBox bounds = GetModelBounds(model);
    Vector3 modelCenter = {
        (bounds.min.x + bounds.max.x) / 2.0f,
        (bounds.min.y + bounds.max.y) / 2.0f,
        (bounds.min.z + bounds.max.z) / 2.0f
    };
    
    Vector3 modelSize = {
        bounds.max.x - bounds.min.x,
        bounds.max.y - bounds.min.y,
        bounds.max.z - bounds.min.z
    };
    
    float maxDimension = fmaxf(modelSize.x, fmaxf(modelSize.y, modelSize.z));
    
    // Adjust camera distance based on model size
    orbit.target = modelCenter;
    orbit.distance = maxDimension * 2.0f;
    if (orbit.distance < CAMERA_MIN_DISTANCE) orbit.distance = CAMERA_MIN_DISTANCE;
    if (orbit.distance > CAMERA_MAX_DISTANCE) orbit.distance = CAMERA_MAX_DISTANCE;
    
    // Set isometric camera initial position
    isometric.target = modelCenter;
    isometric.targetTarget = modelCenter;
    isometric.height = maxDimension * 1.5f;
    if (isometric.height < ISO_CAMERA_MIN_HEIGHT) isometric.height = ISO_CAMERA_MIN_HEIGHT;
    if (isometric.height > ISO_CAMERA_MAX_HEIGHT) isometric.height = ISO_CAMERA_MAX_HEIGHT;
    
    // Model information
    int totalTriangles = 0;
    int totalVertices = 0;
    for (int i = 0; i < model.meshCount; i++) {
        totalTriangles += model.meshes[i].triangleCount;
        totalVertices += model.meshes[i].vertexCount;
    }
    
    // Initialize units array and command marker
    for (int i = 0; i < MAX_UNITS; i++) {
        units[i].active = false;
    }
    unitCount = 0;
    commandMarker.active = false;
    
    // Display settings
    bool showInfo = true;
    bool showGrid = true;
    bool showAxes = true;
    bool showUnits = true;
    
    SetTargetFPS(60);
    
    // Main loop
    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();
        
        // Update
        
        // Switch camera view mode with TAB
        if (IsKeyPressed(KEY_TAB)) {
            viewMode = (viewMode == VIEW_MODE_ORBIT) ? VIEW_MODE_ISOMETRIC : VIEW_MODE_ORBIT;
        }
        
        // Control group management
        bool ctrlPressed = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        
        for (int i = 1; i <= 9; i++) {
            if (IsKeyPressed(KEY_ONE + i - 1)) {
                if (ctrlPressed) {
                    // Assign selected units to control group
                    AssignControlGroup(i);
                } else {
                    // Select control group and center camera
                    Vector3 groupCenter = SelectControlGroup(i);
                    if (groupCenter.x != 0 || groupCenter.y != 0 || groupCenter.z != 0) {
                        // Center camera on group
                        if (viewMode == VIEW_MODE_ORBIT) {
                            orbit.target = groupCenter;
                        } else {
                            isometric.target = groupCenter;
                            isometric.targetTarget = groupCenter;
                            // Adjust camera position to follow target
                            isometric.targetPosition.x = groupCenter.x;
                            isometric.targetPosition.z = groupCenter.z + isometric.height * 0.8f;
                        }
                    }
                }
            }
        }
        
        // Update camera based on view mode
        if (viewMode == VIEW_MODE_ORBIT) {
            UpdateOrbitCamera(&camera, &orbit);
            
            // Right click to command units in orbit mode too
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                Vector3 targetPos = GetGroundPositionFromMouse(GetMousePosition(), camera, model);
                CommandUnitsToPosition(targetPos, model);
            }
        } else {
            UpdateIsometricCamera(&camera, &isometric);
            
            // Handle right click command in isometric mode
            if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
                Vector3 targetPos = GetGroundPositionFromMouse(GetMousePosition(), camera, model);
                CommandUnitsToPosition(targetPos, model);
            }
        }
        
        // Spawn units with SPACE key
        if (IsKeyPressed(KEY_SPACE)) {
            for (int i = 0; i < 5; i++) {
                SpawnUnit(modelCenter, maxDimension * 2.0f);
            }
        }
        
        // Clear all units with C key
        if (IsKeyPressed(KEY_C)) {
            unitCount = 0;
            for (int i = 0; i < MAX_UNITS; i++) {
                units[i].active = false;
            }
        }
        
        // Delete selected units with DELETE key
        if (IsKeyPressed(KEY_DELETE)) {
            for (int i = 0; i < unitCount; i++) {
                if (units[i].selected) {
                    units[i].active = false;
                }
            }
        }
        
        // Update all active units
        if (showUnits) {
            for (int i = 0; i < unitCount; i++) {
                if (units[i].active) {
                    UpdateUnit(&units[i], model, deltaTime);
                }
            }
        }
        
        // Toggle displays
        if (IsKeyPressed(KEY_I)) showInfo = !showInfo;
        if (IsKeyPressed(KEY_G)) showGrid = !showGrid;
        if (IsKeyPressed(KEY_X)) showAxes = !showAxes;
        if (IsKeyPressed(KEY_U)) showUnits = !showUnits;
        
        // Draw
        BeginDrawing();
            ClearBackground((Color){48, 48, 56, 255});
            
            BeginMode3D(camera);
                // Draw grid
                if (showGrid) {
                    DrawGrid(30, 1.0f);
                }
                
                // Draw model
                DrawModelEx(model, (Vector3){0, 0, 0}, (Vector3){0, 1, 0}, 0.0f, (Vector3){1, 1, 1}, WHITE);
                
                // Draw units
                if (showUnits) {
                    for (int i = 0; i < unitCount; i++) {
                        if (units[i].active) {
                            DrawUnit(&units[i], camera);
                        }
                    }
                }
                
                // Draw command marker
                DrawCommandMarker(deltaTime);
                
                // Draw coordinate axes
                if (showAxes) {
                    DrawLine3D((Vector3){0, 0, 0}, (Vector3){2, 0, 0}, RED);    // X axis
                    DrawLine3D((Vector3){0, 0, 0}, (Vector3){0, 2, 0}, GREEN);  // Y axis
                    DrawLine3D((Vector3){0, 0, 0}, (Vector3){0, 0, 2}, BLUE);   // Z axis
                }
                
            EndMode3D();
            
            // Draw selection box for isometric mode
            if (viewMode == VIEW_MODE_ISOMETRIC) {
                DrawSelectionBox(&isometric);
            }
            
            // Draw UI
            if (showInfo) {
                // Camera mode and unit info
                DrawRectangle(10, 10, 180, 145, Fade(BLACK, 0.7f));
                const char* modeText = (viewMode == VIEW_MODE_ORBIT) ? "ORBIT" : "ISOMETRIC";
                DrawText(modeText, 15, 15, 12, GREEN);
                DrawText("MODEL", 15, 35, 10, WHITE);
                DrawText(TextFormat("Meshes: %d", model.meshCount), 15, 50, 10, GRAY);
                DrawText(TextFormat("Triangles: %d", totalTriangles), 15, 65, 10, GRAY);
                DrawText(TextFormat("Vertices: %d", totalVertices), 15, 80, 10, GRAY);
                
                // Unit counter
                int activeUnits = 0;
                int selectedUnits = 0;
                for (int i = 0; i < unitCount; i++) {
                    if (units[i].active) {
                        activeUnits++;
                        if (units[i].selected) selectedUnits++;
                    }
                }
                DrawText(TextFormat("Units: %d/%d", activeUnits, MAX_UNITS), 15, 95, 10, YELLOW);
                DrawText(TextFormat("Selected: %d", selectedUnits), 15, 110, 10, GREEN);
                
                // Show active control groups
                char groupsText[64] = "Groups: ";
                bool hasGroups = false;
                for (int g = 1; g <= 9; g++) {
                    if (controlGroups[g].active && controlGroups[g].unitCount > 0) {
                        char groupNum[4];
                        snprintf(groupNum, sizeof(groupNum), "%d ", g);
                        strcat(groupsText, groupNum);
                        hasGroups = true;
                    }
                }
                if (hasGroups) {
                    DrawText(groupsText, 15, 125, 10, YELLOW);
                }
                
                if (viewMode == VIEW_MODE_ORBIT) {
                    DrawText(TextFormat("Dist: %.1f", orbit.distance), 15, 115, 10, GRAY);
                } else {
                    DrawText(TextFormat("Height: %.1f", isometric.height), 15, 115, 10, GRAY);
                }
            }
            
            // Controls help
            DrawRectangle(10, WINDOW_HEIGHT - 160, 380, 150, Fade(BLACK, 0.7f));
            if (viewMode == VIEW_MODE_ORBIT) {
                DrawText("ORBIT MODE", 15, WINDOW_HEIGHT - 155, 10, GREEN);
                DrawText("Mouse: L-Rotate M-Pan Wheel-Zoom", 15, WINDOW_HEIGHT - 140, 10, GRAY);
                DrawText("R-Click: Command Units", 15, WINDOW_HEIGHT - 125, 10, LIME);
            } else {
                DrawText("ISOMETRIC MODE", 15, WINDOW_HEIGHT - 155, 10, GREEN);
                DrawText("WASD/Arrows: Pan  M-Drag: Pan", 15, WINDOW_HEIGHT - 140, 10, GRAY);
                DrawText("Wheel: Zoom  L-Drag: Select Units", 15, WINDOW_HEIGHT - 125, 10, GRAY);
                DrawText("R-Click: Command Units", 15, WINDOW_HEIGHT - 110, 10, LIME);
            }
            DrawText("[Ctrl+1-9] Assign Group  [1-9] Select Group", 15, WINDOW_HEIGHT - 95, 10, SKYBLUE);
            DrawText("[SPACE] Spawn Units  [C] Clear Units", 15, WINDOW_HEIGHT - 80, 10, YELLOW);
            DrawText("[DELETE] Delete Selected  [U] Toggle Units", 15, WINDOW_HEIGHT - 65, 10, YELLOW);
            DrawText("[TAB] Switch Mode  [R] Reset Camera", 15, WINDOW_HEIGHT - 50, 10, GRAY);
            DrawText("[G] Grid  [X] Axes  [I] Info  [ESC] Exit", 15, WINDOW_HEIGHT - 35, 10, GRAY);
            
            // Edge scroll indicator for isometric mode
            if (viewMode == VIEW_MODE_ISOMETRIC) {
                Vector2 mousePos = GetMousePosition();
                if (mousePos.x < ISO_CAMERA_EDGE_SCROLL_ZONE ||
                    mousePos.x > WINDOW_WIDTH - ISO_CAMERA_EDGE_SCROLL_ZONE ||
                    mousePos.y < ISO_CAMERA_EDGE_SCROLL_ZONE ||
                    mousePos.y > WINDOW_HEIGHT - ISO_CAMERA_EDGE_SCROLL_ZONE) {
                    DrawText("EDGE SCROLL", WINDOW_WIDTH/2 - 40, 20, 10, Fade(YELLOW, 0.7f));
                }
            }
            
            // FPS
            DrawFPS(WINDOW_WIDTH - 80, 10);
            
        EndDrawing();
    }
    
    // Cleanup
    UnloadModel(model);
    CloseWindow();
    
    return 0;
}