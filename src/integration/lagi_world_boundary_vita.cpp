#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/disc_image.h"
#include "lagi/platform.h"
#include "lagi/native_scene_mode.h"
#include "lagi/lagi_render_bridge.h"
#include "audio/systemSounds.h"
#include "lagi/world_map_display.h"
#include "lagi/world_map_motion.h"
#include "kernel/moduleManager.h"
#include "kernel/fade.h"
#include "kernel/cinematicBarsTask.h"
#include "battle/BTL_A3/BTL_A3_map6.h"
#include "kernel/graphicalObject.h"
#include "kernel/vdp1Allocator.h"
#include "mainMenuDebugTasks.h"

void unloadFnt();

static const char* worldAssets[] = {"WORLDMAP.MCB", "WORLDMAP.CGB", nullptr};

// Native mode 6 is WORLD.PRG, not return-to-title. Keep the selected module
// alive while its unported display/travel tasks remain unavailable. Do not run
// the placeholder map initializer: its save-task call resets the map allocator.
struct LagiWorldBoundary : s_workAreaTemplate<LagiWorldBoundary> {
    std::vector<std::uint8_t> program;
    sSaturnMemoryFile worldFile;
    lagi::worldmap::MapDisplayInput destinations;
    lagi::worldmap::MapDisplay display;
    bool displayReady = false;
    bool fontReady = false;
    s32 nameShown = -1;
    bool ownsMapAllocator = false;
    s32 travelStatus = -1;
    int travelDelay = 0;
    int selectionDelay = 15;
    bool waitingForArrival = false;
    fixedPoint arrivalYaw = 0;
    s32 arrivalDistance = 0;
    s32 arrivalHeight = 0;
    s32 arrivalForwardStep = 0;
    s32 arrivalVerticalStep = 0;
    static void Init(LagiWorldBoundary* task) {
        const bool loaded=lagi::disc::read_file("WORLD.PRG",task->program);
        // Validate the complete authored table range before exposing pointers
        // into the task-owned program to future WORLD children.
        constexpr u32 base = 0x06054000;
        constexpr u32 tableEnd = 0x5B2C + 4 * 0x24;
        if (loaded && task->program.size() >= tableEnd) {
            task->worldFile.m_data = task->program.data();
            task->worldFile.m_dataSize = static_cast<u32>(task->program.size());
            task->worldFile.m_base = base;
            const bool fleet = (mainGameState.bitField[0x1B] & 8) != 0;
            const bool destinationsReady = lagi::worldmap::initialize_map_destinations(
                task->destinations, task->worldFile.getSaturnPtr(base), fleet,
                mainGameState.readPackedBits(0x87, 6));
            if (destinationsReady && findFileOnDisc(worldAssets[0]) &&
                findFileOnDisc(worldAssets[1])) {
                resetTempAllocators();
                initDramAllocator(task, townBuffer, sizeof(townBuffer), worldAssets);
                task->ownsMapAllocator = true;
                initVdp1Ram(task, 0x25C18800, 0x62E00);
                auto* owner = allocateNPC(task, 0);
                const bool modelReady = lagi::worldmap::initialize_map_model(
                    task->display.marker, owner, task->worldFile.getSaturnPtr(0x06058774));
                task->displayReady = modelReady && lagi::worldmap::initialize_map_arrival(
                    task->destinations, task->destinations.cameraSelection) &&
                    lagi::worldmap::update_map_display(
                    task->display, task->destinations);
                if (task->displayReady && lagi::native_scene_mode(6)) {
                    task->displayReady = LoadEnvironmentGraphics(task->worldFile);
                    if (task->displayReady) {
                        task->display.marker.flags |= 0x20;
                        auto& fade = g_fadeControls.m0_fade0;
                        fadePalette(&fade, convertColorToU32ForFade(fade.m0_color),
                                    static_cast<s16>(g_fadeControls.m_48), 0x1e);
                    }
                }
            }
        }
        lagi::platform::logging::writef(
            "[WorldMapStartup] status=%02X mode=%d WORLD.PRG loaded=%u bytes=%u displaySeeded=%u; "
            "sceneEnabled=%u\n",
            unsigned(gGameStatus.m4_gameStatus),int(gGameStatus.m0_gameMode),
            loaded?1u:0u,unsigned(task->program.size()),task->displayReady?1u:0u,lagi::native_scene_mode(6)?1u:0u);
    }
    static bool LoadEnvironmentGraphics(sSaturnMemoryFile& worldFile) {
        // WORLD 0605668E..AC loads these four files into VDP2 VRAM.
        constexpr struct { const char* file; u32 offset; u32 capacity; } loads[] = {
            {"FNS_WLD.SCB", 0x10000, 0xE000}, {"FNS_WLD.PNB", 0x1E000, 0x2000},
            {"FRS_WLD.SCB", 0x40000, 0x20000}, {"FRS_WLD.PNB", 0x60000, 0x4000},
        };
        std::vector<std::uint8_t> bytes[4];
        for (unsigned i = 0; i < std::size(loads); ++i) {
            if (!lagi::disc::read_file(loads[i].file, bytes[i]) || bytes[i].empty() ||
                bytes[i].size() > loads[i].capacity)
                return false;
        }
        for (unsigned i = 0; i < std::size(loads); ++i)
            std::memcpy(getVdp2Vram(loads[i].offset), bytes[i].data(), bytes[i].size());
        // WORLD 06059390/0605933C: native NBG0 and RBG0 format descriptors.
        static constexpr sLayerConfig normal[] = {
            {m2_CHCN, 1}, {m5_CHSZ, 1}, {m6_PNB, 1}, {m7_CNSM, 0},
            {m34_W0E, 1}, {m37_W0A, 1}, {m0_END, 0},
        };
        static constexpr sLayerConfig rotation[] = {
            {m2_CHCN, 0}, {m5_CHSZ, 1}, {m6_PNB, 1}, {m7_CNSM, 0},
            {m11_SCN, 8}, {m27_RPMD, 0}, {m34_W0E, 1}, {m37_W0A, 1},
            {m44_CCEN, 1}, {m45_COEN, 0}, {m0_END, 0},
        };
        static constexpr sLayerConfig coefficients[] = {
            {m31_RxKTE, 1}, {m0_END, 0},
        };
        // WORLD 060571D2 uploads the authored palettes before drawing.
        asyncDmaCopy(worldFile.getSaturnPtr(0x06058D1C), getVdp2Cram(0x400), 0x200, 0);
        asyncDmaCopy(worldFile.getSaturnPtr(0x06058F1C), getVdp2Cram(0x600), 0x200, 0);
        asyncDmaCopy(worldFile.getSaturnPtr(0x0605911C), getVdp2Cram(0x000), 0x020, 0);
        asyncDmaCopy(worldFile.getSaturnPtr(0x0605913C), getVdp2Cram(0x200), 0x200, 0);
        setupNBG0(normal);
        setupRGB0(rotation);
        // The recovered RBG helper lacks SPN. WORLD explicitly clears it.
        vdp2Controls.m4_pendingVdp2Regs->m38_PNCR &= 0xFF1F;
        setupRotationParams(coefficients);
        // 060566B6..D6 configures the VRAM bank cycle before loading the
        // native parameter-A plane addresses.
        auto* regs = vdp2Controls.m4_pendingVdp2Regs;
        regs->mE_RAMCTL = (regs->mE_RAMCTL & 0xFF00) | 0x80;
        regs->m10_CYCA0 = 0x310F7544;
        setupRotationMapPlanes(0, worldFile.getSaturnPtr(0x060593D0));
        // 060566D8..E6 maps the same FNS pattern plane to all four slots.
        initLayerMap(0, 0x1E000, 0x1E000, 0x1E000, 0x1E000);
        // WORLD initializes coefficient bank A and the rotation parameter
        // address separately. Use the shared helper so the coefficient bank
        // is also available to beginRotationPass.
        setupVdp2Table(6, coefficientA0, coefficientA1, getVdp2Vram(0x20000), 0x80);
        initRotationCoefficientTables(5, getVdp2Vram(0x2A000));
        return true;
    }
    const char* DestinationName() const {
        const s32 selected = display.selectedTarget;
        if (selected < 0 || selected >= destinations.targetCount)
            return nullptr;
        const u32 address = readSaturnU32(destinations.targetTable + selected * 0x24 + 0x20);
        if (address < worldFile.m_base || address - worldFile.m_base >= program.size())
            return nullptr;
        const auto offset = address - worldFile.m_base;
        const char* name = reinterpret_cast<const char*>(program.data() + offset);
        return std::memchr(name, 0, program.size() - offset) ? name : nullptr;
    }

