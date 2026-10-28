#pragma once

#include <gdal_priv.h>

namespace star::terrain
{
/// Owns a GDAL dataset and closes it on destruction, including while an
/// exception unwinds. Stack-allocate it so every return/throw path releases
/// the handle exactly once.
struct DatasetHandle
{
    explicit DatasetHandle(GDALDataset *dataset = nullptr) : ptr(dataset)
    {
    }

    DatasetHandle(const DatasetHandle &) = delete;
    DatasetHandle &operator=(const DatasetHandle &) = delete;
    DatasetHandle(DatasetHandle &&other) noexcept : ptr(other.ptr)
    {
        other.ptr = nullptr;
    }
    DatasetHandle &operator=(DatasetHandle &&other) noexcept
    {
        if (this != &other)
        {
            close();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }
    ~DatasetHandle()
    {
        close();
        reset();
    }
    explicit operator bool() const
    {
        return ptr != nullptr;
    }

    GDALDataset *get() const
    {
        return ptr;
    }

    GDALDataset *operator->() const
    {
        return ptr;
    }

    /// Drops the handle without closing it, leaving ownership to the caller.
    void reset()
    {
        ptr = nullptr;
    }

    /// Opens `path`, closing any dataset this handle already owns, and takes
    /// ownership of the result. Returns the raw handle (nullptr on failure).
    GDALDataset *open(const char *path, GDALAccess access = GA_ReadOnly)
    {
        close();
        ptr = static_cast<GDALDataset *>(GDALOpen(path, access));
        return ptr;
    }

    GDALDataset *ptr{nullptr};

  private:
    void close()
    {
        if (ptr)
            GDALClose(ptr);
    }
};
} // namespace star::terrain
