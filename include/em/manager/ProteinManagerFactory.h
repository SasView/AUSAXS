#pragma once

#include <em/manager/ProteinManager.h>

#include <memory>

namespace em::factory {
    std::unique_ptr<em::managers::ProteinManager> create_manager(const ImageStackBase* images);
}