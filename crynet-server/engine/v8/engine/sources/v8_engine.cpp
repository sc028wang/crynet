#include "engine/v8/engine/includes/v8_engine.h"

#include <v8.h>

namespace crynet::engine::v8 {

bool V8Engine::vendor_ready() noexcept {
    return true;
}

std::string_view V8Engine::engine_name() noexcept {
    return ::v8::V8::GetVersion();
}

}  // namespace crynet::engine::v8
