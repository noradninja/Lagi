#include "lagi/azel_town_tasks.h"

#include "lagi/azel_compat.h"
#include "lagi/azel_town_bootstrap.h"
#include "lagi/azel_town_runtime.h"
#include "lagi/azel_town_collision.h"
#include "lagi/azel_render_bridge.h"
#include "lagi/debug_mesh.h"
#include "lagi/platform.h"
#include "task.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <vector>

s8 readSaturnS8(sSaturnPtr ptr);
u8 readSaturnU8(sSaturnPtr ptr);
s16 readSaturnS16(sSaturnPtr ptr);
u16 readSaturnU16(sSaturnPtr ptr);
s32 readSaturnS32(sSaturnPtr ptr);
sSaturnPtr readSaturnEA(sSaturnPtr ptr);

void addObjectToDrawList(sProcessed3dModel* model);
void addBillBoardToDrawList(sProcessed3dModel* model);

namespace lagi::azel {
namespace {

struct ScriptContext {
    sSaturnPtr pc{};
    std::array<sSaturnPtr, 16> stack{};
    unsigned stackPointer = 16;
    s32 result = 0;
    u16 waitFrames = 0;
    bool halted = false;
};

std::array<u8, 0x300> g_gameBits{};
ScriptContext g_script{};
unsigned g_pipelineFrames = 0;

struct TownWorldCellTask;
struct RuinLockTask;

struct TownCellObjectNode {
    u32 objectEA = 0;
    u32 definitionEA = 0;
    p_workArea task = nullptr;
    int cellIndex = -1;
    std::int8_t bundleIndex = -1;
    bool bundleAcquired = false;
};

struct WorldGridState {
    bool initialized = false;
    std::int8_t bundleIndex = -1;
    int ringX = 0;
    int ringY = 0;
    int currentX = 0;
    int currentY = 0;
    TownWorldCellTask* cells[8][8]{};
    std::array<std::vector<TownCellObjectNode>, 64> objectLists{};
    std::array<s32, 4> lodDepthThresholds{
        0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF};
    unsigned lodDepthCount = 1;
};

WorldGridState g_worldGrid{};
StaticRoomDebugMesh g_worldScene{};
p_workArea g_worldParent = nullptr;
std::array<RuinLockTask*, 16> g_npcLockTasks{};

struct EdgeRuntimeState {
    bool initialized = false;
    float position[3]{};
    float startPosition[3]{};
    float pitch = 0.0f;
    float yaw = 0.0f;
    float startPitch = 0.0f;
    float startYaw = 0.0f;
    float oldPosition[3]{};
    float stepTranslation[3]{};
    float stepRotationYaw = 0.0f;
    float lookAt[2]{};
    int inputX = 0;
    int inputY = 0;
    unsigned currentAnimation = 0;
    unsigned animationFrame = 0;
    unsigned previousAnimation = 0;
    unsigned previousAnimationFrame = 0;
    unsigned animationLeftOver = 0;
    unsigned transitionRemaining = 0;
    unsigned ambientAnimation = 5;
    bool autoWalk = false;
    float autoWalkTarget[3]{};
    TownCollisionBody collision{};
};

EdgeRuntimeState g_edge{};
struct MainLogicRuntimeState {
    bool initialized = false;
    bool followReady = false;
    int cameraParamsIndex = 0;
    float anchor[3]{};
    float rawCamera[3]{};
    float cameraPosition[3]{};
    float target[3]{};
    float up[3]{};
    float previousEdgePosition[3]{};
    float distance = 0.0f;
    float pitch = 0.0f;
    float yaw = 0.0f;
    float yawOffset = 0.0f;
    TownCollisionBody collision{};
};

MainLogicRuntimeState g_mainLogic{};
bool g_pendingEdgePosition = false;
bool g_pendingEdgeOrientation = false;
float g_pendingEdgePositionValue[3]{};
float g_pendingEdgePitch = 0.0f;
float g_pendingEdgeYaw = 0.0f;

static s32 initNpcWorld(s32 setupIndex);
static s32 initNpcFromStructWorld(u32 objectEA);
static s32 updateWorldGridRaw(s32 x, s32 z);
static s32 setupCameraFollowMode();
static bool getBit(unsigned bit);
static bool pointerValid(sSaturnPtr ptr, u32 bytes);

static float wrapAngle(float value)
{
    constexpr float kPi = 3.14159265358979323846f;
    constexpr float kTau = kPi * 2.0f;
    while (value > kPi) value -= kTau;
    while (value < -kPi) value += kTau;
    return value;
}

static void cameraBasisZ(float yaw, float pitch, float out[3])
{
    const float cp = std::cos(pitch);
    out[0] = std::sin(yaw) * cp;
    out[1] = -std::sin(pitch);
    out[2] = std::cos(yaw) * cp;
}

static void presentCamera()
{
    platform::renderer::town_present_camera(
        g_mainLogic.cameraPosition,
        g_mainLogic.rawCamera,
        g_mainLogic.target,
        g_mainLogic.up,
        g_mainLogic.yaw,
        g_mainLogic.pitch,
        g_mainLogic.distance);
}

static void initializeMainLogic()
{
    g_mainLogic = {};
    g_mainLogic.initialized = true;
    g_mainLogic.cameraParamsIndex = getBit(0x274u * 8u + 7u) ? 1 : 0;
    setCollisionSetup(g_mainLogic.collision, 0);

    sSaturnMemoryFile* overlay = town_overlay_file();
    if (overlay) {
        const sSaturnPtr minimum = overlay->getSaturnPtr(0x0605EEE4u);
        const sSaturnPtr maximum = overlay->getSaturnPtr(0x0605EEF0u);
        if (pointerValid(minimum, 12) && pointerValid(maximum, 12)) {
            constexpr float kFixed = 1.0f / 65536.0f;
            setCollisionBounds(
                g_mainLogic.collision,
                {readSaturnS32(minimum) * kFixed,
                 readSaturnS32(minimum + 4) * kFixed,
                 readSaturnS32(minimum + 8) * kFixed},
                {readSaturnS32(maximum) * kFixed,
                 readSaturnS32(maximum + 4) * kFixed,
                 readSaturnS32(maximum + 8) * kFixed});
        }
    }
}

static s32 setupCameraFollowMode()
{
    if (!g_mainLogic.initialized || !g_edge.initialized)
        return 0;

    constexpr float kAnchorHeight =
        static_cast<float>(0x1800) / 65536.0f;
    constexpr float kInitialCameraOffset =
        static_cast<float>(0x199) / 65536.0f;
    constexpr float kInitialTargetOffset =
        static_cast<float>(-0x1000) / 65536.0f;
    g_mainLogic.anchor[0] = g_edge.position[0];
    g_mainLogic.anchor[1] = g_edge.position[1] + kAnchorHeight;
    g_mainLogic.anchor[2] = g_edge.position[2];

    float basisZ[3]{};
    cameraBasisZ(g_edge.yaw, g_edge.pitch, basisZ);
    for (unsigned i = 0; i < 3; ++i) {
        g_mainLogic.rawCamera[i] =
            g_mainLogic.anchor[i] + basisZ[i] * kInitialCameraOffset;
        g_mainLogic.cameraPosition[i] = g_mainLogic.rawCamera[i];
        g_mainLogic.target[i] =
            g_mainLogic.anchor[i] + basisZ[i] * kInitialTargetOffset;
        g_mainLogic.up[i] = g_mainLogic.cameraPosition[i];
        g_mainLogic.previousEdgePosition[i] = g_edge.position[i];
    }
    g_mainLogic.up[1] += 1.0f;
    g_mainLogic.distance = kInitialCameraOffset;
    g_mainLogic.pitch = g_edge.pitch;
    g_mainLogic.yaw = g_edge.yaw;
    g_mainLogic.yawOffset = 0.0f;
    g_mainLogic.followReady = true;
    presentCamera();
    return 0;
}

static float dot3(const float a[3], const float b[3])
{
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

static void normalize3(float v[3])
{
    const float length = std::sqrt(dot3(v, v));
    if (length > 0.000001f) {
        v[0] /= length;
        v[1] /= length;
        v[2] /= length;
    }
}

static void updateCameraTarget(const float desired[3])
{
    // updateCameraTarget() projects desired X/Y using the current camera
    // matrix, but deliberately projects the Edge anchor for Z.
    float viewZ[3] = {
        g_mainLogic.target[0] - g_mainLogic.cameraPosition[0],
        g_mainLogic.target[1] - g_mainLogic.cameraPosition[1],
        g_mainLogic.target[2] - g_mainLogic.cameraPosition[2]};
    normalize3(viewZ);
    float viewX[3] = {viewZ[2], 0.0f, -viewZ[0]};
    normalize3(viewX);
    float viewY[3] = {
        viewZ[1]*viewX[2] - viewZ[2]*viewX[1],
        viewZ[2]*viewX[0] - viewZ[0]*viewX[2],
        viewZ[0]*viewX[1] - viewZ[1]*viewX[0]};

    const float desiredDelta[3] = {
        desired[0] - g_mainLogic.cameraPosition[0],
        desired[1] - g_mainLogic.cameraPosition[1],
        desired[2] - g_mainLogic.cameraPosition[2]};
    const float anchorDelta[3] = {
        g_mainLogic.anchor[0] - g_mainLogic.cameraPosition[0],
        g_mainLogic.anchor[1] - g_mainLogic.cameraPosition[1],
        g_mainLogic.anchor[2] - g_mainLogic.cameraPosition[2]};
    const float depth = dot3(anchorDelta, viewZ);
    if (std::fabs(depth) <= 0.000001f) {
        std::copy(desired, desired + 3, g_mainLogic.target);
        return;
    }

    float projectedX = std::fabs(dot3(desiredDelta, viewX) / depth);
    float projectedY = std::fabs(dot3(desiredDelta, viewY) / depth);
    if (projectedX == 0.0f && projectedY == 0.0f) {
        std::copy(desired, desired + 3, g_mainLogic.target);
        return;
    }
    projectedX = std::min(projectedX, 128.0f);
    projectedY = std::min(projectedY, 128.0f);
    float weight = (projectedX*projectedX + projectedY*projectedY) * 0.5f;
    weight = std::clamp(
        weight,
        static_cast<float>(0xCCC) / 65536.0f,
        static_cast<float>(0xB333) / 65536.0f);
    const float retain = 1.0f - weight;
    for (unsigned i = 0; i < 3; ++i)
        g_mainLogic.target[i] =
            desired[i] + (g_mainLogic.target[i] - desired[i]) * retain;
}

static void updateFollowCamera()
{
    if (!g_mainLogic.followReady || !g_edge.initialized)
        return;

    constexpr float kTau = 6.28318530717958647692f;
    constexpr float kAnchorHeight =
        static_cast<float>(0x1800) / 65536.0f;
    constexpr float kDesiredDistance =
        static_cast<float>(0x2CCC) / 65536.0f;
    constexpr float kDesiredPitch0 =
        static_cast<float>(0xAAAAAA) / static_cast<float>(0x10000000) * kTau;
    constexpr float kDesiredPitch1 =
        static_cast<float>(-0x555555) / static_cast<float>(0x10000000) * kTau;
    constexpr float kMaxPitch =
        static_cast<float>(0x13E93E9) / static_cast<float>(0x10000000) * kTau;

    g_mainLogic.anchor[0] = g_edge.position[0];
    g_mainLogic.anchor[1] = g_edge.position[1] + kAnchorHeight;
    g_mainLogic.anchor[2] = g_edge.position[2];

    if (g_mainLogic.distance <
        static_cast<float>(0x151EB) / 65536.0f * kDesiredDistance) {
        const auto& solve = g_mainLogic.collision.collisionSolveTranslation;
        g_mainLogic.rawCamera[0] += solve.x;
        g_mainLogic.rawCamera[1] += solve.y;
        g_mainLogic.rawCamera[2] += solve.z;
    }

    const float backX = g_mainLogic.rawCamera[0] - g_mainLogic.anchor[0];
    const float backY = g_mainLogic.rawCamera[1] - g_mainLogic.anchor[1];
    const float backZ = g_mainLogic.rawCamera[2] - g_mainLogic.anchor[2];
    float distance = std::sqrt(backX*backX + backY*backY + backZ*backZ);
    distance = std::max(distance, 0.000001f);
    g_mainLogic.yaw = std::atan2(backX, backZ);
    g_mainLogic.pitch = std::atan2(
        -backY, std::sqrt(backX*backX + backZ*backZ));

    float maxTurn = std::atan2(
        static_cast<float>(0x147) / 65536.0f, distance);
    maxTurn = std::min(
        maxTurn,
        static_cast<float>(0x1555555) /
            static_cast<float>(0x10000000) * kTau);
    const float desiredPitch = g_mainLogic.cameraParamsIndex
        ? kDesiredPitch1 : kDesiredPitch0;
    const float pitchStep = std::clamp(
        wrapAngle(desiredPitch - g_mainLogic.pitch), -maxTurn, maxTurn);
    g_mainLogic.pitch = std::clamp(
        g_mainLogic.pitch + pitchStep, -kMaxPitch, kMaxPitch);

    const float yawStep = wrapAngle(
        g_edge.yaw + g_mainLogic.yawOffset - g_mainLogic.yaw);
    const bool stationary =
        g_edge.position[0] == g_mainLogic.previousEdgePosition[0] &&
        g_edge.position[1] == g_mainLogic.previousEdgePosition[1] &&
        g_edge.position[2] == g_mainLogic.previousEdgePosition[2];
    const float followArc =
        static_cast<float>(0x71C71C7) /
        static_cast<float>(0x10000000) * kTau;
    if ((yawStep < followArc && yawStep > -followArc) || stationary)
        g_mainLogic.yaw = wrapAngle(
            g_mainLogic.yaw + std::clamp(yawStep, -maxTurn, maxTurn));

    float distanceStep =
        (kDesiredDistance - distance) *
        (static_cast<float>(0x3333) / 65536.0f);
    const float maxDistanceStep = static_cast<float>(0x599) / 65536.0f;
    distance += std::clamp(distanceStep, -maxDistanceStep, maxDistanceStep);
    g_mainLogic.distance = distance;

    float basisZ[3]{};
    cameraBasisZ(g_mainLogic.yaw, g_mainLogic.pitch, basisZ);
    for (unsigned i = 0; i < 3; ++i)
        g_mainLogic.rawCamera[i] =
            g_mainLogic.anchor[i] + basisZ[i] * distance;

    float retain = static_cast<float>(0xCCCC) / 65536.0f;
    if (distance < kDesiredDistance) {
        retain = ((distance - static_cast<float>(0x1000) / 65536.0f) * retain) /
            (kDesiredDistance - static_cast<float>(0x1000) / 65536.0f);
        retain = std::max(0.0f, retain);
    }
    for (unsigned i = 0; i < 3; ++i)
        g_mainLogic.cameraPosition[i] =
            g_mainLogic.rawCamera[i] +
            (g_mainLogic.cameraPosition[i] - g_mainLogic.rawCamera[i]) * retain;

    const float targetScale = -distance * 0.5f;
    float edgeBasisZ[3]{};
    cameraBasisZ(g_edge.yaw, g_edge.pitch, edgeBasisZ);
    const float desiredTarget[3] = {
        g_mainLogic.anchor[0] + edgeBasisZ[0] * targetScale,
        g_mainLogic.anchor[1] + edgeBasisZ[1] * targetScale,
        g_mainLogic.anchor[2] + edgeBasisZ[2] * targetScale};
    updateCameraTarget(desiredTarget);

    for (unsigned i = 0; i < 3; ++i) {
        g_mainLogic.up[i] = g_mainLogic.cameraPosition[i];
        g_mainLogic.previousEdgePosition[i] = g_edge.position[i];
    }
    g_mainLogic.up[1] += 1.0f;
    g_mainLogic.collision.ownerPosition = {
        g_mainLogic.rawCamera[0],
        g_mainLogic.rawCamera[1],
        g_mainLogic.rawCamera[2]};
    g_mainLogic.collision.ownerRotation = {
        g_mainLogic.pitch, g_mainLogic.yaw, 0.0f};
    registerCollisionBody(g_mainLogic.collision);
    presentCamera();
}

static void presentEdge()
{
    const float transition = g_edge.transitionRemaining
        ? 1.0f - static_cast<float>(g_edge.transitionRemaining) / 5.0f
        : 1.0f;
    platform::renderer::town_present_edge(
        g_edge.position[0], g_edge.position[1], g_edge.position[2],
        g_edge.yaw,
        (g_edge.collision.contactMask & 0x4u) != 0,
        g_edge.collision.contactCount,
        g_edge.currentAnimation, g_edge.animationFrame,
        g_edge.previousAnimation, g_edge.previousAnimationFrame,
        transition);
}

static void setEdgePositionRaw(s32 x, s32 y, s32 z)
{
    constexpr float kFixed = 1.0f / 65536.0f;
    g_pendingEdgePosition = true;
    g_pendingEdgePositionValue[0] = static_cast<float>(x) * kFixed;
    g_pendingEdgePositionValue[1] = static_cast<float>(y) * kFixed;
    g_pendingEdgePositionValue[2] = static_cast<float>(z) * kFixed;
    std::copy(std::begin(g_pendingEdgePositionValue),
              std::end(g_pendingEdgePositionValue),
              std::begin(g_edge.position));
    presentEdge();
}

static void setEdgeOrientationRaw(s32 x, s32 y, s32)
{
    constexpr float kTau = 6.28318530717958647692f;
    g_pendingEdgeOrientation = true;
    g_pendingEdgePitch = static_cast<float>(x) /
        static_cast<float>(0x10000000) * kTau;
    g_pendingEdgeYaw = static_cast<float>(y) /
        static_cast<float>(0x10000000) * kTau;
    g_edge.pitch = g_pendingEdgePitch;
    g_edge.yaw = g_pendingEdgeYaw;
    presentEdge();
}

static void setEdgeAnimation(unsigned animation, unsigned transitionFrames = 5)
{
    if (!platform::renderer::town_edge_animation_frames(animation) ||
        animation == g_edge.currentAnimation)
        return;
    g_edge.previousAnimation = g_edge.currentAnimation;
    g_edge.previousAnimationFrame = g_edge.animationFrame;
    g_edge.currentAnimation = animation;
    g_edge.animationFrame = 0;
    g_edge.transitionRemaining = transitionFrames;
}

static void setupNpcWalkInZDirection(float zDirection, int distance)
{
    if (!g_edge.initialized) return;
    const float step = -zDirection;
    const float cp = std::cos(g_edge.pitch);
    const float forward[3] = {
        std::sin(g_edge.yaw) * cp,
        -std::sin(g_edge.pitch),
        std::cos(g_edge.yaw) * cp};
    g_edge.stepTranslation[0] = 0.0f;
    g_edge.stepTranslation[1] = 0.0f;
    g_edge.stepTranslation[2] = step;
    for (unsigned i = 0; i < 3; ++i)
        g_edge.autoWalkTarget[i] =
            g_edge.position[i] + forward[i] * step * distance;
    g_edge.autoWalk = true;
}

static void readEdgeInput()
{
    const int digitalX = platform::input::digital_x();
    const int digitalY = platform::input::digital_y();
    if (digitalX || digitalY) {
        // The Vita render bridge mirrors Saturn presentation X. Keep the
        // platform axes physical and convert once at the native town input.
        g_edge.inputX = -digitalX * 0x10000;
        g_edge.inputY = digitalY * 0x10000;
    } else {
        g_edge.inputX = static_cast<int>(std::lround(
            -platform::input::analog_x() * 65536.0f));
        // Vita LY is negative upward; Azel's town input is positive forward.
        g_edge.inputY = static_cast<int>(std::lround(
            -platform::input::analog_y() * 65536.0f));
    }
}

static void updateEdgeAnimation(float movedDistance)
{
    const bool grounded = (g_edge.collision.contactMask & 0x4u) != 0;
    const unsigned movementRate = static_cast<unsigned>(std::lround(
        movedDistance * 65536.0f * static_cast<float>(0x1E1)));
    unsigned animationSteps = 0;
    if (movementRate) {
        const unsigned counter = g_edge.animationLeftOver + movementRate;
        g_edge.animationLeftOver = counter & 0xFFFFu;
        // Saturn advances the animation state once per game frame. Never
        // catch up by skipping decoded poses when rendering falls below
        // 30 Hz; the whole simulation and animation slow together.
        animationSteps = std::min(counter >> 16, 1u);
    }

    if (!grounded &&
        (g_edge.stepTranslation[1] < -static_cast<float>(0x199) / 65536.0f ||
         g_edge.stepTranslation[1] > 0.0f)) {
        setEdgeAnimation(4);
        animationSteps = std::max(animationSteps, 1u);
    } else if (movementRate > 0x666u) {
        unsigned desired = 1u;
        if (g_edge.currentAnimation == 2u)
            desired = movementRate < 0x28000u ? 1u : 2u;
        else if (movementRate > 0x30000u)
            desired = 2u;
        setEdgeAnimation(desired);
    } else {
        if (g_edge.currentAnimation < 5u || g_edge.currentAnimation > 8u) {
            g_edge.ambientAnimation = 5u;
            setEdgeAnimation(g_edge.ambientAnimation);
        }
        animationSteps = std::max(animationSteps, 1u);
    }

    const unsigned frames = platform::renderer::town_edge_animation_frames(
        g_edge.currentAnimation);
    if (frames && animationSteps) {
        g_edge.animationFrame =
            (g_edge.animationFrame + animationSteps) % frames;
        if (movementRate <= 0x666u && g_edge.animationFrame == 0u) {
            const unsigned next = 5u + ((g_edge.ambientAnimation - 4u) & 3u);
            g_edge.ambientAnimation = next;
            setEdgeAnimation(next, 5u);
        }
    }
    if (g_edge.transitionRemaining)
        --g_edge.transitionRemaining;
}

static void updateEdgeLookAt()
{
    constexpr float kTau = 6.28318530717958647692f;
    const float limit = static_cast<float>(0x1C71C71) /
        static_cast<float>(0x10000000) * kTau;
    const float turn = std::clamp(g_edge.stepRotationYaw, -limit, limit);
    const float yawRate = turn != 0.0f
        ? static_cast<float>(0xB333) / 65536.0f
        : 0.5f;
    g_edge.lookAt[1] += (turn - g_edge.lookAt[1]) * yawRate;
    g_edge.lookAt[0] *= static_cast<float>(0xB333) / 65536.0f;
}

static void updateEdgePositionNative()
{
    std::copy(std::begin(g_edge.position), std::end(g_edge.position),
              std::begin(g_edge.oldPosition));

    const auto& solve = g_edge.collision.collisionSolveTranslation;
    g_edge.position[0] += solve.x;
    g_edge.position[1] += solve.y;
    g_edge.position[2] += solve.z;

    const bool grounded = (g_edge.collision.contactMask & 0x4u) != 0;
    if (grounded && g_edge.collision.floorNormal.y >=
            static_cast<float>(0xB504) / 65536.0f &&
        g_edge.stepTranslation[1] < 0.0f)
        g_edge.stepTranslation[1] = 0.0f;

    if (g_edge.autoWalk) {
        const float dx = g_edge.autoWalkTarget[0] - g_edge.position[0];
        const float dy = g_edge.autoWalkTarget[1] - g_edge.position[1];
        const float dz = g_edge.autoWalkTarget[2] - g_edge.position[2];
        const float remaining = std::sqrt(dx*dx + dy*dy + dz*dz);
        const float step = std::fabs(g_edge.stepTranslation[2]);
        if (remaining <= step) {
            std::copy(std::begin(g_edge.autoWalkTarget),
                      std::end(g_edge.autoWalkTarget),
                      std::begin(g_edge.position));
            g_edge.autoWalk = false;
            g_edge.stepTranslation[2] = 0.0f;
        } else {
            constexpr float kTau = 6.28318530717958647692f;
            const float targetYaw = std::atan2(
                g_edge.position[0] - g_edge.autoWalkTarget[0],
                g_edge.position[2] - g_edge.autoWalkTarget[2]);
            const float maxTurn =
                static_cast<float>(0x2D82D8) /
                static_cast<float>(0x10000000) * kTau;
            g_edge.stepRotationYaw = std::clamp(
                wrapAngle(targetYaw - g_edge.yaw), -maxTurn, maxTurn);
            g_edge.yaw = wrapAngle(g_edge.yaw + g_edge.stepRotationYaw);
        }
    } else {
        readEdgeInput();
        const float inputX = static_cast<float>(g_edge.inputX) / 65536.0f;
        const float inputY = static_cast<float>(g_edge.inputY) / 65536.0f;
        const float inputMagnitude = std::min(
            1.0f, std::sqrt(inputX*inputX + inputY*inputY));
        float rotationStep = 0.0f;
        if (inputMagnitude > 0.0f) {
            const float desired = std::atan2(inputX, inputY) + g_mainLogic.yaw;
            rotationStep = wrapAngle(desired - g_edge.yaw);
            constexpr float kTau = 6.28318530717958647692f;
            const float baseTurn =
                static_cast<float>(0x4FA4FA) /
                static_cast<float>(0x10000000) * kTau;
            const float distanceFactor = std::max(
                0.0f,
                (g_mainLogic.distance -
                 static_cast<float>(0xA8F) / 65536.0f) /
                (static_cast<float>(0x1000) / 65536.0f));
            const float minimumTurn =
                static_cast<float>(0xE38E3) /
                static_cast<float>(0x10000000) * kTau;
            const float maxTurn = std::max(minimumTurn, baseTurn * distanceFactor);
            rotationStep = std::clamp(rotationStep, -maxTurn, maxTurn);
        }
        g_edge.stepRotationYaw = rotationStep;
        g_edge.yaw = wrapAngle(g_edge.yaw + rotationStep);

        const float speed = static_cast<float>(
            platform::input::run_held() ? -0x212 : -0x109) / 65536.0f;
        const float desiredStep = inputMagnitude * speed;
        const float damped =
            (g_edge.stepTranslation[2] * std::cos(rotationStep) - desiredStep) *
            (static_cast<float>(0xE666) / 65536.0f);
        g_edge.stepTranslation[0] = 0.0f;
        g_edge.stepTranslation[2] = desiredStep + damped;
    }

    g_edge.stepTranslation[1] -= static_cast<float>(0x56) / 65536.0f;
    g_edge.stepTranslation[1] = std::max(
        g_edge.stepTranslation[1], -static_cast<float>(0x800) / 65536.0f);

    float forward[3] = {
        -std::sin(g_edge.yaw), 0.0f, -std::cos(g_edge.yaw)};
    if (grounded) {
        const auto& n = g_edge.collision.floorNormal;
        const float projection = forward[0]*n.x + forward[2]*n.z;
        forward[0] -= n.x * projection;
        forward[1] -= n.y * projection;
        forward[2] -= n.z * projection;
        const float length = std::sqrt(
            forward[0]*forward[0] + forward[1]*forward[1] +
            forward[2]*forward[2]);
        if (length > 0.000001f)
            for (float& component : forward) component /= length;
    }
    g_edge.position[0] += forward[0] * -g_edge.stepTranslation[2];
    g_edge.position[1] +=
        forward[1] * -g_edge.stepTranslation[2] + g_edge.stepTranslation[1];
    g_edge.position[2] += forward[2] * -g_edge.stepTranslation[2];

    updateEdgeLookAt();

    const float dx = g_edge.position[0] - g_edge.oldPosition[0];
    const float dy = g_edge.position[1] - g_edge.oldPosition[1];
    const float dz = g_edge.position[2] - g_edge.oldPosition[2];
    updateEdgeAnimation(std::sqrt(dx*dx + dy*dy + dz*dz));
}

static sSaturnPtr align2(sSaturnPtr ptr)
{
    ptr.m_offset = (ptr.m_offset + 1) & ~1;
    return ptr;
}

static sSaturnPtr align4(sSaturnPtr ptr)
{
    ptr.m_offset = (ptr.m_offset + 3) & ~3;
    return ptr;
}

static bool pointerValid(sSaturnPtr ptr, u32 bytes = 1)
{
    if (!ptr.m_file || !ptr.m_file->m_data || ptr.m_offset < ptr.m_file->m_base)
        return false;
    const u32 offset = static_cast<u32>(ptr.m_offset) - ptr.m_file->m_base;
    return offset <= ptr.m_file->m_dataSize &&
           bytes <= ptr.m_file->m_dataSize - offset;
}

static unsigned adjustedBit(s16 encoded)
{
    return static_cast<unsigned>(encoded < 1000 ? encoded + 3334 : encoded);
}

static bool getBit(unsigned bit)
{
    return bit < g_gameBits.size() * 8u &&
           (g_gameBits[bit / 8u] & (0x80u >> (bit & 7u))) != 0;
}

static void setBit(unsigned bit, bool value)
{
    if (bit >= g_gameBits.size() * 8u)
        return;
    const u8 mask = static_cast<u8>(0x80u >> (bit & 7u));
    if (value) g_gameBits[bit / 8u] |= mask;
    else g_gameBits[bit / 8u] &= static_cast<u8>(~mask);
}

static u32 readPackedBits(unsigned first, unsigned count)
{
    u32 value = 0;
    for (unsigned i = 0; i < count && i < 32; ++i)
        value = (value << 1) | (getBit(first + i) ? 1u : 0u);
    return value;
}

static void writePackedBits(unsigned first, unsigned count, u32 value)
{
    for (unsigned i = 0; i < count && i < 32; ++i)
        setBit(first + i, (value & (1u << (count - 1u - i))) != 0);
}


static azel_bridge::SubmissionState makeTownSubmissionState(
    const float position[3],
    s16 rx, s16 ry, s16 rz,
    bool billboard)
{
    azel_bridge::SubmissionState state{};
    constexpr float kTau = 6.28318530717958647692f;
    const float ax = static_cast<float>(rx) / 4096.0f * kTau;
    const float ay = static_cast<float>(ry) / 4096.0f * kTau;
    const float az = static_cast<float>(rz) / 4096.0f * kTau;
    const float cx = std::cos(ax), sx = std::sin(ax);
    const float cy = std::cos(ay), sy = std::sin(ay);
    const float cz = std::cos(az), sz = std::sin(az);
    const float m[9] = {
        cz*cy, cz*sy*sx - sz*cx, cz*sy*cx + sz*sx,
        sz*cy, sz*sy*sx + cz*cx, sz*sy*cx - cz*sx,
        -sy,   cy*sx,            cy*cx};

    state.modelMatrix[0] = static_cast<s32>(std::lround(m[0]*65536.0f));
    state.modelMatrix[1] = static_cast<s32>(std::lround(m[1]*65536.0f));
    state.modelMatrix[2] = static_cast<s32>(std::lround(m[2]*65536.0f));
    state.modelMatrix[4] = static_cast<s32>(std::lround(m[3]*65536.0f));
    state.modelMatrix[5] = static_cast<s32>(std::lround(m[4]*65536.0f));
    state.modelMatrix[6] = static_cast<s32>(std::lround(m[5]*65536.0f));
    state.modelMatrix[8] = static_cast<s32>(std::lround(m[6]*65536.0f));
    state.modelMatrix[9] = static_cast<s32>(std::lround(m[7]*65536.0f));
    state.modelMatrix[10] = static_cast<s32>(std::lround(m[8]*65536.0f));
    state.modelMatrix[3] = static_cast<s32>(std::lround(position[0]*65536.0f));
    state.modelMatrix[7] = static_cast<s32>(std::lround(position[1]*65536.0f));
    state.modelMatrix[11] = static_cast<s32>(std::lround(position[2]*65536.0f));
    state.hasModelMatrix = true;
    state.billboard = billboard;

    if (g_worldScene.lightingValid) {
        state.lightVector[0] = static_cast<s32>(
            -g_worldScene.lightDirection[0] * 65536.0f);
        state.lightVector[1] = static_cast<s32>(
            -g_worldScene.lightDirection[1] * 65536.0f);
        state.lightVector[2] = static_cast<s32>(
            -g_worldScene.lightDirection[2] * 65536.0f);
        state.lightColor[0] = g_worldScene.lightColor[0];
        state.lightColor[1] = g_worldScene.lightColor[1];
        state.lightColor[2] = g_worldScene.lightColor[2];
        state.hasLight = true;
    }
    return state;
}

static void detachTownObjectTask(p_workArea task)
{
    if (!task)
        return;
    for (auto& list : g_worldGrid.objectLists) {
        for (auto& node : list) {
            if (node.task == task) {
                node.task = nullptr;
                node.cellIndex = -1;
                node.bundleIndex = -1;
                node.bundleAcquired = false;
                return;
            }
        }
    }
}

struct RuinLockTask final : s_workAreaTemplateWithArg<RuinLockTask, u32> {
    u32 objectEA = 0;
    int cellIndex = -1;
    std::int8_t bundleIndex = -1;
    bool ownsBundleReference = false;
    bool collisionReady = false;
    bool resourcesReleased = false;
    u16 modelOffset = 0;
    u16 npcIndex = 0xFFFFu;
    u16 collisionModelOffset = 0;
    float position[3]{};
    s32 rotationRaw[3]{};
    s16 status = 0;
    s16 translationLength = 0x32;
    TownCollisionBody collision{};

    static void ReleaseResources(RuinLockTask* self)
    {
        if (!self || self->resourcesReleased)
            return;
        self->resourcesReleased = true;
        if (self->npcIndex < g_npcLockTasks.size() &&
            g_npcLockTasks[self->npcIndex] == self)
            g_npcLockTasks[self->npcIndex] = nullptr;
        if (self->ownsBundleReference && self->bundleIndex >= 0) {
            release_town_runtime_bundle(self->bundleIndex);
            self->ownsBundleReference = false;
        }
    }

    static void Remove(RuinLockTask* self)
    {
        if (!self)
            return;
        detachTownObjectTask(self);
        ReleaseResources(self);
        self->getTask()->markFinished();
    }

    static void Init(RuinLockTask* self, u32 objectEA)
    {
        self->objectEA = objectEA;
        self->translationLength = 0x32;
        sSaturnMemoryFile* const overlay = town_overlay_file();
        if (!overlay || !objectEA)
            return;

        const sSaturnPtr object = overlay->getSaturnPtr(objectEA);
        if (!pointerValid(object, 0x2Au))
            return;

        constexpr float kFixed = 1.0f / 65536.0f;
        self->position[0] = readSaturnS32(object + 8) * kFixed;
        self->position[1] = readSaturnS32(object + 0x0C) * kFixed;
        self->position[2] = readSaturnS32(object + 0x10) * kFixed;
        self->rotationRaw[0] = readSaturnS32(object + 0x14);
        self->rotationRaw[1] = readSaturnS32(object + 0x18);
        self->rotationRaw[2] = readSaturnS32(object + 0x1C);
        self->modelOffset = readSaturnU16(object + 0x20);
        self->npcIndex = readSaturnU16(object + 0x28);

        if (self->npcIndex < g_npcLockTasks.size())
            g_npcLockTasks[self->npcIndex] = self;
    }

    static bool InitializeCollision(RuinLockTask* self)
    {
        if (!self || self->collisionReady || self->bundleIndex < 0)
            return self && self->collisionReady;

        sSaturnMemoryFile* const overlay = town_overlay_file();
        if (!overlay)
            return false;
        const sSaturnPtr object = overlay->getSaturnPtr(self->objectEA);
        if (!pointerValid(object, 0x28))
            return false;

        const sSaturnPtr config = readSaturnEA(object + 0x24);
        if (!pointerValid(config, 0x20))
            return false;

        self->collisionModelOffset = readSaturnU16(config + 2);
        self->collision.owner = self;
        self->collision.collisionModel = self->collisionModelOffset
            ? town_runtime_model(self->bundleIndex, self->collisionModelOffset)
            : nullptr;
        self->collision.collisionScriptEA =
            static_cast<u32>(readSaturnEA(config + 4).m_offset);
        // Azel's town LCS consumes the same m3C script carried by this
        // collision body. Keep it explicitly available for the later LCS UI
        // milestone without inventing a temporary activation path here.
        self->collision.interactionScriptEA =
            self->collision.collisionScriptEA;

        setCollisionSetup(self->collision, readSaturnU8(config));
        constexpr float kFixed = 1.0f / 65536.0f;
        setCollisionBounds(
            self->collision,
            {readSaturnS32(config + 8) * kFixed,
             readSaturnS32(config + 0x0C) * kFixed,
             readSaturnS32(config + 0x10) * kFixed},
            {readSaturnS32(config + 0x14) * kFixed,
             readSaturnS32(config + 0x18) * kFixed,
             readSaturnS32(config + 0x1C) * kFixed});
        self->collisionReady = true;
        return true;
    }

    static void UpdateTask(RuinLockTask* self)
    {
        if (!InitializeCollision(self))
            return;

        switch (self->status) {
        case 0:
            break;
        case 1:
            self->position[1] -= static_cast<float>(0x7A) / 65536.0f;
            if (self->translationLength > 0)
                --self->translationLength;
            if (self->translationLength == 0)
                self->status = 2;
            break;
        case 2:
            Remove(self);
            return;
        default:
            break;
        }

        if (platform::renderer::town_scene_active()) {
            constexpr float kTau = 6.28318530717958647692f;
            self->collision.ownerPosition = {
                self->position[0], self->position[1], self->position[2]};
            self->collision.ownerRotation = {
                static_cast<float>(self->rotationRaw[0] >> 16) / 4096.0f * kTau,
                static_cast<float>(self->rotationRaw[1] >> 16) / 4096.0f * kTau,
                static_cast<float>(self->rotationRaw[2] >> 16) / 4096.0f * kTau};
            registerCollisionBody(self->collision);
        }
    }

    static void DrawTask(RuinLockTask* self)
    {
        if (!self->collisionReady || self->bundleIndex < 0 ||
            !g_mainLogic.followReady)
            return;

        sProcessed3dModel* const model =
            town_runtime_model(self->bundleIndex, self->modelOffset);
        if (!model)
            return;

        const auto state = makeTownSubmissionState(
            self->position,
            static_cast<s16>(self->rotationRaw[0] >> 16),
            static_cast<s16>(self->rotationRaw[1] >> 16),
            static_cast<s16>(self->rotationRaw[2] >> 16),
            false);
        azel_bridge::set_town_submission_context(
            self->bundleIndex,
            self->cellIndex >= 0 ? static_cast<u32>(self->cellIndex) : 0u,
            self->objectEA,
            self->modelOffset,
            state);
        addObjectToDrawList(model);
    }

    static void DeleteTask(RuinLockTask* self)
    {
        ReleaseResources(self);
    }

    static const TypedTaskDefinition* getTypedTaskDefinition()
    {
        static const TypedTaskDefinition definition{
            Init, UpdateTask, DrawTask, DeleteTask};
        return &definition;
    }
};

static s32 disableRuinLock(s32 npcIndex, s32 nextStatus)
{
    if (npcIndex >= 0 &&
        static_cast<std::size_t>(npcIndex) < g_npcLockTasks.size()) {
        RuinLockTask* const lock =
            g_npcLockTasks[static_cast<std::size_t>(npcIndex)];
        if (lock)
            lock->status = static_cast<s16>(nextStatus);
    }
    return 0;
}

static s32 waitForRuinLockDisableCompletion(s32 npcIndex)
{
    if (npcIndex < 0 ||
        static_cast<std::size_t>(npcIndex) >= g_npcLockTasks.size())
        return 1;
    RuinLockTask* const lock =
        g_npcLockTasks[static_cast<std::size_t>(npcIndex)];
    return !lock || lock->status == 2 ? 1 : 0;
}

static s32 dispatchNative(u32 functionEA, unsigned argc, const s32* args)
{
    switch (functionEA) {
    case 0x0600CCB4u:
        return argc == 1 ? initNpcWorld(args[0]) : 0;
    case 0x06014DF2u:
        return argc == 1 ? initNpcFromStructWorld(static_cast<u32>(args[0])) : 0;
    case 0x06054334u: // disableLock
        return argc == 2 ? disableRuinLock(args[0], args[1]) : 0;
    case 0x06054364u: // waitForLockDisableCompletion
        return argc == 1 ? waitForRuinLockDisableCompletion(args[0]) : 0;
    case 0x06027110u: // setNextGameStatus
    case 0x0602C1D0u: // fadeOutAllSequences
    case 0x0602C2CAu: // playSystemSoundEffect
    case 0x0602C32Au: // playBattleSoundEffect
    case 0x0603011Eu: // terminateTown
    case 0x0605C83Cu: // TwnFadeOut
    case 0x0605C7C4u: // TwnFadeIn
    case 0x0605B320u: // NPC control
    case 0x0605C55Cu: // townCamera_setup; decoded scene lighting owns it today.
        return 0;
    case 0x0600CDD4u: // getNpcData0_5d
        return 0;
    case 0x0605AEE0u:
        if (argc == 4 && args[0] == 0)
            setEdgePositionRaw(args[1], args[2], args[3]);
        return 0;
    case 0x0605AF0Eu:
        if (argc == 4 && args[0] == 0)
            setEdgeOrientationRaw(args[1], args[2], args[3]);
        return 0;
    case 0x060144C0u:
        return argc == 2 ? updateWorldGridRaw(args[0], args[1]) : 0;
    case 0x06057058u: {
        const s32 result = setupCameraFollowMode();
        setupNpcWalkInZDirection(
            static_cast<float>(227) / 65536.0f, 36);
        return result;
    }
    case 0x0600CC78u: // setSomethingInNpc0
    case 0x06057570u: // hasLoadingCompleted
    case 0x0605762Au: // setupAutoWalk
        return functionEA == 0x06057570u ? 1 : 0;
    case 0x0605800Eu: // isObjectCloseEnoughToActivate
        return 1;
    default:
        platform::logging::writef(
            "[TownScript] unported native %08X argc=%u at %08X; script paused\n",
            static_cast<unsigned>(functionEA), argc,
            static_cast<unsigned>(g_script.pc.m_offset));
        g_script.halted = true;
        return 0;
    }
}

static sSaturnPtr callNative(sSaturnPtr ptr)
{
    const unsigned argc = readSaturnU8(ptr);
    sSaturnPtr data = align4(ptr + 1);
    const u32 functionEA = static_cast<u32>(readSaturnEA(data).m_offset);
    s32 args[4]{};
    if (argc > 4) {
        g_script.halted = true;
        return {};
    }
    for (unsigned i = 0; i < argc; ++i)
        args[i] = readSaturnS32(data + static_cast<int>(4 + i * 4));
    g_script.result = dispatchNative(functionEA, argc, args);
    return data + static_cast<int>((argc + 1) * 4);
}

static void runScript()
{
    if (g_script.halted || g_script.pc.isNull())
        return;
    if (g_script.waitFrames) {
        --g_script.waitFrames;
        return;
    }

    sSaturnPtr pc = g_script.pc;
    for (unsigned operations = 0; operations < 512; ++operations) {
        if (!pointerValid(pc)) {
            g_script.halted = true;
            break;
        }
        const u8 opcode = readSaturnU8(pc);
        pc += 1u;
        switch (opcode) {
        case 1:
            if (g_script.stackPointer == g_script.stack.size()) {
                g_script.pc = {};
                return;
            }
            pc = g_script.stack[g_script.stackPointer++];
            break;
        case 2:
            pc = align2(pc);
            g_script.waitFrames = static_cast<u16>(readSaturnU16(pc) - 1u);
            g_script.pc = pc + 2;
            return;
        case 3: pc = readSaturnEA(align4(pc)); break;
        case 5:
            pc = g_script.result ? align4(pc + 4) : readSaturnEA(align4(pc));
            break;
        case 6: {
            sSaturnPtr address = align4(pc);
            if (g_script.stackPointer == 0) { g_script.halted = true; break; }
            g_script.stack[--g_script.stackPointer] = address + 4;
            pc = readSaturnEA(address);
            break;
        }
        case 7: pc = callNative(pc); break;
        case 8: pc = align2(pc); g_script.result = readSaturnS16(pc) == g_script.result; pc += 2u; break;
        case 9: pc = align2(pc); g_script.result = readSaturnS16(pc) != g_script.result; pc += 2u; break;
        case 10: pc = align2(pc); g_script.result = readSaturnS16(pc) < g_script.result; pc += 2u; break;
        case 11: pc = align2(pc); g_script.result = readSaturnS16(pc) <= g_script.result; pc += 2u; break;
        case 12: pc = align2(pc); g_script.result = readSaturnS16(pc) > g_script.result; pc += 2u; break;
        case 13: pc = align2(pc); g_script.result = readSaturnS16(pc) >= g_script.result; pc += 2u; break;
        case 14: pc = align2(pc); g_script.result += readSaturnS16(pc); pc += 2u; break;
        case 15: pc = align2(pc); setBit(adjustedBit(readSaturnS16(pc)), true); pc += 2u; break;
        case 16: pc = align2(pc); setBit(adjustedBit(readSaturnS16(pc)), false); pc += 2u; break;
        case 17: pc = align2(pc); g_script.result = getBit(adjustedBit(readSaturnS16(pc))); pc += 2u; break;
        case 18: {
            const unsigned count = readSaturnU8(pc); pc += 1u; pc = align2(pc);
            const unsigned first = adjustedBit(readSaturnS16(pc)); pc += 2u;
            g_script.result = static_cast<s32>(readPackedBits(first, count));
            break;
        }
        case 20: {
            const unsigned count = readSaturnU8(pc); pc += 1u; pc = align2(pc);
            const unsigned first = adjustedBit(readSaturnS16(pc)); pc += 2u;
            const s16 delta = readSaturnS16(pc); pc += 2u;
            writePackedBits(first, count, readPackedBits(first, count) + delta);
            break;
        }
        case 21: {
            const unsigned count = readSaturnU8(pc); pc += 1u; pc = align4(pc);
            pc = g_script.result >= 0 && static_cast<unsigned>(g_script.result) < readSaturnU8(pc)
                ? readSaturnEA(pc + g_script.result * 4)
                : pc + static_cast<int>(count * 4);
            break;
        }
        case 22: {
            const unsigned count = readSaturnU8(pc); pc += 1u; pc = align4(pc);
            if (g_script.result >= 0 && static_cast<unsigned>(g_script.result) < count) {
                if (g_script.stackPointer == 0) { g_script.halted = true; break; }
                g_script.stack[--g_script.stackPointer] = pc + static_cast<int>(count * 4);
                pc = readSaturnEA(pc + g_script.result * 4);
            } else pc += static_cast<unsigned>(count * 4);
            break;
        }
        case 24: g_script.result = 0; break;
        case 25: case 26: case 27: case 29: case 32: case 33: case 34:
            if (opcode == 27) pc = align4(pc) + 4;
            else if (opcode == 33 || opcode == 34) pc += 1u;
            break;
        case 31: {
            const sSaturnPtr retry = pc - 1u;
            pc = callNative(pc);
            if (!g_script.result) { g_script.pc = retry; return; }
            break;
        }
        case 36: pc = align2(pc) + 2; pc = align4(pc) + 4; break;
        case 46: pc = align2(pc) + 2; break;
        default:
            platform::logging::writef(
                "[TownScript] unsupported opcode %u at %08X; script paused\n",
                static_cast<unsigned>(opcode),
                static_cast<unsigned>(pc.m_offset - 1));
            g_script.halted = true;
            break;
        }
        if (g_script.halted)
            break;
    }
    g_script.pc = pc;
}

template<class T> static const typename T::TypedTaskDefinition* taskDefinition(
    typename T::FunctionType update,
    typename T::FunctionType draw = nullptr)
{
    static typename T::TypedTaskDefinition definition{nullptr, update, draw, nullptr};
    return &definition;
}

struct TownScriptTask final : s_workAreaTemplate<TownScriptTask> {
    static void Init(TownScriptTask* self) {
        resetCollisionFrame();
        g_worldParent = self;
    }
    static void UpdateTask(TownScriptTask*) {
        if (platform::renderer::town_scene_active()) {
            processAllCollisions();
            resetCollisionFrame();
        }
        runScript();
    }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        static const TypedTaskDefinition definition{Init, UpdateTask, nullptr, nullptr};
        return &definition;
    }
};

struct TownWorldCellTask final : s_workAreaTemplateWithArg<TownWorldCellTask, int> {
    int cellIndex = -1;

