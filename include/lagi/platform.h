#pragma once
namespace lagi::platform {
bool init(); void shutdown(); bool running(); void begin_frame(); void end_frame();
namespace input { bool init(); void update(); bool exit_requested(); }
namespace filesystem { bool init(); const char* data_root(); bool game_data_present(); }
namespace renderer { bool init(); void shutdown(); void begin_frame(); void end_frame(); }
namespace audio { bool init(); void shutdown(); void update(); }
}
