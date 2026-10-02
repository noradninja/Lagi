#include "lagi/lagi_town_collision.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace lagi::azel {
namespace {

constexpr float kFixedPointUnit = 1.0f / 65536.0f;
constexpr float kEpsilon = 1.0e-7f;

struct ContactFace {
    TownCollisionVec3 normal{};
    float distance = 0.0f;
    float planeDistance = 0.0f;
};

struct ProcessedVertex {
    TownCollisionVec3 position{};
    std::uint32_t clipFlags = 0;
};

struct CollisionRegistry {
    std::array<std::array<TownCollisionBody*, 0x3f>, 5> bodies{};
    std::array<unsigned, 5> counts{};
};

static constexpr std::array<TownCollisionSetup, 5> kCollisionSetups{{
    {1, 0, 0x10},
    {1, 1, 0x1c},
    {0, 0, 0x18},
    {2, 1, 0x00},
    {3, 1, 0x00},
}};

CollisionRegistry g_registry{};
std::array<ContactFace, 12> g_contactFaces{};
float g_collisionPositionBias = 0.0f;

static TownCollisionVec3 add(
    const TownCollisionVec3& a, const TownCollisionVec3& b)
{
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}

static TownCollisionVec3 sub(
    const TownCollisionVec3& a, const TownCollisionVec3& b)
{
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}

static TownCollisionVec3 mul(const TownCollisionVec3& v, float s)
{
    return {v.x * s, v.y * s, v.z * s};
}

static float dot(const TownCollisionVec3& a, const TownCollisionVec3& b)
{
    return a.x*b.x + a.y*b.y + a.z*b.z;
}

static float lengthSquared(const TownCollisionVec3& v)
{
    return dot(v, v);
}

static float safeDiv(float numerator, float denominator)
{
    return std::fabs(denominator) > kEpsilon
        ? numerator / denominator
        : 0.0f;
}

static TownCollisionVec3 rotateYxz(
    TownCollisionVec3 v, const TownCollisionVec3& r)
{
    const float cy = std::cos(r.y), sy = std::sin(r.y);
    const float cx = std::cos(r.x), sx = std::sin(r.x);
    const float cz = std::cos(r.z), sz = std::sin(r.z);
    v = {cy*v.x + sy*v.z, v.y, -sy*v.x + cy*v.z};
    v = {v.x, cx*v.y - sx*v.z, sx*v.y + cx*v.z};
    return {cz*v.x - sz*v.y, sz*v.x + cz*v.y, v.z};
}

static TownCollisionVec3 inverseRotateYxz(
    TownCollisionVec3 v, const TownCollisionVec3& r)
{
    const float cz = std::cos(r.z), sz = std::sin(r.z);
    const float cx = std::cos(r.x), sx = std::sin(r.x);
    const float cy = std::cos(r.y), sy = std::sin(r.y);
    v = {cz*v.x + sz*v.y, -sz*v.x + cz*v.y, v.z};
    v = {v.x, cx*v.y + sx*v.z, -sx*v.y + cx*v.z};
    return {cy*v.x - sy*v.z, v.y, sy*v.x + cy*v.z};
}

static float component(const TownCollisionVec3& v, int axis)
{
    return axis == 0 ? v.x : (axis == 1 ? v.y : v.z);
}

static void setComponent(TownCollisionVec3& v, int axis, float value)
{
    if (axis == 0) v.x = value;
    else if (axis == 1) v.y = value;
    else v.z = value;
}

static bool projectedPointInQuad(
    const std::array<ProcessedVertex, 4>& vertices,
    int axis0, int axis1,
    const TownCollisionVec3& half,
    bool nonNegative)
{
    for (unsigned i = 0; i < 4; ++i) {
        const auto& a = vertices[i].position;
        const auto& b = vertices[(i + 1u) & 3u].position;
        const float cross =
            (component(a, axis0) - component(half, axis0)) *
                (component(b, axis1) - component(half, axis1)) -
            (component(b, axis0) - component(half, axis0)) *
                (component(a, axis1) - component(half, axis1));
        if (nonNegative ? cross < 0.0f : cross > 0.0f)
            return false;
    }
    return true;
}

static void recordContact(
    TownCollisionBody& body,
    int axis,
    bool negativeFace,
    float plane,
    const TownCollisionVec3& projected,
    const TownCollisionVec3& normal)
{
    const float normalAxis = component(normal, axis);
    if (std::fabs(normalAxis) <= kEpsilon)
        return;

    const float half = component(body.halfAabb, axis);
    const float projectedAxis = component(projected, axis);
    const float distance = safeDiv(plane, normalAxis) +
        (negativeFace ? -half : half);

    static constexpr int kPrimaryNegative[3] = {1, 3, 5};
    static constexpr int kPrimaryPositive[3] = {0, 2, 4};
    static constexpr int kBackNegative[3] = {7, 9, 11};
    static constexpr int kBackPositive[3] = {6, 8, 10};
    static constexpr std::uint32_t kPrimaryNegativeBits[3] = {0x10, 0x8, 0x1};
    static constexpr std::uint32_t kPrimaryPositiveBits[3] = {0x20, 0x4, 0x2};
    static constexpr std::uint32_t kBackNegativeBits[3] = {0x1000, 0x800, 0x100};
    static constexpr std::uint32_t kBackPositiveBits[3] = {0x2000, 0x400, 0x200};

    const bool backFace = plane >= 0.0f;
    const int index = backFace
        ? (negativeFace ? kBackNegative[axis] : kBackPositive[axis])
        : (negativeFace ? kPrimaryNegative[axis] : kPrimaryPositive[axis]);
    body.contactMask |= backFace
        ? (negativeFace ? kBackNegativeBits[axis] : kBackPositiveBits[axis])
        : (negativeFace ? kPrimaryNegativeBits[axis] : kPrimaryPositiveBits[axis]);

    ContactFace& face = g_contactFaces[static_cast<std::size_t>(index)];
    const bool deeper = negativeFace
        ? face.distance > distance
        : face.distance < distance;
    if (deeper) {
        face.normal = normal;
        face.distance = distance;
        face.planeDistance = plane +
            (negativeFace ? -projectedAxis : projectedAxis);
    }
}

static std::uint32_t transformTownMeshVertices(
    const TownCollisionBody& body,
    const TownRuntimeCell& cell,
    const TownRuntimeCollisionInstance& instance,
    std::array<ProcessedVertex, 255>& output)
{
    std::uint32_t allClipped = 0x3f;
    const TownCollisionVec3 base{
        cell.origin[0] + instance.position[0],
        cell.origin[1] + instance.position[1],
        cell.origin[2] + instance.position[2]};

    for (std::size_t i = 0; i < instance.model.vertices.size(); ++i) {
        const auto& source = instance.model.vertices[i];
        TownCollisionVec3 world{
            base.x + source.x,
            base.y + source.y,
            base.z + source.z};
        const TownCollisionVec3 biasedPosition{
            body.position.x + g_collisionPositionBias,
            body.position.y,
            body.position.z + g_collisionPositionBias};
        TownCollisionVec3 local = inverseRotateYxz(
            sub(world, biasedPosition), body.ownerRotation);
        local = add(local, body.halfAabb);

        std::uint32_t flags = 0;
        for (int axis = 0; axis < 3; ++axis) {
            flags <<= 1;
            if (component(local, axis) <= 0.0f)
                flags |= 1u;
            flags <<= 1;
            if (component(local, axis) >= component(body.halfAabb, axis) * 2.0f)
                flags |= 1u;
        }
        output[i] = {local, flags};
        allClipped &= flags;
    }
    return allClipped;
}

static bool testTownMeshQuadForCollision(
    TownCollisionBody& body,
    const TownRuntimeCollisionQuad& quad,
    const std::array<ProcessedVertex, 255>& transformed)
{
    std::array<ProcessedVertex, 4> vertices{{
        transformed[quad.indices[0]], transformed[quad.indices[1]],
        transformed[quad.indices[2]], transformed[quad.indices[3]]}};
    if (vertices[0].clipFlags & vertices[1].clipFlags &
        vertices[2].clipFlags & vertices[3].clipFlags)
        return false;

    TownCollisionVec3 normal = inverseRotateYxz(
        {quad.normal[0], quad.normal[1], quad.normal[2]},
        body.ownerRotation);
    const float normalLength = std::sqrt(lengthSquared(normal));
    if (normalLength <= kEpsilon)
        return false;
    normal = mul(normal, 1.0f / normalLength);

    const TownCollisionVec3 projected{
        normal.x * body.halfAabb.x,
        normal.y * body.halfAabb.y,
        normal.z * body.halfAabb.z};
    const float plane = dot(normal, vertices[0].position) -
        projected.x - projected.y - projected.z;

    bool contacted = false;
    auto testAxis = [&](int axis, int projection0, int projection1) {
        const float projectedAxis = component(projected, axis);
        if (plane * plane >= projectedAxis * projectedAxis)
            return;
        const bool negativeFace = plane > projectedAxis;
        if (!projectedPointInQuad(
                vertices, projection0, projection1,
                body.halfAabb, negativeFace))
            return;
        contacted = true;
        if ((quad.cmdSrca & 0x0f) == 0)
            recordContact(
                body, axis, negativeFace, plane, projected, normal);
    };

    // These are the XY, YZ, and ZX tests used by Azel's
    // testTownMeshQuadForCollision, in the same order.
    testAxis(2, 0, 1);
    testAxis(0, 1, 2);
    testAxis(1, 2, 0);
    if (contacted) {
        ++body.contactCount;
        if (quad.onCollisionScriptIndex)
            body.lastCollisionScriptIndex = quad.onCollisionScriptIndex;
    }
    return contacted;
}

static float horizontalScale(const ContactFace& face)
{
    const float horizontal = std::sqrt(std::max(
        0.0f, 1.0f - face.normal.y * face.normal.y));
    return safeDiv(face.planeDistance, horizontal);
}

static void separationForSingleFace(
    TownCollisionVec3& separation, const ContactFace& face)
{
    const float scale = horizontalScale(face);
    separation.x = face.normal.x * scale;
    separation.z = face.normal.z * scale;
}

} // namespace

