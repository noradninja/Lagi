#pragma once
#include <cstdint>
#include <limits>

namespace neptune {
// One native scene transaction at a time. Publication counters are NOT
// completion fences. An ambiguous scene boundary disables reuse until the
// context is destroyed; intermediate finishes must not close a transaction.
class GeometrySubmissionEpoch {
public:
    std::uint64_t begin() noexcept {
        if (failed_ || active_ || issued_ == std::numeric_limits<std::uint64_t>::max()) {
            fail();
            return 0;
        }
        active_ = ++issued_;
        return active_;
    }
    std::uint64_t active() const noexcept { return failed_ ? 0 : active_; }
    bool failed() const noexcept { return failed_; }
    void fail() noexcept { failed_ = true; active_ = 0; }
    std::uint64_t complete(bool confirmed) noexcept {
        if (!confirmed || failed_ || !active_) {
            fail();
            return 0;
        }
        const auto completed = active_;
        active_ = 0;
        return completed;
    }
private:
    std::uint64_t issued_ = 0, active_ = 0;
    bool failed_ = false;
};
} // namespace neptune