    static void Init(TownWorldCellTask* self, int index) {
        self->cellIndex = index;
    }
    static void DrawTask(TownWorldCellTask* self) {
        const auto& runtime = town_runtime();
        if (self->cellIndex < 0 ||
            self->cellIndex >= static_cast<int>(runtime.cells.size()) ||
            !g_mainLogic.followReady)
            return;

        const auto& cell = runtime.cells[
            static_cast<std::size_t>(self->cellIndex)];
        float viewZ[3] = {
            g_mainLogic.target[0] - g_mainLogic.cameraPosition[0],
            g_mainLogic.target[1] - g_mainLogic.cameraPosition[1],
            g_mainLogic.target[2] - g_mainLogic.cameraPosition[2]};
        normalize3(viewZ);
        float up[3] = {
            g_mainLogic.up[0] - g_mainLogic.cameraPosition[0],
            g_mainLogic.up[1] - g_mainLogic.cameraPosition[1],
            g_mainLogic.up[2] - g_mainLogic.cameraPosition[2]};
        normalize3(up);
        float viewX[3] = {
            up[1]*viewZ[2] - up[2]*viewZ[1],
            up[2]*viewZ[0] - up[0]*viewZ[2],
            up[0]*viewZ[1] - up[1]*viewZ[0]};
        normalize3(viewX);

        const float cellDelta[3] = {
            cell.origin[0] - g_mainLogic.cameraPosition[0],
            cell.origin[1] - g_mainLogic.cameraPosition[1],
            cell.origin[2] - g_mainLogic.cameraPosition[2]};
        const float cellDepth = dot3(cellDelta, viewZ);
        const float cellSide = dot3(cellDelta, viewX);
        const float cellRadius =
            static_cast<float>(0x10A3D) / 65536.0f *
            runtime.gridCellSize;
        const float nearClip = static_cast<float>(0x800) / 65536.0f;
        constexpr float kPi = 3.14159265358979323846f;
        const float widthScale =
            (176.0f / std::tan(40.0f * kPi / 180.0f)) *
            (352.0f / 320.0f);
        const float widthRatio = 176.0f / widthScale;
        const float widthRatio2 =
            std::sqrt(176.0f*176.0f + widthScale*widthScale) /
            widthScale;
        if (cellDepth < nearClip - cellRadius)
            return;
        const float horizontalLimit =
            cellDepth * widthRatio + cellRadius * widthRatio2;
        if (cellSide < -horizontalLimit || cellSide > horizontalLimit)
            return;

        sSaturnMemoryFile* const overlay = town_overlay_file();
        if (!overlay) return;

        unsigned objectIndex = 0;
        if (cell.staticObjectListEA) {
            sSaturnPtr entry = overlay->getSaturnPtr(cell.staticObjectListEA);
            constexpr unsigned kMaxObjects = 512;
            for (; objectIndex < kMaxObjects; ++objectIndex, entry += 0x18) {
                const s32 lodTableEA = readSaturnS32(entry);
                if (!lodTableEA) break;
                const float position[3] = {
                    cell.origin[0] + readSaturnS32(entry + 4) / 65536.0f,
                    cell.origin[1] + readSaturnS32(entry + 8) / 65536.0f,
                    cell.origin[2] + readSaturnS32(entry + 12) / 65536.0f};
                const float delta[3] = {
                    position[0] - g_mainLogic.cameraPosition[0],
                    position[1] - g_mainLogic.cameraPosition[1],
                    position[2] - g_mainLogic.cameraPosition[2]};
                const float depth = dot3(delta, viewZ);
                unsigned lod = 0;
                const s32 depthRaw = static_cast<s32>(std::clamp(
                    depth * 65536.0f,
                    static_cast<float>(INT32_MIN),
                    static_cast<float>(INT32_MAX)));
                while (lod + 1u < g_worldGrid.lodDepthCount &&
                       depthRaw > g_worldGrid.lodDepthThresholds[lod])
                    ++lod;
                const sSaturnPtr lodTable = readSaturnEA(entry);
                u16 modelOffset = readSaturnU16(lodTable + lod * 2u);
                sProcessed3dModel* const model = town_runtime_model(
                    g_worldGrid.bundleIndex, modelOffset);
                if (!model) continue;
                const auto state = makeTownSubmissionState(
                    position,
                    readSaturnS16(entry + 0x10),
                    readSaturnS16(entry + 0x12),
                    readSaturnS16(entry + 0x14),
                    false);
                azel_bridge::set_town_submission_context(
                    g_worldGrid.bundleIndex,
                    static_cast<u32>(self->cellIndex), objectIndex,
                    modelOffset, state);
                addObjectToDrawList(model);
            }
        }

        if (cell.billboardListEA) {
            sSaturnPtr entry = overlay->getSaturnPtr(cell.billboardListEA);
            constexpr unsigned kMaxBillboards = 512;
            for (unsigned i = 0; i < kMaxBillboards; ++i, ++objectIndex,
                 entry += 0x10) {
                const u32 modelOffset = static_cast<u32>(readSaturnS32(entry));
                if (!modelOffset) break;
                const float position[3] = {
                    cell.origin[0] + readSaturnS32(entry + 4) / 65536.0f,
                    cell.origin[1] + readSaturnS32(entry + 8) / 65536.0f,
                    cell.origin[2] + readSaturnS32(entry + 12) / 65536.0f};
                sProcessed3dModel* const model = town_runtime_model(
                    g_worldGrid.bundleIndex, modelOffset);
                if (!model) continue;
                const auto state = makeTownSubmissionState(position, 0, 0, 0, true);
                azel_bridge::set_town_submission_context(
                    g_worldGrid.bundleIndex,
                    static_cast<u32>(self->cellIndex), objectIndex,
                    modelOffset, state);
                addBillBoardToDrawList(model);
            }
        }
    }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        static const TypedTaskDefinition definition{Init, nullptr, DrawTask, nullptr};
        return &definition;
    }
};


static void destroyCellObject(TownCellObjectNode& node)
{
    if (!node.task)
        return;

    if (node.definitionEA == 0x0605EA20u) {
        auto* const lock = static_cast<RuinLockTask*>(node.task);
        RuinLockTask::ReleaseResources(lock);
    }
    node.task->getTask()->markFinished();
    node.task = nullptr;
    node.cellIndex = -1;
    node.bundleIndex = -1;
    node.bundleAcquired = false;
}

static void deleteCellObjects(int x, int y)
{
    const auto& runtime = town_runtime();
    if (x < 0 || y < 0 || x >= runtime.gridWidth || y >= runtime.gridHeight)
        return;
    const std::size_t index =
        static_cast<std::size_t>(y * runtime.gridWidth + x);
    if (index >= g_worldGrid.objectLists.size())
        return;
    for (auto& node : g_worldGrid.objectLists[index])
        destroyCellObject(node);
}

static void createCellObjects(int x, int y)
{
    const auto& runtime = town_runtime();
    if (!g_worldParent || x < 0 || y < 0 ||
        x >= runtime.gridWidth || y >= runtime.gridHeight)
        return;
    const int cellIndex = y * runtime.gridWidth + x;
    if (cellIndex < 0 ||
        static_cast<std::size_t>(cellIndex) >= g_worldGrid.objectLists.size())
        return;

    sSaturnMemoryFile* const overlay = town_overlay_file();
    if (!overlay)
        return;

    for (auto& node :
         g_worldGrid.objectLists[static_cast<std::size_t>(cellIndex)]) {
        if (node.task || !node.objectEA)
            continue;

        const sSaturnPtr object = overlay->getSaturnPtr(node.objectEA);
        if (!pointerValid(object, 8))
            continue;

        const s32 requestedBundle = readSaturnS32(object);
        node.definitionEA =
            static_cast<u32>(readSaturnEA(object + 4).m_offset);
        node.cellIndex = cellIndex;
        node.bundleIndex = requestedBundle > 0
            ? static_cast<std::int8_t>(requestedBundle)
            : g_worldGrid.bundleIndex;
        node.bundleAcquired = false;

        if (requestedBundle > 0) {
            if (!acquire_town_runtime_bundle(node.bundleIndex)) {
                node.bundleIndex = -1;
                continue;
            }
            node.bundleAcquired = true;
        }

        p_workArea task = nullptr;
        if (node.definitionEA == 0x0605EA20u) {
            auto* const lock =
                createSubTaskWithArg<RuinLockTask>(g_worldParent, node.objectEA);
            if (lock) {
                lock->cellIndex = cellIndex;
                lock->bundleIndex = node.bundleIndex;
                lock->ownsBundleReference = node.bundleAcquired;
                task = lock;
            }
        }

        if (!task) {
            if (node.bundleAcquired && node.bundleIndex >= 0)
                release_town_runtime_bundle(node.bundleIndex);
            node.bundleAcquired = false;
            node.bundleIndex = -1;
            continue;
        }

        node.task = task;
        const TownRuntimeBundle* const bundle =
            town_runtime_bundle(node.bundleIndex);
        platform::logging::writef(
            "[TownWorld] cell=%d object=%08X bundle=%d refs=%u\n",
            cellIndex,
            static_cast<unsigned>(node.objectEA),
            static_cast<int>(node.bundleIndex),
            bundle ? bundle->refCount : 0u);
    }
}

static void deleteCell(int x, int y)
{
    if (x < 0 || y < 0 || x >= town_runtime().gridWidth ||
        y >= town_runtime().gridHeight)
        return;
    TownWorldCellTask*& task =
        g_worldGrid.cells[(g_worldGrid.ringY + y - g_worldGrid.currentY) & 7]
                         [(g_worldGrid.ringX + x - g_worldGrid.currentX) & 7];
    if (task) {
        deleteCellObjects(x, y);
        task->getTask()->markFinished();
    }
    task = nullptr;
}

static void createCell(int x, int y)
{
    const auto& runtime = town_runtime();
    if (!g_worldParent || x < 0 || y < 0 ||
        x >= runtime.gridWidth || y >= runtime.gridHeight)
        return;
    const int index = y * runtime.gridWidth + x;
    if (index < 0 || index >= static_cast<int>(runtime.cells.size()) ||
        !runtime.cells[static_cast<std::size_t>(index)].valid)
        return;
    TownWorldCellTask*& slot =
        g_worldGrid.cells[(g_worldGrid.ringY + y - g_worldGrid.currentY) & 7]
                         [(g_worldGrid.ringX + x - g_worldGrid.currentX) & 7];
    if (!slot) {
        slot = createSubTaskWithArg<TownWorldCellTask>(g_worldParent, index);
        if (slot)
            createCellObjects(x, y);
    }
}

static void deleteAllGridCells()
{
    const auto& runtime = town_runtime();
    for (int y = 0; y < runtime.gridHeight; ++y)
        for (int x = 0; x < runtime.gridWidth; ++x)
            deleteCellObjects(x, y);

    for (auto& row : g_worldGrid.cells)
        for (auto*& cell : row) {
            if (cell)
                cell->getTask()->markFinished();
            cell = nullptr;
        }
}

static void createGridCells(int centerX, int centerY)
{
    g_worldGrid.ringX = 0;
    g_worldGrid.ringY = 0;
    g_worldGrid.currentX = centerX;
    g_worldGrid.currentY = centerY;
    for (int y = -2; y <= 2; ++y)
        for (int x = -2; x <= 2; ++x)
            createCell(centerX + x, centerY + y);
}

static void resetWorldGrid()
{
    deleteAllGridCells();
    if (g_worldGrid.bundleIndex >= 0)
        release_town_runtime_bundle(g_worldGrid.bundleIndex);
    g_worldGrid = {};
    g_worldGrid.bundleIndex = -1;
    g_worldScene = {};
    g_npcLockTasks.fill(nullptr);
}

static s32 initNpcWorld(s32 setupIndex)
{
    const auto& runtime = town_runtime();
    if (setupIndex != 0 || runtime.setupNpcFileIndex < 0)
        return 0;

    resetWorldGrid();
    if (!acquire_town_runtime_bundle(runtime.setupNpcFileIndex))
        return 0;
    g_worldGrid.bundleIndex = runtime.setupNpcFileIndex;

    if (!build_town_world_scene(g_worldScene) ||
        !platform::renderer::load_static_room_viewer(g_worldScene)) {
        resetWorldGrid();
        return 0;
    }

    g_edge = {};
    g_edge.initialized = g_worldScene.edgeTransformValid;
    if (g_edge.initialized) {
        std::copy(std::begin(g_worldScene.edgePosition),
                  std::end(g_worldScene.edgePosition),
                  std::begin(g_edge.position));
        std::copy(std::begin(g_worldScene.edgePosition),
                  std::end(g_worldScene.edgePosition),
                  std::begin(g_edge.startPosition));
        constexpr float kTau = 6.28318530717958647692f;
        g_edge.pitch = g_worldScene.edgeRotation[0] * kTau;
        g_edge.yaw = g_worldScene.edgeRotation[1] * kTau;
        if (g_pendingEdgePosition)
            std::copy(std::begin(g_pendingEdgePositionValue),
                      std::end(g_pendingEdgePositionValue),
                      std::begin(g_edge.position));
        if (g_pendingEdgeOrientation) {
            g_edge.pitch = g_pendingEdgePitch;
            g_edge.yaw = g_pendingEdgeYaw;
        }
        std::copy(std::begin(g_edge.position),
                  std::end(g_edge.position),
                  std::begin(g_edge.startPosition));
        std::copy(std::begin(g_edge.position),
                  std::end(g_edge.position),
                  std::begin(g_edge.oldPosition));
        g_edge.startPitch = g_edge.pitch;
        g_edge.startYaw = g_edge.yaw;
        g_edge.currentAnimation = 0;
        g_edge.previousAnimation = 0;
        if (g_worldScene.edgeCollisionValid) {
            setCollisionSetup(g_edge.collision, 0);
            setCollisionBounds(
                g_edge.collision,
                {g_worldScene.edgeCollisionMin[0],
                 g_worldScene.edgeCollisionMin[1],
                 g_worldScene.edgeCollisionMin[2]},
                {g_worldScene.edgeCollisionMax[0],
                 g_worldScene.edgeCollisionMax[1],
                 g_worldScene.edgeCollisionMax[2]});
        }
        presentEdge();
    }

    g_worldGrid.initialized = true;
    createGridCells(-3, -3);
    platform::logging::writef(
        "[TownWorld] initNPC setup=%d bundle=%d grid=%dx%d refs=%u\n",
        static_cast<int>(setupIndex),
        static_cast<int>(runtime.setupNpcFileIndex),
        static_cast<int>(runtime.gridWidth),
        static_cast<int>(runtime.gridHeight),
        town_runtime_bundle(runtime.setupNpcFileIndex)->refCount);
    return 0;
}

static s32 initNpcFromStructWorld(u32 objectEA)
{
    if (!g_worldGrid.initialized || !objectEA)
        return 0;
    sSaturnMemoryFile* overlay = town_overlay_file();
    if (!overlay)
        return 0;
    const sSaturnPtr object = overlay->getSaturnPtr(objectEA);
    const float x = static_cast<float>(readSaturnS32(object + 8)) / 65536.0f;
    const float z = static_cast<float>(readSaturnS32(object + 0x10)) / 65536.0f;
    const auto& runtime = town_runtime();
    const int cellX = std::clamp(
        static_cast<int>(x / runtime.gridCellSize),
        0, static_cast<int>(runtime.gridWidth) - 1);
    const int cellY = std::clamp(
        static_cast<int>(z / runtime.gridCellSize),
        0, static_cast<int>(runtime.gridHeight) - 1);
    TownCellObjectNode node{};
    node.objectEA = objectEA;
    node.definitionEA =
        static_cast<u32>(readSaturnEA(object + 4).m_offset);
    g_worldGrid.objectLists[static_cast<std::size_t>(
        cellY * runtime.gridWidth + cellX)].push_back(node);
    return 1;
}

static s32 updateWorldGridRaw(s32 x, s32 z)
{
    if (!g_worldGrid.initialized || town_runtime().gridCellSize <= 0.0f)
        return 0;
    const float worldX = static_cast<float>(x) / 65536.0f;
    const float worldZ = static_cast<float>(z) / 65536.0f;
    const int nextX = static_cast<int>(worldX / town_runtime().gridCellSize);
    const int nextY = static_cast<int>(worldZ / town_runtime().gridCellSize);
    const int dx = nextX - g_worldGrid.currentX;
    const int dy = nextY - g_worldGrid.currentY;

    if (std::abs(dx) > 1 || std::abs(dy) > 1) {
        deleteAllGridCells();
        createGridCells(nextX, nextY);
    } else {
        if (dx < 0) {
            for (int y = -2; y <= 2; ++y)
                deleteCell(g_worldGrid.currentX + 2, g_worldGrid.currentY + y);
            g_worldGrid.ringX = (g_worldGrid.ringX - 1) & 7;
            --g_worldGrid.currentX;
            for (int y = -2; y <= 2; ++y)
                createCell(g_worldGrid.currentX - 2, g_worldGrid.currentY + y);
        } else if (dx > 0) {
            for (int y = -2; y <= 2; ++y)
                deleteCell(g_worldGrid.currentX - 2, g_worldGrid.currentY + y);
            g_worldGrid.ringX = (g_worldGrid.ringX + 1) & 7;
            ++g_worldGrid.currentX;
            for (int y = -2; y <= 2; ++y)
                createCell(g_worldGrid.currentX + 2, g_worldGrid.currentY + y);
        }
        if (dy < 0) {
            for (int cellX = -2; cellX <= 2; ++cellX)
                deleteCell(g_worldGrid.currentX + cellX, g_worldGrid.currentY + 2);
            g_worldGrid.ringY = (g_worldGrid.ringY - 1) & 7;
            --g_worldGrid.currentY;
            for (int cellX = -2; cellX <= 2; ++cellX)
                createCell(g_worldGrid.currentX + cellX, g_worldGrid.currentY - 2);
        } else if (dy > 0) {
            for (int cellX = -2; cellX <= 2; ++cellX)
                deleteCell(g_worldGrid.currentX + cellX, g_worldGrid.currentY - 2);
            g_worldGrid.ringY = (g_worldGrid.ringY + 1) & 7;
            ++g_worldGrid.currentY;
            for (int cellX = -2; cellX <= 2; ++cellX)
                createCell(g_worldGrid.currentX + cellX, g_worldGrid.currentY + 2);
        }
    }
    update_town_runtime_active_cell(worldX, worldZ);
    return 0;
}

struct RuinBackgroundTask final : s_workAreaTemplate<RuinBackgroundTask> {
    static const TypedTaskDefinition* getTypedTaskDefinition() { return taskDefinition<RuinBackgroundTask>(nullptr); }
};

struct TownEdgeTask final : s_workAreaTemplate<TownEdgeTask> {
    static void UpdateTask(TownEdgeTask*) {
        if (!g_edge.initialized || !platform::renderer::town_scene_active())
            return;

        if (platform::input::reset_view_pressed()) {
            std::copy(std::begin(g_edge.startPosition),
                      std::end(g_edge.startPosition),
                      std::begin(g_edge.position));
            g_edge.pitch = g_edge.startPitch;
            g_edge.yaw = g_edge.startYaw;
            std::fill(std::begin(g_edge.stepTranslation),
                      std::end(g_edge.stepTranslation), 0.0f);
            g_edge.stepRotationYaw = 0.0f;
            g_edge.lookAt[0] = 0.0f;
            g_edge.lookAt[1] = 0.0f;
            g_edge.inputX = 0;
            g_edge.inputY = 0;
            g_edge.currentAnimation = 0;
            g_edge.previousAnimation = 0;
            g_edge.animationFrame = 0;
            g_edge.previousAnimationFrame = 0;
            g_edge.animationLeftOver = 0;
            g_edge.transitionRemaining = 0;
            g_edge.ambientAnimation = 5;
            g_edge.autoWalk = false;
        }

        updateEdgePositionNative();
        g_edge.collision.ownerPosition = {
            g_edge.position[0], g_edge.position[1], g_edge.position[2]};
        g_edge.collision.ownerRotation = {g_edge.pitch, g_edge.yaw, 0.0f};
        registerCollisionBody(g_edge.collision);
        presentEdge();
    }
    static void DrawTask(TownEdgeTask*) { platform::renderer::town_camera_update(); }
    static const TypedTaskDefinition* getTypedTaskDefinition() { return taskDefinition<TownEdgeTask>(UpdateTask, DrawTask); }
};

struct TownMainLogicTask final : s_workAreaTemplate<TownMainLogicTask> {
    static void UpdateTask(TownMainLogicTask*) {
        if (!g_edge.initialized || !platform::renderer::town_scene_active())
            return;
        const int x = static_cast<int>(
            std::lround(g_edge.position[0] * 65536.0f));
        const int z = static_cast<int>(
            std::lround(g_edge.position[2] * 65536.0f));
        updateWorldGridRaw(x, z);
        if (platform::input::reset_view_pressed())
            setupCameraFollowMode();
        updateFollowCamera();
    }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        return taskDefinition<TownMainLogicTask>(UpdateTask);
    }
};

