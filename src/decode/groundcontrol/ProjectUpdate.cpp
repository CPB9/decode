#include "decode/groundcontrol/ProjectUpdate.h"
#include "decode/parser/Project.h"
#include "decode/model/ValueInfoCache.h"

namespace decode {

ProjectUpdate::ProjectUpdate(const Project* project, const Device* device, const ValueInfoCache* cache)
    : project(project)
    , device(device)
    , cache(cache)
{
}

ProjectUpdate::ProjectUpdate(const ProjectUpdate& other)
    : project(other.project)
    , device(other.device)
    , cache(other.cache)
{
}

ProjectUpdate::ProjectUpdate(ProjectUpdate&& other)
    : project(std::move(other.project))
    , device(std::move(other.device))
    , cache(std::move(other.cache))
{
}

ProjectUpdate::~ProjectUpdate()
{
}
}