void setCollisionSetup(TownCollisionBody& body, int presetIndex)
{
    body.setupIndex = std::clamp(presetIndex, 0, 4);
    body.setup = kCollisionSetups[static_cast<std::size_t>(body.setupIndex)];
}

void setCollisionBounds(
    TownCollisionBody& body,
    const TownCollisionVec3& corner0,
    const TownCollisionVec3& corner1)
{
    body.aabbCenter = mul(add(corner0, corner1), 0.5f);
    body.halfAabb = {
        body.aabbCenter.x - std::min(corner0.x, corner1.x),
        body.aabbCenter.y - std::min(corner0.y, corner1.y),
        body.aabbCenter.z - std::min(corner0.z, corner1.z)};
    body.sphereRadius = std::sqrt(lengthSquared(body.halfAabb));
}

void resetCollisionFrame()
{
    g_registry = {};
    g_collisionPositionBias = g_collisionPositionBias < 0.0f
        ? 2.0f * kFixedPointUnit
        : -2.0f * kFixedPointUnit;
}

void registerCollisionBody(TownCollisionBody& body)
{
    const unsigned list = static_cast<unsigned>(
        std::clamp(body.setupIndex, 0, 4));
    unsigned& count = g_registry.counts[list];
    if (count >= g_registry.bodies[list].size())
        return;
    g_registry.bodies[list][count++] = &body;
    body.position = add(
        body.ownerPosition,
        rotateYxz(body.aabbCenter, body.ownerRotation));
}

