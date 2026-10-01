namespace lagi::azel { struct StaticRoomDebugMesh; struct BasicWingDebugMesh; }
#pragma once
namespace lagi::platform {
bool init(); void shutdown(); bool running(); void begin_frame(); void end_frame();
namespace input { bool init(); void update(); bool exit_requested(); float analog_x(); float analog_y(); float analog_zoom(); int digital_x(); int digital_y(); bool run_held(); bool reset_view_pressed(); bool prev_mode_pressed(); bool next_mode_pressed(); }
namespace filesystem { bool init(); const char* data_root(); bool game_data_present(); }
namespace logging { bool init(); void shutdown(); void writef(const char* format, ...); const char* path(); bool ready(); int last_error(); }
namespace renderer { bool init(); void shutdown(); void begin_frame(); void end_frame(); void set_azel_alive(bool alive); void set_disc_alive(bool alive); void status(const char* text, unsigned int color); void failure(const char* text); void toggle_debug_console(); bool debug_console_visible(); bool load_basic_wing_viewer(); bool load_edge_idle_model(const lagi::azel::BasicWingDebugMesh& mesh); bool load_static_room_viewer(const lagi::azel::StaticRoomDebugMesh& mesh); bool town_scene_active(); void town_camera_update(); unsigned town_edge_animation_frames(unsigned animation); void town_present_edge(float x, float y, float z, float yaw, bool grounded, unsigned contacts, unsigned animation, unsigned frame, unsigned previousAnimation, unsigned previousFrame, float transition); void town_present_camera(const float position[3], const float rawPosition[3], const float target[3], const float up[3], float yaw, float pitch, float distance); }
namespace audio { bool init(); void shutdown(); void update(); }
}
