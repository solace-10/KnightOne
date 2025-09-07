#pragma once

#if defined(TARGET_PLATFORM_WEB)

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace WingsOfSteel::Private
{

class ManifestEntry;

class Manifest
{
public:
    Manifest();
    ~Manifest();

    void Initialize();

    bool IsValid() const;
    ManifestEntry* GetEntry(const std::string& path) const;

private:
    void OnDownloadSucceeded(const std::string& url, const char* pData, size_t dataSize);
    void OnDownloadFailed(const std::string& url, int statusCode);

    bool m_IsValid;
    using ManifestEntryUniquePtr = std::unique_ptr<ManifestEntry>;
    using ManifestData = std::unordered_map<std::string, ManifestEntryUniquePtr>;
    ManifestData m_ManifestData;
};

} // namespace WingsOfSteel::Private

#endif // TARGET_PLATFORM_WEB