    void UpdateDestinationName() {
        if (!fontReady || nameShown == display.selectedTarget)
            return;
        nameShown = display.selectedTarget;
        setupVDP2StringRendering(2, 0x16, 0x28, 4);
        vdp2StringContext.m0 = 0;
        // Clear the previous label's fixed-width row before replacing it.
        drawStringSmallFont("                                        ");
        setupVDP2StringRendering(2, 0x16, 0x28, 4);
        if (const char* name = DestinationName())
            drawStringSmallFont(name);
    }
    static void Update(LagiWorldBoundary* task) {
        if (!task->displayReady || !lagi::native_scene_mode(6))
            return;

        if (!task->fontReady && findFileOnDisc("WORLDMAP.FNT")) {
            loadFnt("WORLDMAP.FNT");
            task->fontReady = true;
        }
        auto& display = task->display;
        if (task->waitingForArrival) {
            auto& arrival = task->destinations;
            if (task->arrivalDistance == 0xCCC0) {
                lagi::worldmap::update_map_arrival_launch(arrival, task->arrivalYaw,
                    task->arrivalHeight, task->arrivalForwardStep, task->arrivalVerticalStep);
                // Refresh the measured distance after moving. Holding the
                // initial launch distance forever would pin this phase.
                lagi::worldmap::retarget_map_arrival(arrival, display.selectedTarget,
                    task->arrivalYaw, task->arrivalDistance, task->arrivalHeight);
            } else {
                lagi::worldmap::update_map_arrival_step(task->arrivalDistance,
                    task->arrivalForwardStep, arrival.arrivalBusy);
            }
            if (arrival.arrivalBusy) {
                task->waitingForArrival = false;
                auto& fade = g_fadeControls.m0_fade0;
                fadePalette(&fade, convertColorToU32ForFade(fade.m0_color),
                            task->travelStatus == 0x66 ? 0xFFFF : 0x8000, 0x3c);
            }
        } else if (task->travelStatus >= 0) {
            auto& fade = g_fadeControls.m0_fade0;
            if (task->travelDelay > 0) {
                if (--task->travelDelay == 0)
                    fadePalette(&fade, convertColorToU32ForFade(fade.m0_color), 0x8000, 0x1e);
            } else if (fade.m20_stopped && gGameStatus.m8_nextGameStatus == 0) {
                lagi::platform::logging::writef(
                    "[WorldMapTravel] dispatch selected=%d status=%02X\n",
                    int(display.selectedTarget), unsigned(task->travelStatus));
                setNextGameStatus(task->travelStatus);
                task->getTask()->markFinished();
                return;
            }
        }
        // WORLD camera state 2 waits 15 updates before accepting input.
        if (task->selectionDelay > 0)
            --task->selectionDelay;
        else if (display.state == 1) {
            const auto& pad = graphicEngineStatus.m4514.m0_inputDevices[0].m0_current;
            const auto entry = task->destinations.targetTable + display.selectedTarget * 0x24;
            const auto destination = lagi::worldmap::read_map_destination(entry);
            // WORLD camera states 7 and 9 use the close-up path for types 2/3.
            // Types 0/1 use the arrival camera and the child completion flag.
            if ((pad.m8_newButtonDown & 6) && destination.canTravel() &&
                (destination.type >= 2 || task->destinations.arrivalReady) &&
                (destination.storyBit <= 0 ||
                 (unsigned(destination.storyBit) / 8 < sizeof(mainGameState.bitField) &&
                  mainGameState.getBit(destination.storyBit)))) {
                if (destination.type >= 2) {
                    task->travelStatus = destination.gameStatus;
                    task->travelDelay = 15;
                    display.state = 2;
                } else if (lagi::worldmap::retarget_map_arrival(task->destinations,
                               display.selectedTarget, task->arrivalYaw,
                               task->arrivalDistance, task->arrivalHeight)) {
                    task->travelStatus = destination.gameStatus;
                    task->waitingForArrival = true;
                    task->arrivalForwardStep = 0;
                    task->arrivalVerticalStep = 0;
                    display.state = 3;
                }
            }
            constexpr unsigned masks[] = {0x10, 0x80, 0x20, 0x40};
            s32 selected = display.selectedTarget;
            for (unsigned direction = 0; direction < 4; ++direction) {
                if (task->travelStatus < 0 && (pad.mC_newButtonDown2 & masks[direction]))
                    selected = lagi::worldmap::find_map_destination(
                        task->destinations, display.selectedTarget, direction);
            }
            if (selected != display.selectedTarget) {
                display.selectedTarget = selected;
                if (pad.m8_newButtonDown & 0xF0)
                    display.marker.flags |= 1;
                else
                    display.marker.flags |= 2 | 8;
                playSystemSoundEffect(10);
            }
        }
        if (lagi::worldmap::update_map_display(display, task->destinations)) {
            task->UpdateDestinationName();
            lagi::worldmap::apply_map_camera_and_light(display);
            lagi::azel_bridge::capture_native_scene_view_matrix();
        }
    }

