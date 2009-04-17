/*
 * Copyright (c) 2008 James Molloy, Jörg Pfähler, Matthew Iselin
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#ifndef FAT_DIRECTORY_H
#define FAT_DIRECTORY_H

#include "FatFilesystem.h"
#include <vfs/Directory.h>
#include <utilities/Vector.h>

/** A File is a file, a directory or a symlink. */
class FatDirectory : public Directory
{
private:
  /** Copy constructors are hidden - unused! */
  FatDirectory(const FatDirectory &file);
  FatDirectory& operator =(const FatDirectory&);
public:
  /** Constructor, should be called only by a Filesystem. */
  FatDirectory(String name, uintptr_t cluster,
           class FatFilesystem *pFs, File *pParent);
  /** Destructor */
  virtual ~FatDirectory();

  uint64_t read(uint64_t location, uint64_t size, uintptr_t buffer)
  {return 0;}
  uint64_t write(uint64_t location, uint64_t size, uintptr_t buffer)
  {return 0;}

  void truncate()
  {}

  /** Reads directory contents into File* cache. */
  virtual void cacheDirectoryContents();

  /** Adds a directory entry. */
  virtual bool addEntry(String filename, File *pFile, size_t type);
  /** Removes a directory entry. */
  virtual bool removeEntry(File *pFile);

  /** Updates inode attributes. */
  void fileAttributeChanged();

private:
  
  FatFilesystem::FatType m_Type;
  uintptr_t m_BlockSize;
  bool m_bRootDir;
  
  /** Number of bytes to iterate over for this directory */
  uint32_t m_DirBlockSize;
};

#endif
