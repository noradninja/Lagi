#include "lagi/lagi_azel_upstream_prelude.h"
#include "lagi/disc_image.h"
#include "lagi/prepared_disc_files.h"
#include "lagi/platform.h"
#include "lagi/native_scene_mode.h"
#include "lagi/lagi_render_bridge.h"
#include "audio/systemSounds.h"
#include "lagi/world_map_display.h"
#include "lagi/world_map_motion.h"
#include "lagi/world_map_environment.h"
#include "lagi/world_map_arrival.h"
#include "lagi/world_map_message.h"
#include "kernel/moduleManager.h"
#include "kernel/fade.h"
#include "kernel/cinematicBarsTask.h"
#include "battle/BTL_A3/BTL_A3_map6.h"
#include "kernel/graphicalObject.h"
#include "kernel/vdp1Allocator.h"
#include "trigo.h"
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
    lagi::worldmap::MapEnvironment environment;
    lagi::worldmap::MapArrivalAnimation arrivalAnimation;
    bool displayReady = false;
    bool fontReady = false;
    lagi::worldmap::MapMessage message;
    bool ownsMapAllocator = false;
    s_workAreaCopy* modelOwner = nullptr;
    s32 travelStatus = -1;
    int travelDelay = 0;
    int selectionDelay = 15;
    bool waitingForArrival = false;
    bool preparedFilesHandedOff = false;
    bool waitingForMapToggle = false;
    fixedPoint arrivalYaw = 0;
    s32 arrivalDistance = 0;
    s32 arrivalHeight = 0;
    s32 arrivalForwardStep = 0;
    s32 arrivalVerticalStep = 0;
    static void Init(LagiWorldBoundary* task) {
        lagi::disc::clear_prepared_native_files();
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
                task->modelOwner = allocateNPC(task, 0);
                const bool modelReady = lagi::worldmap::initialize_map_model(
                    task->display.marker, task->modelOwner, task->worldFile.getSaturnPtr(0x06058774));
                if (fleet) task->display.marker.flags |= 0x40;
                task->displayReady = modelReady && lagi::worldmap::initialize_map_arrival(
                    task->destinations, task->destinations.cameraSelection) &&
                    lagi::worldmap::initialize_map_arrival_animation(task->arrivalAnimation, task) &&
                    lagi::worldmap::update_map_display(
                    task->display, task->destinations);
                if (task->displayReady && lagi::native_scene_mode(6)) {
                    task->displayReady = LoadEnvironmentGraphics(task->worldFile);
                    if (task->displayReady) {
                        // WORLD entry 06054016..54 initializes its own
                        // projection and clipping distances.
                        reset3dEngine();
                        auto& projection = graphicEngineStatus.m405C;
                        projection.m10_nearClipDistance = 0x800;
                        projection.m30_oneOverNearClip = FP_Div(0x10000, projection.m10_nearClipDistance);
                        projection.m14_farClipDistance = 0x400000;
                        projection.m38_oneOverFarClip = FP_Div(0x8000, projection.m14_farClipDistance);
                        projection.m34_oneOverFarClip256 = projection.m38_oneOverFarClip << 8;
                        task->display.marker.flags |= 0x20;
                        BeginSelectionFadeIn();
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
    static void BeginSelectionFadeIn() {
        // WORLD camera state 1 (06057ED0) and state FF clear both CLOFSL
        // buffers before fading A back in. Inherited B can remain black
        // after field exit, hiding an otherwise submitted RBG0 terrain.
        if (static_cast<std::int8_t>(g_fadeControls.m_4C) <=
            static_cast<std::int8_t>(g_fadeControls.m_4D)) {
            vdp2Controls.m20_registers[0].m112_CLOFSL = 0;
            vdp2Controls.m20_registers[1].m112_CLOFSL = 0;
            vdp2Controls.m_isDirty = true;
        }
        auto& fade = g_fadeControls.m0_fade0;
        fadePalette(&fade, convertColorToU32ForFade(fade.m0_color),
                    static_cast<s16>(g_fadeControls.m_48), 0x1e);
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
        // Reset the old scene tables and establish the VDP2 text layer.
        reinitVdp2();
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
        // WORLD 06056708..6A supplies black line/back colors and its own
        // VDP1 and VDP2 priority bands. Do not inherit the outgoing scene.
        const u16 black = 0;
        std::memcpy(getVdp2Vram(0x2A400), &black, sizeof(black));
        std::memcpy(getVdp2Vram(0x2A600), &black, sizeof(black));
        regs->mA8_LCTA = (regs->mA8_LCTA & 0xFFF80000) | 0x15200;
        regs->mAC_BKTA = (regs->mAC_BKTA & 0xFFF80000) | 0x15300;
        regs->mF0_PRISA = 0x0404;
        regs->mF2_PRISB = 0x0407;
        regs->mF4_PRISC = 0x0404;
        regs->mF6_PRISD = 0x0602;
        regs->mF8_PRINA = 0x0602;
        regs->mFA_PRINB = 0x0700;
        regs->mFC_PRIR = 5;
        vdp2Controls.m_isDirty = true;
        return true;
    }
    static void Update(LagiWorldBoundary* task) {
        if (!task->displayReady || !lagi::native_scene_mode(6))
            return;

        if (!task->fontReady && findFileOnDisc("WORLDMAP.FNT")) {
            loadFnt("WORLDMAP.FNT");
            setupVDP2StringRendering(2, 0x16, 0x28, 4);
            vdp2StringContext.m0 = 0;
            task->fontReady = true;
        }
        auto& display = task->display;
        const auto& pad = graphicEngineStatus.m4514.m0_inputDevices[0].m0_current;
        lagi::worldmap::apply_map_orbit_buttons(display,pad.m8_newButtonDown);
        bool returnedToSelection = false;
        if (task->waitingForMapToggle && g_fadeControls.m0_fade0.m20_stopped) {
            // Native camera state FF waits for fade completion before changing
            // the table, then uses return-to-selection (display state 4).
            lagi::worldmap::toggle_map_destinations(task->destinations, display);
            task->waitingForMapToggle = false;
            display.marker.flags |= 0x20;
            display.state = 4;
            // Native camera re-entry initializes message mode directly.
            task->message.mode=3;task->message.queuedMode=-1;
            task->selectionDelay = 15;
            lagi::disc::clear_prepared_native_files();
            BeginSelectionFadeIn();
            returnedToSelection = true;
        } else if (task->waitingForArrival && task->destinations.arrivalBusy) {
            task->waitingForArrival = false;
            auto& fade = g_fadeControls.m0_fade0;
            fadePalette(&fade, convertColorToU32ForFade(fade.m0_color),
                        task->travelStatus == 0x66 ? 0xFFFF : 0x8000, 0x3c);
        } else if (!task->waitingForArrival && task->travelStatus >= 0) {
            auto& fade = g_fadeControls.m0_fade0;
            if (task->travelDelay > 0) {
                if (--task->travelDelay == 0)
                    fadePalette(&fade, convertColorToU32ForFade(fade.m0_color), 0x8000, 0x1e);
            } else if (fade.m20_stopped && gGameStatus.m8_nextGameStatus == 0) {
                lagi::platform::logging::writef(
                    "[WorldMapTravel] dispatch selected=%d status=%02X\n",
                    int(display.selectedTarget), unsigned(task->travelStatus));
                setNextGameStatus(task->travelStatus);
                task->preparedFilesHandedOff = true;
                task->getTask()->markFinished();
                return;
            }
        }
        // WORLD camera state 2 waits 15 updates before accepting input.
        if (task->waitingForMapToggle || returnedToSelection) {
            // No confirmation or navigation during the native table-switch fade.
        } else if (task->selectionDelay > 0)
            --task->selectionDelay;
        else if (display.state == 1) {
            const auto entry = task->destinations.targetTable + display.selectedTarget * 0x24;
            const auto destination = lagi::worldmap::read_map_destination(entry);
            // WORLD camera states 7 and 9 use the close-up path for types 2/3.
            // Types 0/1 use the arrival camera and the child completion flag.
            if ((pad.m8_newButtonDown & 6) &&
                (destination.storyBit <= 0 ||
                 (unsigned(destination.storyBit) / 8 < sizeof(mainGameState.bitField) &&
                  mainGameState.getBit(destination.storyBit)))) {
                if (destination.gameStatus < 0) {
                    clearVdp2TextArea();
                    playSystemSoundEffect(0);
                    auto& fade = g_fadeControls.m0_fade0;
                    fadePalette(&fade, convertColorToU32ForFade(fade.m0_color), 0, 0x1e);
                    task->waitingForMapToggle = true;
                } else if (destination.canTravel() &&
                           (destination.type >= 2 || task->destinations.arrivalReady) &&
                           lagi::worldmap::prepare_map_travel(destination)) {
                    clearVdp2TextArea();
                    playSystemSoundEffect(0);
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
            }
            constexpr unsigned masks[] = {0x10, 0x80, 0x20, 0x40};
            s32 selected = display.selectedTarget;
            for (unsigned direction = 0; direction < 4; ++direction) {
                if (!task->waitingForMapToggle && task->travelStatus < 0 && (pad.mC_newButtonDown2 & masks[direction]))
                    selected = lagi::worldmap::find_map_destination(
                        task->destinations, display.selectedTarget, direction);
            }
            if (selected != display.selectedTarget) {
                display.selectedTarget = selected;
                lagi::worldmap::request_map_message(task->message,3);
                if (pad.m8_newButtonDown & 0xF0)
                    display.marker.flags |= 1;
                else
                    display.marker.flags |= 2 | 8;
                playSystemSoundEffect(10);
            }
        }
        if (lagi::worldmap::update_map_display(display, task->destinations)) {
            lagi::worldmap::apply_map_camera_and_light(display);
            lagi::azel_bridge::capture_native_scene_view_matrix();
            if (task->fontReady)
                lagi::worldmap::update_map_message(task->message,task,
                    task->worldFile.getSaturnPtr(0x06054000),
                    task->destinations.targetTable,task->destinations.targetCount,
                    display.selectedTarget,task->travelStatus>=0 || task->waitingForMapToggle);
        }
        lagi::worldmap::advance_map_environment(task->environment);
        // Native child order is camera, display, environment, then arrival.
        // Consume the old child's position/flag above and advance it only
        // afterward. Phase 2 keeps moving during the travel fade as well.
        if (task->destinations.arrivalReady &&
            lagi::worldmap::retarget_map_arrival(task->destinations,display.selectedTarget,
                task->arrivalYaw,task->arrivalDistance,task->arrivalHeight)) {
            lagi::worldmap::update_map_arrival_animation(task->arrivalAnimation);
            lagi::worldmap::update_map_arrival_heading(task->destinations,
                task->arrivalYaw,task->arrivalDistance);
            if (task->travelStatus >= 0 && display.state == 3)
                lagi::worldmap::update_map_arrival_motion(task->destinations,
                    task->destinations.arrivalRotation[1],task->arrivalDistance,
                    task->arrivalHeight,task->arrivalForwardStep,task->arrivalVerticalStep);
        }
    }

    static bool BeginEnvironmentRotation() {
        // WORLD 06056DDE..42 queries the active clipping and projection.
        std::array<s16, 4> clip{};
        s16 projectionX = 0;
        s16 projectionY = 0;
        getVdp1ClippingCoordinates(clip);
        // WORLD 06056F06..26 derives window 0 from the live VDP1 clip.
        // VDP2 stores horizontal coordinates in half-dot units.
        auto* regs = vdp2Controls.m4_pendingVdp2Regs;
        regs->mC0_WPSX0 = static_cast<u16>(s32(clip[0]) * 2);
        regs->mC2_WPSY0 = static_cast<u16>(clip[1]);
        regs->mC4_WPEX0 = static_cast<u16>(s32(clip[2]) * 2);
        regs->mC6_WPEY0 = static_cast<u16>(clip[3]);
        vdp2Controls.m_isDirty = true;
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
                                     const lagi::worldmap::MapEnvironment& environment) {
        if (!BeginEnvironmentRotation())
            return false;

        const auto camera=lagi::worldmap::map_environment_camera(
            {display.camera[0].asS32(),display.camera[1].asS32(),display.camera[2].asS32()},
            environment,[](s32 numerator,s32 denominator){
                return FP_Div(numerator,fixedPoint(denominator)).asS32();
            });
        const sVec3_FP environmentCamera={camera[0],camera[1],camera[2]};
        // WORLD 06056FD0..06057024 supplies -0xB60B6 for an exactly
        // zero pitch before building its environment-only rotation matrix.
        const s32 pitch=display.rotation[0].asS32()!=0
            ? display.rotation[0].asS32() : -0xB60B6;
        // WORLD 060570B4..DA applies pitch/yaw, scale, and then roll.
        buildRotationMatrixPitchYaw(-0x4000000 - pitch,
                                    -display.rotation[1].asS32());
        scaleRotationMatrix(fixedPoint(environment.scale));
        writeRotationParams(-display.rotation[2].asS32());
        TranslateEnvironmentRotation(environmentCamera,fixedPoint(environment.scale),
                                     fixedPoint(environment.planeHeight));
        drawCinematicBar(6);
        commitRotationPass();
        return true;
    }
    static void Draw(LagiWorldBoundary* task) {
        if (!task->displayReady || !lagi::native_scene_mode(6))
            return;

        if (!DrawEnvironmentPassA(task->display,task->environment))
            return;
        const auto& table=gCoefficientTables[0][vdp2Controls.m0_doubleBufferIndex];
        const auto& row=gVdp2RotationMatrix.m[2];
        const auto ground=lagi::worldmap::map_projected_ground(
            {row[0].asS32(),row[1].asS32(),row[2].asS32()},
            {table.m34,table.m36,table.m38},table.m8_Zst,
            [](s32 numerator,s32 denominator){
                return FP_Div(numerator,fixedPoint(denominator)).asS32();
            });
        auto* coefficients=gVdp2CoefficientTables[0][vdp2Controls.m0_doubleBufferIndex];
        if (!coefficients || !lagi::worldmap::apply_map_coefficient_wave(
                std::span(*coefficients),task->environment.wave,
                [](unsigned index){return getSin(index).asS32();}))
            return;
        const auto origin=lagi::worldmap::map_normal_background_origin(
            task->display.rotation[1].asS32(),ground,task->environment.verticalOffset);
        auto* regs=vdp2Controls.m4_pendingVdp2Regs;
        regs->m70_SCXN0=origin.horizontal;
        regs->m74_SCYN0=origin.scrollY;
        // WORLD 06056F28..7C, with native environment +69 initially zero.
        regs->mF8_PRINA=0x0602;
        regs->mFA_PRINB=0x0700;
        regs->mFC_PRIR=task->display.camera[1].asS32()>task->environment.planeHeight?5:3;
        vdp2Controls.m_isDirty=true;
        lagi::azel_bridge::begin_view_relative_submission_scope();
        lagi::worldmap::draw_map_trail(task->display, task->modelOwner);
        lagi::worldmap::draw_map_model(task->display.marker);
        lagi::worldmap::draw_map_arrival(task->arrivalAnimation,task->destinations,
                                       task->display.camera[1].asS32());
        lagi::azel_bridge::end_view_relative_submission_scope();
    }
    static void Delete(LagiWorldBoundary* task) {
        // The native task walker tears down owned children before this callback.
        if (!task->preparedFilesHandedOff)
            lagi::disc::clear_prepared_native_files();
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
