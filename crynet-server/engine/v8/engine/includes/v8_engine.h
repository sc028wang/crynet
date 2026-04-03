#pragma once


#include "engine/base/includes/ScriptEngine.h"

namespace crynet::engine {

/**
 * @cn
 * `V8` 引擎占位封装，用于在 T0 阶段确认三方依赖接入链路可用。
 *
 * @en
 * Placeholder `V8` engine wrapper used during T0 to validate the third-party integration path.
 */
class V8Engine final : public ScriptEngine {

};

}  // namespace crynet::engine
