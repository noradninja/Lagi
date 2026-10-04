#pragma once
#include <cstddef>
#include <cstdint>

namespace lagi::azel { struct StaticRoomDebugMesh; struct BasicWingDebugMesh; }
namespace lagi::platform {
bool init(); void shutdown(); bool running(); void begin_frame(); void end_frame();
namespace input { bool init(); void update(); bool exit_requested(); unsigned short saturn_buttons_down(); unsigned short saturn_buttons_pressed(); float analog_x(); float analog_y(); float analog_zoom(); float analog_camera_x(); float analog_camera_y(); int digital_x(); int digital_y(); bool run_held(); bool camera_held(); bool reset_scene_pressed(); bool prev_mode_pressed(); bool next_mode_pressed(); }
namespace filesystem { bool init(); const char* data_root(); bool game_data_present(); }
namespace logging { bool init(); void shutdown(); void writef(const char* format, ...); const char* path(); bool ready(); int last_error(); }
namespace renderer { bool init(); void shutdown(); void begin_frame(); void end_frame(); void set_fov(float degrees); void invalidate_cram_range(unsigned int start, unsigned int size); void invalidate_vdp1_texture_range(unsigned int start, unsigned int size); void set_azel_alive(bool alive); void set_disc_alive(bool alive); void status(const char* text, unsigned int color); void failure(const char* text); void show_town_scene(); void toggle_debug_console(); void toggle_full_debug_screen(); bool debug_console_visible(); bool load_basic_wing_viewer(); bool load_edge_idle_model(lagi::azel::BasicWingDebugMesh&& mesh); bool load_edge_shadow_model(lagi::azel::BasicWingDebugMesh&& mesh); bool load_static_room_viewer(const lagi::azel::StaticRoomDebugMesh& mesh); bool town_scene_active(); void town_profile_tasks_us(unsigned int microseconds); void town_wait_render_slot(); void town_publish_frame(); void town_fade_in(unsigned int frames); void town_fade_out(unsigned int frames); void town_camera_update(); unsigned town_edge_animation_frames(unsigned animation); void town_present_edge(float x, float y, float z, float yaw, bool grounded, unsigned contacts, unsigned animation, unsigned frame, unsigned previousAnimation, unsigned previousFrame, float transition); void town_present_camera(const float position[3], const float rawPosition[3], const float target[3], const float up[3], float yaw, float pitch, float distance); void town_present_vdp2_text(const unsigned char* vram, const unsigned char* cram, const unsigned char* lineScroll); bool movie_present_frame(const std::uint32_t* rgba, unsigned int width, unsigned int height, unsigned int pitchPixels); bool movie_present_cinepak_payload(const std::uint32_t* payload, unsigned int payloadWidth, unsigned int payloadHeight, unsigned int sourceWidth, unsigned int sourceHeight); bool movie_republish_frame(); bool frontend_present_vdp2(const unsigned char* vram, const unsigned char* cram, unsigned int layout, int scrollX, int scrollY, unsigned int flags); void set_azel_color_offset_state(
    unsigned int enableMask,
    unsigned int selectMask,
    int aRed, int aGreen, int aBlue,
    int bRed, int bGreen, int bBlue); void movie_clear_frame(); }
namespace audio {
bool init();
void shutdown();
void update();
bool start_pcm_stream(unsigned int sampleRate, unsigned int channels);
std::size_t write_pcm_frames(const std::int16_t* interleaved, std::size_t frames);
void finish_pcm_stream();
void stop_pcm_stream();
std::uint64_t played_pcm_frames();
std::size_t queued_pcm_frames();
bool pcm_stream_drained();
bool pcm_stream_active();
}
}
