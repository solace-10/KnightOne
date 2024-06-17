#pragma once

#include "vfs/vfs.hpp"

namespace WingsOfSteel::Pandora::Private
{

class VFSImpl
{
public:
    VFSImpl() {}
    virtual ~VFSImpl() {}

    virtual bool FileExists(const std::string& path) const = 0;
    virtual void FileRead(const std::string& path, FileReadCallback onFileReadCompleted) = 0;
};

} // namespace WingsOfSteel::Pandora::Private