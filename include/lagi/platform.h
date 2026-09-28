#pragma once
namespace lagi::platform {
bool init(); void shutdown(); bool running(); void begin_frame(); void end_frame();
namespace input { bool init(); void update(); bool exit_requested(); }
namespace filesystem { bool init(); const char* data_root(); bool game_data_present(); }
namespace renderer { bool init(); void shutdown(); void begin_frame(); void end_frame(); void set_azel_alive(bool alive); void set_disc_alive(bool alive); void status(const char* text, unsigned int color); void failure(const char* text); void toggle_debug_console(); bool debug_console_visible(); }
namespace audio { bool init(); void shutdown(); void update(); }
}