    static bool BeginEnvironmentRotation() {
        // WORLD 06056DDE..42 queries the active clipping and projection.
        std::array<s16, 4> clip{};
        s16 projectionX = 0;
        s16 projectionY = 0;
        getVdp1ClippingCoordinates(clip);
        getVdp1ProjectionParams(&projectionX, &projectionY);
        if (!projectionY)
            return false;

        beginRotationPass(0, intDivide(projectionX, fixedPoint::fromInteger(projectionY)));
        // WORLD 0605706E..AA uses the clipping midpoint with signed
        // truncation toward zero and places the viewpoint at projectionY.
        auto& table = gCoefficientTables[0][vdp2Controls.m0_doubleBufferIndex];
        table.m34 = (s32(clip[0]) + clip[2]) / 2;
        table.m36 = (s32(clip[1]) + clip[3]) / 2;
        table.m38 = projectionY;
        table.m3C = table.m34;
        table.m3E = table.m36;
        table.m40 = 0;
        return true;
    }
    static void TranslateEnvironmentRotation(const sVec3_FP& camera,
                                             fixedPoint scale, fixedPoint planeHeight) {
        // WORLD 060570DE..71A0 subtracts the rotated viewpoint offset
        // and the pivot position from the camera-derived map translation.
        const auto& table = gCoefficientTables[0][vdp2Controls.m0_doubleBufferIndex];
        const s32 delta[] = {table.m34 - table.m3C, table.m36 - table.m3E,
                             table.m38 - table.m40};
        const s32 pivot[] = {table.m3C, table.m3E, table.m40};
        const s32 origin[] = {
            MTH_Mul(scale, s32(u32(camera[0].asS32()) << 4)),
            MTH_Mul(scale, s32(u32(camera[2].asS32()) << 4)),
            s32((u32(camera[1].asS32()) - u32(planeHeight.asS32())) << 4),
        };

        auto& matrix = gVdp2RotationMatrix;
        fixedPoint* const translation[] = {&matrix.Mx, &matrix.My, &matrix.Mz};
        for (unsigned row = 0; row < 3; ++row) {
            u32 value = u32(origin[row]) - (u32(pivot[row]) << 16);
            for (unsigned column = 0; column < 3; ++column)
                value -= u32(matrix.m[row][column].asS32()) * u32(delta[column]);
            *translation[row] = s32(value);
        }
    }
    static bool DrawEnvironmentPassA(const lagi::worldmap::MapDisplay& display,
                                     fixedPoint scale, fixedPoint planeHeight) {
        if (!BeginEnvironmentRotation())
            return false;

        // WORLD 060570B4..DA applies pitch/yaw, scale, and then roll.
        buildRotationMatrixPitchYaw(-0x4000000 - display.rotation[0].asS32(),
                                    -display.rotation[1].asS32());
        scaleRotationMatrix(scale);
        writeRotationParams(-display.rotation[2].asS32());
        TranslateEnvironmentRotation(display.camera, scale, planeHeight);
        drawCinematicBar(6);
        commitRotationPass();
        return true;
    }
    static void Draw(LagiWorldBoundary* task) {
        if (!task->displayReady || !lagi::native_scene_mode(6))
            return;

        // Retail WORLD environment defaults at 0605676E and 060568DA.
        DrawEnvironmentPassA(task->display, fixedPoint(0x5B333), fixedPoint(-81));
        lagi::worldmap::draw_map_model(task->display.marker);
    }
    static void Delete(LagiWorldBoundary* task) {
        if (task->fontReady)
            unloadFnt();
        if (task->ownsMapAllocator) {
            freeRamResources(task);
            vdp1FreeLastAllocation(task);
        }
    }
    static const TypedTaskDefinition* getTypedTaskDefinition() {
        static const TypedTaskDefinition definition={Init,Update,Draw,Delete};
        return &definition;
    }
};

p_workArea lagiLoadNativeWorldBoundary(p_workArea parent,s32)
{
    // Return a real child, never the manager itself as upstream's unfinished
    // loadWorldOverlay currently does. No setNextGameStatus/fallback is issued.
    return createSubTask<LagiWorldBoundary>(parent);
}