void beginBodyCollisionTest(TownCollisionBody& body)
{
    g_contactFaces = {};
    body.contactMask = 0;
    body.pairedBody = nullptr;
    body.floorNormal = {};
    body.collisionSolveTranslation = {};
    body.contactCount = 0;
    body.lastCollisionScriptIndex = 0;
}

int getSubCellQuadrant(float worldX, float worldZ)
{
    const TownRuntimeState& runtime = town_runtime();
    if (runtime.gridCellSize <= 0.0f)
        return 0;
    const int twiceX = static_cast<int>(worldX / runtime.gridCellSize * 2.0f);
    const int twiceZ = static_cast<int>(worldZ / runtime.gridCellSize * 2.0f);
    return (twiceX & 1) + ((twiceZ & 1) << 1);
}

const TownRuntimeCell* getCellAtWorldPos(float worldX, float worldZ)
{
    const TownRuntimeState& runtime = town_runtime();
    if (!runtime.initialized || runtime.gridCellSize <= 0.0f ||
        runtime.gridWidth <= 0 || runtime.gridHeight <= 0)
        return nullptr;
    const int x = static_cast<int>(std::floor(worldX / runtime.gridCellSize));
    const int z = static_cast<int>(std::floor(worldZ / runtime.gridCellSize));
    if (x < 0 || z < 0 || x >= runtime.gridWidth || z >= runtime.gridHeight)
        return nullptr;
    const std::size_t index = static_cast<std::size_t>(
        z * runtime.gridWidth + x);
    return index < runtime.cells.size() && runtime.cells[index].valid
        ? &runtime.cells[index]
        : nullptr;
}

