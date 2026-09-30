namespace lagi::azel { struct StaticRoomDebugMesh; struct BasicWingDebugMesh; }
#pragma once
namespace lagi::platform {
bool init(); void shutdown(); bool running(); void begin_frame(); void end_frame();
namespace input { bool init(); void update(); bool exit_requested(); float analog_x(); float analog_y(); float analog_zoom(); bool reset_view_pressed(); bool prev_mode_pressed(); bool next_mode_pressed(); bool resolution_toggle_pressed(); }
namespace filesystem { bool init(); const char* data_root(); bool game_data_present(); }
namespace logging { bool init(); void shutdown(); void writef(const char* format, ...); const char* path(); bool ready(); int last_error(); }
namespace renderer { bool init(); void shutdown(); void begin_frame(); void end_frame(); void set_azel_alive(bool alive); void set_disc_alive(bool alive); void status(const char* text, unsigned int color); void failure(const char* text); void toggle_debug_console(); bool debug_console_visible(); bool load_basic_wing_viewer(); bool load_edge_idle_model(const lagi::azel::BasicWingDebugMesh& mesh); bool load_static_room_viewer(const lagi::azel::StaticRoomDebugMesh& mesh); }
namespace audio { bool init(); void shutdown(); void update(); }
}
