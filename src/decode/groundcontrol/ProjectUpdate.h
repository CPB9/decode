#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/groundcontrol/AllowUnsafeMessageType.h"

namespace decode {

class Project;
class ValueInfoCache;
struct Device;

struct ProjectUpdate {
    ProjectUpdate(const Project* project, const Device* device, const ValueInfoCache* cache);
    ProjectUpdate(const ProjectUpdate& other);
    ProjectUpdate(ProjectUpdate&& other);
    ~ProjectUpdate();

    Rc<const Project> project;
    Rc<const Device> device;
    Rc<const ValueInfoCache> cache;
};
}

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::ProjectUpdate);