void processTownMeshCollision(
    TownCollisionBody& body,
    const TownRuntimeCell& cell,
    const TownRuntimeCollisionInstance& instance)
{
    if (instance.model.vertices.empty() ||
        instance.model.vertices.size() > 255u)
        return;
    std::array<ProcessedVertex, 255> transformed{};
    if (transformTownMeshVertices(body, cell, instance, transformed))
        return;

    for (const auto& quad : instance.model.quads) {
        if (body.setupIndex == 0 && (quad.cmdSrca & 0x0f00) != 0)
            continue;
        testTownMeshQuadForCollision(body, quad, transformed);
    }
}

void testBodyAgainstCell(
    TownCollisionBody& body,
    const TownRuntimeCell* cell)
{
    if (!cell || !cell->valid || !cell->collisionListEA)
        return;
    const TownCollisionVec3 positionInCell{
        body.position.x - cell->origin[0],
        body.position.y - cell->origin[1],
        body.position.z - cell->origin[2]};
    for (const auto& instance : cell->collisionInstances) {
        const TownCollisionVec3 meshPosition{
            instance.position[0], instance.position[1], instance.position[2]};
        const float radius = instance.model.radius + body.sphereRadius;
        if (lengthSquared(sub(positionInCell, meshPosition)) < radius * radius)
            processTownMeshCollision(body, *cell, instance);
    }
}

void handleCollisionWithTownEnv(TownCollisionBody& body)
{
    const TownRuntimeState& runtime = town_runtime();
    const float cellSize = runtime.gridCellSize;
    const float x = body.position.x;
    const float z = body.position.z;
    switch (getSubCellQuadrant(x, z)) {
    case 0:
        testBodyAgainstCell(body, getCellAtWorldPos(x-cellSize, z));
        testBodyAgainstCell(body, getCellAtWorldPos(x, z-cellSize));
        testBodyAgainstCell(body, getCellAtWorldPos(x-cellSize, z-cellSize));
        break;
    case 1:
        testBodyAgainstCell(body, getCellAtWorldPos(x+cellSize, z));
        testBodyAgainstCell(body, getCellAtWorldPos(x, z-cellSize));
        testBodyAgainstCell(body, getCellAtWorldPos(x+cellSize, z-cellSize));
        break;
    case 2:
        testBodyAgainstCell(body, getCellAtWorldPos(x-cellSize, z));
        testBodyAgainstCell(body, getCellAtWorldPos(x, z+cellSize));
        testBodyAgainstCell(body, getCellAtWorldPos(x-cellSize, z+cellSize));
        break;
    case 3:
        testBodyAgainstCell(body, getCellAtWorldPos(x+cellSize, z));
        testBodyAgainstCell(body, getCellAtWorldPos(x, z+cellSize));
        testBodyAgainstCell(body, getCellAtWorldPos(x+cellSize, z+cellSize));
        break;
    }
    testBodyAgainstCell(body, getCellAtWorldPos(x, z));

    // A one-cell town still owns a real cell even when its Saturn grid origin
    // is not zero. Preserve Azel's active-cell ownership in that case.
    if (runtime.cells.size() == 1u && !getCellAtWorldPos(x, z))
        testBodyAgainstCell(body, town_runtime_active_cell());
}

