/*-------------------------------------------------------------------------
This source file is a part of m8rscript

For the latest info, see http://www.marrin.org/

Copyright (c) 2016, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice,
      this list of conditions and the following disclaimer.
      
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
      
    - Neither the name of the <ORGANIZATION> nor the names of its
      contributors may be used to endorse or promote products derived from
      this software without specific prior written permission.
      
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include "LittleFS.h"

#include "Application.h"

using namespace m8r;

lfs_t LittleFS::_littleFileSystem;


LittleFS::LittleFS(const char* name)
{
    memset(&_littleFileSystem, 0, sizeof(_littleFileSystem));
    setConfig(_config, name);
}

LittleFS::~LittleFS()
{
    lfs_unmount(&_littleFileSystem);
}

bool LittleFS::mount()
{
    system()->printf(ROMSTR("Mounting LittleFS...\n"));
    int32_t result = internalMount();
    if (result != 0) {
        if (result != 0) {
            system()->printf(ROMSTR("ERROR: Not a valid LittleFS filesystem. Please format.\n"));
            _error = Error::Code::FSNotFormatted;
        } else {
            system()->printf(ROMSTR("ERROR: LittleFS mount failed, error=%d\n"), result);
            _error = Error::Code::MountFailed;
        }
        return false;
    }
    if (!mounted()) {
        system()->printf(ROMSTR("ERROR: LittleFS filesystem failed to mount\n"));
        _error = Error::Code::MountFailed;
        return false;
    }

    system()->printf(ROMSTR("LittleFS mounted successfully\n"));
    _error = Error::Code::OK;

    return true;
}

bool LittleFS::mounted() const
{
    return true; //SPIFFS_mounted(const_cast<spiffs_t*>(&_littleFileSystem));
}

void LittleFS::unmount()
{
    if (mounted()) {
        lfs_unmount(&_littleFileSystem);
    }
}

bool LittleFS::format()
{
    if (!mounted()) {
        internalMount();
    }
    unmount();
    
    int32_t result = lfs_format(&_littleFileSystem, &_config);
    if (result != 0) {
        system()->printf(ROMSTR("ERROR: LittleFS format failed, error=%d\n"), result);
        return false;
    }
    mount();
    return true;
}

struct FileModeEntry {
    FS::FileOpenMode _mode;
    int _flags;
};

static const FileModeEntry _fileModeMap[] = {
    { FS::FileOpenMode::Read,  LFS_O_RDONLY },
    { FS::FileOpenMode::ReadUpdate, LFS_O_RDWR },
    { FS::FileOpenMode::Write,  LFS_O_WRONLY | LFS_O_CREAT | LFS_O_TRUNC },
    { FS::FileOpenMode::WriteUpdate, LFS_O_RDWR | LFS_O_CREAT | LFS_O_TRUNC },
    { FS::FileOpenMode::Append,  LFS_O_WRONLY | LFS_O_CREAT | LFS_O_APPEND },
    { FS::FileOpenMode::AppendUpdate, LFS_O_RDWR | LFS_O_CREAT },
};

std::shared_ptr<File> LittleFS::open(const char* name, FileOpenMode mode)
{
    return std::shared_ptr<File>(new LittleFile(name, mode));
}

std::shared_ptr<Directory> LittleFS::openDirectory(const char* name)
{
    if (!mounted()) {
        return nullptr;
    }
    return std::shared_ptr<LittleDirectory>(new LittleDirectory(name));
}

bool LittleFS::makeDirectory(const char* name)
{
    int result = lfs_mkdir(&_littleFileSystem, name);
    (void) result;
    //_error = error;
    return _error == Error::Code::OK;
}

bool LittleFS::remove(const char* name)
{
    return lfs_remove(&_littleFileSystem, name) == 0;
}

bool LittleFS::rename(const char* src, const char* dst)
{
    return lfs_rename(&_littleFileSystem, src, dst) == 0;
}

uint32_t LittleFS::totalSize() const
{
    return 1000;
}

uint32_t LittleFS::totalUsed() const
{
    return 100;
}

int32_t LittleFS::internalMount()
{
    return lfs_mount(&_littleFileSystem, &_config);
}

LittleDirectory::LittleDirectory(const char* name)
{
    lfs_dir_open(&LittleFS::_littleFileSystem, &_dir, name);
    next();
}

bool LittleDirectory::next()
{
    lfs_info info;
    lfs_dir_read(&LittleFS::_littleFileSystem, &_dir, &info);
    _size = info.size;
    _name = String(info.name);
    return true;
}

LittleFile::LittleFile(const char* name, FS::FileOpenMode mode)
{
    // Convert mode to lfs_flags
    int flags = 0;
    for (int i = 0; i < sizeof(_fileModeMap) / sizeof(FileModeEntry); ++i) {
        if (_fileModeMap[i]._mode == mode) {
            flags = _fileModeMap[i]._flags;
            break;
        }
    }

    lfs_file_open(&LittleFS::_littleFileSystem, &_file, name, flags);
}

LittleFile::~LittleFile()
{
    close();
}
  
int32_t LittleFile::read(char* buf, uint32_t size)
{
    if (_mode == FS::FileOpenMode::Write || _mode == FS::FileOpenMode::Append) {
        _error = Error::Code::NotReadable;
        return -1;
    }
    return lfs_file_read(&LittleFS::_littleFileSystem, &_file, buf, size);
}

int32_t LittleFile::write(const char* buf, uint32_t size)
{
    if (_mode == FS::FileOpenMode::Read) {
        _error = Error::Code::NotWritable;
        return -1;
    }
    
    if (_mode == FS::FileOpenMode::AppendUpdate) {
        seek(0, SeekWhence::End);
    }

    return lfs_file_write(&LittleFS::_littleFileSystem, &_file, const_cast<char*>(buf), size);
}

void LittleFile::close()
{
    lfs_file_close(&LittleFS::_littleFileSystem, &_file);
    _error = Error::Code::FileClosed;
}

bool LittleFile::seek(int32_t offset, SeekWhence whence)
{
    if (_mode == FS::FileOpenMode::Append) {
        _error = Error::Code::SeekNotAllowed;
        return -1;
    }
    return lfs_file_seek(&LittleFS::_littleFileSystem, &_file, offset, 0) == 0;
}

int32_t LittleFile::tell() const
{
    return lfs_file_tell(&LittleFS::_littleFileSystem, const_cast<lfs_file_t*>(&_file));
}

bool LittleFile::eof() const
{
    return true; //SPIFFS_eof(SpiffsFS::sharedSpiffs(), _file) > 0;
}


