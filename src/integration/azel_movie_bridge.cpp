#include "lagi/azel_movie_bridge.h"

#include "lagi/movie/movie_player.h"

namespace lagi::azel {
namespace {

movie::MoviePlayer g_moviePlayer;

} // namespace

bool movie_backend_open(const char* path)
{
    return g_moviePlayer.open(path);
}

void movie_backend_update(std::uint64_t elapsedMicroseconds)
{
    g_moviePlayer.update(elapsedMicroseconds);
}

void movie_backend_close()
{
    g_moviePlayer.close();
}

bool movie_backend_active()
{
    return g_moviePlayer.active();
}

bool movie_backend_finished()
{
    return g_moviePlayer.finished();
}

std::uint64_t movie_backend_pts()
{
    return g_moviePlayer.current_video_pts();
}

std::uint32_t movie_backend_video_width()
{
    return g_moviePlayer.video_width();
}

std::uint32_t movie_backend_video_height()
{
    return g_moviePlayer.video_height();
}

const char* movie_backend_error()
{
    return g_moviePlayer.last_error().c_str();
}

} // namespace lagi::azel