void computeCollisionSeparation(TownCollisionBody& body)
{
    // Promote back-facing contacts exactly as Azel does when the matching
    // primary face was not already recorded.
    struct Promotion { std::uint32_t back, primary; int from, to; };
    static constexpr Promotion promotions[] = {
        {0x400, 0x8, 8, 2}, {0x800, 0x4, 9, 3},
        {0x200, 0x1, 10, 4}, {0x100, 0x2, 11, 5},
        {0x2000, 0x10, 6, 0}, {0x1000, 0x20, 7, 1},
    };
    for (const auto& p : promotions) {
        if ((body.contactMask & p.back) && !(body.contactMask & p.primary)) {
            body.contactMask |= p.primary;
            g_contactFaces[p.to] = g_contactFaces[p.from];
            break;
        }
    }

    TownCollisionVec3 separation{};
    const std::uint32_t horizontalMask = body.contactMask & 0x33;
    const ContactFace& posX = g_contactFaces[0];
    const ContactFace& negX = g_contactFaces[1];
    const ContactFace& posZ = g_contactFaces[4];
    const ContactFace& negZ = g_contactFaces[5];

    auto solveTwoPlanes = [&](const ContactFace& a,
                              const ContactFace& b) {
        separation.x = safeDiv(
            (a.distance-b.distance)*a.normal.z*b.normal.z,
            a.normal.x*b.normal.z-b.normal.x*a.normal.z);
        separation.z = a.distance -
            safeDiv(a.normal.x, a.normal.z)*separation.x;
    };
    TownCollisionVec3 alternate{};
    switch (horizontalMask) {
    case 0:
        break;
    case 0x1:
        separationForSingleFace(separation, negZ);
        break;
    case 0x2:
        separationForSingleFace(separation, posZ);
        break;
    case 0x3:
        solveTwoPlanes(posZ, negZ);
        break;
    case 0x10:
        separationForSingleFace(separation, negX);
        break;
    case 0x11:
        if (negX.normal.z <= 0.0f && negZ.normal.x <= 0.0f) {
            separationForSingleFace(separation, negX);
            separationForSingleFace(alternate, negZ);
            separation.x = std::min(separation.x, alternate.x);
            separation.z = std::min(separation.z, alternate.z);
        } else {
            separation.z = negZ.distance;
            separation.x = negX.distance -
                safeDiv(negX.normal.z, negX.normal.x)*separation.z;
        }
        break;
    case 0x12:
        if (negX.normal.z < 0.0f || posZ.normal.x > 0.0f) {
            separation.z = posZ.distance;
            separation.x = negX.distance -
                safeDiv(negX.normal.z, negX.normal.x)*separation.z;
        } else {
            separationForSingleFace(separation, negX);
            separationForSingleFace(alternate, posZ);
            separation.x = std::min(separation.x, alternate.x);
            separation.z = std::max(separation.z, alternate.z);
        }
        break;
    case 0x13:
        separation.x = safeDiv(
            (posZ.distance-negZ.distance)*posZ.normal.z*negZ.normal.z,
            posZ.normal.x*negZ.normal.z-negZ.normal.x*posZ.normal.z);
        separation.x = std::min(separation.x, negX.distance);
        separation.z = posZ.distance -
            safeDiv(posZ.normal.x, posZ.normal.z)*separation.x;
        break;
    case 0x20:
        separationForSingleFace(separation, posX);
        break;
    case 0x21:
        if (posX.normal.z >= 0.0f && negZ.normal.x <= 0.0f) {
            separationForSingleFace(separation, posX);
            separationForSingleFace(alternate, negZ);
            separation.x = std::max(separation.x, alternate.x);
            separation.z = std::min(separation.z, alternate.z);
        } else {
            separation.z = negZ.distance;
            separation.x = posX.distance -
                safeDiv(posX.normal.z, posX.normal.x)*separation.z;
        }
        break;
    case 0x22:
        if (posX.normal.z >= 0.0f && posZ.normal.x >= 0.0f) {
            separationForSingleFace(separation, posX);
            separationForSingleFace(alternate, posZ);
            separation.x = std::max(separation.x, alternate.x);
            separation.z = std::max(separation.z, alternate.z);
        } else {
            separation.z = posZ.distance;
            separation.x = posX.distance -
                safeDiv(posX.normal.z, posX.normal.x)*separation.z;
        }
        break;
    case 0x23:
        separation.x = safeDiv(
            (posZ.distance-negZ.distance)*posZ.normal.z*negZ.normal.z,
            posZ.normal.x*negZ.normal.z-negZ.normal.x*posZ.normal.z);
        separation.x = std::max(separation.x, posX.distance);
        separation.z = posZ.distance -
            safeDiv(posZ.normal.x, posZ.normal.z)*separation.x;
        break;
    case 0x30:
        separation.z = safeDiv(
            (posX.distance-negX.distance)*posX.normal.x*negX.normal.x,
            posX.normal.z*negX.normal.x-negX.normal.z*posX.normal.x);
        separation.x = posX.distance -
            safeDiv(posX.normal.z, posX.normal.x)*separation.z;
        break;
    case 0x31:
        separation.z = safeDiv(
            (posX.distance-negX.distance)*posX.normal.x*negX.normal.x,
            posX.normal.z*negX.normal.x-negX.normal.z*posX.normal.x);
        separation.z = std::min(separation.z, negZ.distance);
        separation.x = posX.distance -
            safeDiv(posX.normal.z, posX.normal.x)*separation.z;
        break;
    case 0x32:
        separation.z = safeDiv(
            (posX.distance-negX.distance)*posX.normal.x*negX.normal.x,
            posX.normal.z*negX.normal.x-negX.normal.z*posX.normal.x);
        separation.z = std::max(separation.z, posZ.distance);
        separation.x = posX.distance -
            safeDiv(posX.normal.z, posX.normal.x)*separation.z;
        break;
    case 0x33: {
        const bool solveYFromX =
            std::fabs(posX.normal.y+negX.normal.y) <
            std::fabs(posZ.normal.y+negZ.normal.y);
        const ContactFace& a = solveYFromX ? posX : posZ;
        const ContactFace& b = solveYFromX ? negX : negZ;
        const float av = solveYFromX ? a.normal.x : a.normal.z;
        const float bv = solveYFromX ? b.normal.x : b.normal.z;
        separation.y = safeDiv(
            (a.distance-b.distance)*av*bv,
            a.normal.y*bv-b.normal.y*av);
        separation.x = posX.distance -
            safeDiv(posX.normal.y, posX.normal.x)*separation.y;
        separation.z = posZ.distance -
            safeDiv(posZ.normal.y, posZ.normal.z)*separation.y;
        body.contactMask &= 0x0c;
        break;
    }
    default:
        break;
    }

    switch (body.contactMask & 0x0c) {
    case 0x4:
        separation.y = g_contactFaces[2].distance;
        body.floorNormal = g_contactFaces[2].normal;
        break;
    case 0x8:
        separation.y = g_contactFaces[3].distance;
        break;
    case 0x0c: {
        const ContactFace& a = g_contactFaces[2];
        const ContactFace& b = g_contactFaces[3];
        const float denomX = a.normal.x*b.normal.y - b.normal.x*a.normal.y;
        const float denomZ = a.normal.z*b.normal.y - b.normal.z*a.normal.y;
        if (std::fabs(denomX) >= std::fabs(denomZ)) {
            const float x = safeDiv(
                (a.distance-b.distance)*a.normal.y*b.normal.y,
                denomX);
            separation.x = std::copysign(
                std::max(std::fabs(separation.x), std::fabs(x)), x);
            separation.y = a.distance - safeDiv(a.normal.x, a.normal.y)*separation.x;
        } else {
            const float z = safeDiv(
                (a.distance-b.distance)*a.normal.y*b.normal.y,
                denomZ);
            separation.z = std::copysign(
                std::max(std::fabs(separation.z), std::fabs(z)), z);
            separation.y = a.distance - safeDiv(a.normal.z, a.normal.y)*separation.z;
        }
        body.floorNormal = a.normal;
        break;
    }
    default:
        break;
    }

    separation.x = std::clamp(
        separation.x, -body.halfAabb.x*0.5f, body.halfAabb.x*0.5f);
    separation.y = std::clamp(
        separation.y, -body.halfAabb.y*0.5f, body.halfAabb.y*0.5f);
    separation.z = std::clamp(
        separation.z, -body.halfAabb.z*0.5f, body.halfAabb.z*0.5f);
    body.collisionSolveTranslation =
        rotateYxz(separation, body.ownerRotation);
}

void endBodyCollisionTest(TownCollisionBody& body)
{
    if (body.setup.collisionType == 0 || body.setup.collisionType == 1)
        computeCollisionSeparation(body);
}

void processAllCollisions()
{
    for (int list = 3; list >= 0; --list) {
        for (unsigned i = 0; i < g_registry.counts[list]; ++i) {
            TownCollisionBody* body = g_registry.bodies[list][i];
            if (!body || !body->setup.collisionLayersBitField)
                continue;
            beginBodyCollisionTest(*body);
            handleCollisionWithTownEnv(*body);
            endBodyCollisionTest(*body);
        }
    }
}

} // namespace lagi::azel
