#if defined(TARGET_PLATFORM_WEB)

#include "vfs/private/web/vfs_web.hpp"

#include <cassert>
#include <sstream>
#include <vector>

#include <emscripten/fetch.h>
#include <xxhash.h>

#include "core/log.hpp"
#include "vfs/private/web/manifest.hpp"
#include "vfs/file.hpp"

namespace WingsOfSteel::Pandora::Private
{

void downloadSucceeded(emscripten_fetch_t *fetch) 
{
    Log::Info() << "Finished downloading " << fetch->numBytes << " bytes from URL " << fetch->url;
    // The data is now available at fetch->data[0] through fetch->data[fetch->numBytes-1];
    emscripten_fetch_close(fetch); // Free data associated with the fetch.
}

void downloadFailed(emscripten_fetch_t *fetch) 
{
    Log::Info() << "Downloading " << fetch->url << " failed, HTTP failure status code: " << fetch->status;
    emscripten_fetch_close(fetch); // Also free data on failure.
}

VFSWeb::VFSWeb()
{
    Log::Info() << "Creating VFS web backend...";
}

VFSWeb::~VFSWeb()
{

}

void VFSWeb::Initialize()
{
    Log::Info() << "Initializing VFS web backend.";
    m_pManifest = std::make_unique<Manifest>();
    m_pManifest->Initialize();
}

void VFSWeb::Update()
{
    if (m_InProgress.has_value())
    {

    }

    if (!m_Queue.empty() && !m_InProgress.has_value())
    {
        QueuedFile& queuedFile = m_Queue.front();
        m_InProgress = queuedFile;
        m_Queue.pop_front();

        std::stringstream url;
        url << VFS_WEB_HOST << "/data/core" << m_InProgress->path;
        Log::Info() << "Downloading '" << url.str() << "'...";

        emscripten_fetch_attr_t attr;
        emscripten_fetch_attr_init(&attr);
        strcpy(attr.requestMethod, "GET");
        attr.attributes = EMSCRIPTEN_FETCH_LOAD_TO_MEMORY | EMSCRIPTEN_FETCH_PERSIST_FILE;
        attr.userData = this;
        attr.onsuccess = [](emscripten_fetch_t* pFetch)
        {
            VFSWeb* pVFS = reinterpret_cast<VFSWeb*>(pFetch->userData);
            std::stringstream hash;
            hash << XXH3_64bits(pFetch->data, pFetch->numBytes * sizeof(char));
            Log::Info() << "Hash: " << hash.str() << "| size: " << pFetch->numBytes;

            assert(pVFS->m_InProgress.has_value());

            FileData fileData;
            fileData.reserve(pFetch->numBytes);
            std::memcpy(fileData.data(), pFetch->data, pFetch->numBytes * sizeof(char));

            Log::Info() << "Downloaded '" << pVFS->m_InProgress->path << "'.";
            pVFS->m_InProgress->onFileReadCompleted(FileReadResult::Ok, std::make_shared<File>(std::move(fileData)));
            pVFS->m_InProgress.reset();
            emscripten_fetch_close(pFetch);
        };
        attr.onerror = [](emscripten_fetch_t* pFetch)
        {
            VFSWeb* pVFS = reinterpret_cast<VFSWeb*>(pFetch->userData);
            assert(pVFS->m_InProgress.has_value());
            Log::Error() << "Failed to download " << pVFS->m_InProgress->path;
            exit(-1);
            emscripten_fetch_close(pFetch);
        };
        emscripten_fetch(&attr, url.str().c_str());
    }
}

bool VFSWeb::FileExists(const std::string& path) const
{
    return false;
}
    
void VFSWeb::FileRead(const std::string& path, FileReadCallback onFileReadCompleted)
{
    QueuedFile queuedFile;
    queuedFile.path = path;
    queuedFile.onFileReadCompleted = onFileReadCompleted;
    m_Queue.push_back(queuedFile);
}

} // namespace WingsOfSteel::Pandora::Private

#endif // TARGET_PLATFORM_WEB