struct TownCameraTask final : s_workAreaTemplate<TownCameraTask> {
    static const TypedTaskDefinition* getTypedTaskDefinition() { return taskDefinition<TownCameraTask>(nullptr); }
};

struct TwnRuinTask final : s_workAreaTemplate<TwnRuinTask> {
    static void Init(TwnRuinTask* self) {
        sSaturnMemoryFile* overlay = town_overlay_file();
        const TownRuntimeState& town = town_runtime();
        g_script = {};
        g_gameBits.fill(0);
        g_pipelineFrames = 0;
        g_edge = {};
        g_mainLogic = {};
        g_pendingEdgePosition = false;
        g_pendingEdgeOrientation = false;
        initializeMainLogic();
        if (overlay && town.initialScriptEA)
            g_script.pc = overlay->getSaturnPtr(town.initialScriptEA);

        // overlayStart_TWN_RUIN creates these siblings in this exact order.
        createSubTask<TownScriptTask>(self);
        createSubTask<RuinBackgroundTask>(self);
        createSubTask<TownEdgeTask>(self);
        createSubTask<TownMainLogicTask>(self);
        createSubTask<TownCameraTask>(self);
        platform::logging::writef(
            "[TownTasks] overlayStart_TWN_RUIN script=%08X children=script/background/edge/main/camera\n",
            static_cast<unsigned>(town.initialScriptEA));
    }
    static void UpdateTask(TwnRuinTask*) { ++g_pipelineFrames; }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        static const TypedTaskDefinition definition{Init, UpdateTask, nullptr, nullptr};
        return &definition;
    }
};

TwnRuinTask* g_twnRuinTask = nullptr;

} // namespace

bool start_twn_ruin_task_pipeline()
{
    if (!town_runtime().initialized || !town_overlay_file())
        return false;
    g_twnRuinTask = createRootTask<TwnRuinTask>();
    return g_twnRuinTask != nullptr;
}

bool twn_ruin_task_pipeline_alive()
{
    return g_twnRuinTask && g_pipelineFrames > 0;
}

} // namespace lagi::azel
