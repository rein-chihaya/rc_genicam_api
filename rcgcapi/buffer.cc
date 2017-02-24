/*
 * Roboception GmbH
 * Munich, Germany
 * www.roboception.com
 *
 * Copyright (c) 2017 Roboception GmbH
 * All rights reserved
 *
 * Author: Heiko Hirschmueller
 */

#include "buffer.h"
#include "stream.h"

#include "gentl_wrapper.h"
#include "exception.h"

namespace rcg
{

Buffer::Buffer(const std::shared_ptr<const GenTLWrapper> &_gentl, Stream *_parent)
{
  parent=_parent;
  gentl=_gentl;
  buffer=0;
}

void Buffer::setHandle(void *handle)
{
  buffer=handle;
}

namespace
{

template<class T> inline T getBufferValue(const std::shared_ptr<const GenTLWrapper> &gentl,
                                          void *stream, void *buffer, GenTL::BUFFER_INFO_CMD cmd)
{
  T ret=0;

  GenTL::INFO_DATATYPE type;
  size_t size=sizeof(T);

  if (stream != 0 && buffer != 0)
  {
    gentl->DSGetBufferInfo(stream, buffer, cmd, &type, &ret, &size);
  }

  return ret;
}

inline bool getBufferBool(const std::shared_ptr<const GenTLWrapper> &gentl,
                          void *stream, void *buffer, GenTL::BUFFER_INFO_CMD cmd)
{
  bool8_t ret=0;

  GenTL::INFO_DATATYPE type;
  size_t size=sizeof(ret);

  if (stream != 0 && buffer != 0)
  {
    gentl->DSGetBufferInfo(stream, buffer, cmd, &type, &ret, &size);
  }

  return ret != 0;
}

inline std::string getBufferString(const std::shared_ptr<const GenTLWrapper> &gentl,
                                   void *stream, void *buffer, GenTL::BUFFER_INFO_CMD cmd)
{
  std::string ret;

  GenTL::INFO_DATATYPE type;
  char tmp[1024]="";
  size_t size=sizeof(tmp);

  if (stream != 0 && buffer != 0)
  {
    if (gentl->DSGetBufferInfo(stream, buffer, cmd, &type, &ret, &size) == GenTL::GC_ERR_SUCCESS)
    {
      if (type == GenTL::INFO_DATATYPE_STRING)
      {
        ret=tmp;
      }
    }
  }

  return ret;
}

}

void *Buffer::getBase() const
{
  return getBufferValue<void *>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_BASE);
}

size_t Buffer::getSize() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_SIZE);
}

void *Buffer::getUserPtr() const
{
  return getBufferValue<void *>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_USER_PTR);
}

uint64_t Buffer::getTimestamp() const
{
  return getBufferValue<uint64_t>(gentl, parent->getHandle(), buffer,
                                  GenTL::BUFFER_INFO_TIMESTAMP);
}

bool Buffer::getNewData() const
{
  return getBufferBool(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_NEW_DATA);
}

bool Buffer::getIsQueued() const
{
  return getBufferBool(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_IS_QUEUED);
}

bool Buffer::getIsAcquiring() const
{
  return getBufferBool(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_IS_ACQUIRING);
}

bool Buffer::getIsIncomplete() const
{
  return getBufferBool(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_IS_INCOMPLETE);
}

std::string Buffer::getTLType() const
{
  return getBufferString(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_TLTYPE);
}

size_t Buffer::getSizeFilled() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer,
                                GenTL::BUFFER_INFO_SIZE_FILLED);
}

size_t Buffer::getWidth() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_WIDTH);
}

size_t Buffer::getHeight() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_HEIGHT);
}

size_t Buffer::getXOffset() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_XOFFSET);
}

size_t Buffer::getYOffset() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_YOFFSET);
}

size_t Buffer::getXPadding() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_XPADDING);
}

size_t Buffer::getYPadding() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_YPADDING);
}

uint64_t Buffer::getFrameID() const
{
  return getBufferValue<uint64_t>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_FRAMEID);
}

bool Buffer::getImagePresent() const
{
  return getBufferBool(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_IMAGEPRESENT);
}

size_t Buffer::getImageOffset() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer,
                                GenTL::BUFFER_INFO_IMAGEOFFSET);
}

size_t Buffer::getPayloadType() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer,
                                GenTL::BUFFER_INFO_PAYLOADTYPE);
}

uint64_t Buffer::getPixelFormat() const
{
  return getBufferValue<uint64_t>(gentl, parent->getHandle(), buffer,
                                  GenTL::BUFFER_INFO_PIXELFORMAT);
}

uint64_t Buffer::getPixelFormatNamespace() const
{
  return getBufferValue<uint64_t>(gentl, parent->getHandle(), buffer,
                                  GenTL::BUFFER_INFO_PIXELFORMAT_NAMESPACE);
}

size_t Buffer::getDeliveredImageHeight() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer,
                                GenTL::BUFFER_INFO_DELIVERED_IMAGEHEIGHT);
}

size_t Buffer::getDeliveredChunkPayloadSize() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer,
                                GenTL::BUFFER_INFO_DELIVERED_CHUNKPAYLOADSIZE);
}

uint64_t Buffer::getChunkLayoutID() const
{
  return getBufferValue<uint64_t>(gentl, parent->getHandle(), buffer,
                                  GenTL::BUFFER_INFO_CHUNKLAYOUTID);
}

std::string Buffer::getFilename() const
{
  return getBufferString(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_FILENAME);
}

bool Buffer::getIsLittleEndian() const
{
  bool ret=false;

  GenTL::INFO_DATATYPE type;
  int32_t v;
  size_t size=sizeof(v);
  GenTL::GC_ERROR err=GenTL::GC_ERR_SUCCESS;

  if (parent->getHandle() != 0 && buffer != 0)
  {
    err=gentl->DSGetBufferInfo(parent->getHandle(), buffer, GenTL::BUFFER_INFO_PIXEL_ENDIANNESS,
                               &type, &v, &size);

    if (err == GenTL::GC_ERR_SUCCESS && type == GenTL::INFO_DATATYPE_INT32 &&
        v == GenTL::PIXELENDIANNESS_LITTLE)
    {
      ret=true;
    }
  }

  return ret;
}

size_t Buffer::getDataSize() const
{
  return getBufferValue<size_t>(gentl, parent->getHandle(), buffer, GenTL::BUFFER_INFO_DATA_SIZE);
}

uint64_t Buffer::getTimestampNS() const
{
  return getBufferValue<uint64_t>(gentl, parent->getHandle(), buffer,
                                  GenTL::BUFFER_INFO_TIMESTAMP_NS);
}

bool Buffer::getDataLargerThanBuffer() const
{
  return getBufferBool(gentl, parent->getHandle(), buffer,
                       GenTL::BUFFER_INFO_DATA_LARGER_THAN_BUFFER);
}

bool Buffer::getContainsChunkdata() const
{
  return getBufferBool(gentl, parent->getHandle(), buffer,
                       GenTL::BUFFER_INFO_CONTAINS_CHUNKDATA);
}

void *Buffer::getHandle() const
{
  return buffer;
}

}