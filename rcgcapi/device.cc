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

#include "device.h"
#include "stream.h"

#include "gentl_wrapper.h"
#include "exception.h"
#include "cport.h"

#include <iostream>

namespace rcg
{

Device::Device(const std::shared_ptr<Interface> &_parent,
               const std::shared_ptr<const GenTLWrapper> &_gentl, const char *_id)
{
  parent=_parent;
  gentl=_gentl;
  id=_id;

  n_open=0;
  dev=0;
  rp=0;
}

Device::~Device()
{
  if (n_open > 0)
  {
    gentl->DevClose(dev);
    parent->close();
  }
}

std::shared_ptr<Interface> Device::getParent() const
{
  return parent;
}

const std::string &Device::getID() const
{
  return id;
}

void Device::open(ACCESS access)
{
  if (n_open == 0)
  {
    parent->open();

    GenTL::DEVICE_ACCESS_FLAGS mode;

    switch (access)
    {
      case READONLY:
        mode=GenTL::DEVICE_ACCESS_READONLY;
        break;

      case CONTROL:
        mode=GenTL::DEVICE_ACCESS_CONTROL;
        break;

      case EXCLUSIVE:
        mode=GenTL::DEVICE_ACCESS_EXCLUSIVE;
        break;
    }

    if (gentl->IFOpenDevice(parent->getHandle(), id.c_str(), mode, &dev) != GenTL::GC_ERR_SUCCESS)
    {
      parent->close();
      throw GenTLException("Device::open()", gentl);
    }
  }

  n_open++;
}

void Device::close()
{
  if (n_open > 0)
  {
    n_open--;
  }

  if (n_open == 0)
  {
    gentl->DevClose(dev);
    dev=0;
    rp=0;

    nodemap=0;
    rnodemap=0;

    cport=0;
    rport=0;

    parent->close();
  }
}

namespace
{

int find(const std::vector<std::shared_ptr<Stream> > &list, const std::string &id)
{
  for (size_t i=0; i<list.size(); i++)
  {
    if (list[i]->getID() == id)
    {
      return i;
    }
  }

  return -1;
}

}

std::vector<std::shared_ptr<Stream> > Device::getStreams()
{
  std::vector<std::shared_ptr<Stream> > ret;

  if (dev != 0)
  {
    // get list of previously requested devices that are still in use

    std::vector<std::shared_ptr<Stream> > current;

    for (size_t i=0; i<slist.size(); i++)
    {
      std::shared_ptr<Stream> p=slist[i].lock();
      if (p)
      {
        current.push_back(p);
      }
    }

    // create list of interfaces, using either existing interfaces or
    // instantiating new ones

    uint32_t n=0;
    if (gentl->DevGetNumDataStreams(dev, &n) != GenTL::GC_ERR_SUCCESS)
    {
      throw GenTLException("Device::getStreams()", gentl);
    }

    for (uint32_t i=0; i<n; i++)
    {
      char tmp[256]="";
      size_t size=sizeof(tmp);

      if (gentl->DevGetDataStreamID(dev, i, tmp, &size) != GenTL::GC_ERR_SUCCESS)
      {
        throw GenTLException("Device::getStreams()", gentl);
      }

      int k=find(current, tmp);

      if (k >= 0)
      {
        ret.push_back(current[k]);
      }
      else
      {
        ret.push_back(std::shared_ptr<Stream>(new Stream(shared_from_this(), gentl, tmp)));
      }
    }

    // update internal list of devices for reusage on next call

    slist.clear();
    for (size_t i=0; i<ret.size(); i++)
    {
      slist.push_back(ret[i]);
    }
  }

  return ret;

}

namespace
{

std::string cDevGetInfo(const Device *obj, const std::shared_ptr<const GenTLWrapper> &gentl,
                        GenTL::DEVICE_INFO_CMD info)
{
  std::string ret;

  GenTL::INFO_DATATYPE type;
  char tmp[1024]="";
  size_t tmp_size=sizeof(tmp);
  GenTL::GC_ERROR err=GenTL::GC_ERR_ERROR;

  if (obj->getHandle() != 0)
  {
    err=gentl->DevGetInfo(obj->getHandle(), info, &type, tmp, &tmp_size);
  }
  else if (obj->getParent()->getHandle() != 0)
  {
    err=gentl->IFGetDeviceInfo(obj->getParent()->getHandle(), obj->getID().c_str(), info, &type,
                               tmp, &tmp_size);
  }

  if (err == GenTL::GC_ERR_SUCCESS && type == GenTL::INFO_DATATYPE_STRING)
  {
    ret=tmp;
  }

  return ret;
}

}

std::string Device::getVendor() const
{
  return cDevGetInfo(this, gentl, GenTL::DEVICE_INFO_VENDOR);
}

std::string Device::getModel() const
{
  return cDevGetInfo(this, gentl, GenTL::DEVICE_INFO_MODEL);
}

std::string Device::getTLType() const
{
  return cDevGetInfo(this, gentl, GenTL::DEVICE_INFO_TLTYPE);
}

std::string Device::getDisplayName() const
{
  return cDevGetInfo(this, gentl, GenTL::DEVICE_INFO_DISPLAYNAME);
}

std::string Device::getAccessStatus() const
{
  std::string ret;

  GenTL::INFO_DATATYPE type;
  int32_t status=-1;
  size_t size=sizeof(ret);
  GenTL::GC_ERROR err=GenTL::GC_ERR_ERROR;

  if (dev != 0)
  {
    err=gentl->DevGetInfo(dev, GenTL::DEVICE_INFO_ACCESS_STATUS, &type, &status, &size);
  }
  else if (parent->getHandle() != 0)
  {
    err=gentl->IFGetDeviceInfo(getParent()->getHandle(), id.c_str(),
                               GenTL::DEVICE_INFO_ACCESS_STATUS, &type, &status, &size);
  }

  if (err == GenTL::GC_ERR_SUCCESS)
  {
    if (type == GenTL::INFO_DATATYPE_INT32)
    {
      switch (status)
      {
        case GenTL::DEVICE_ACCESS_STATUS_READWRITE:
          ret="ReadWrite";
          break;

        case GenTL::DEVICE_ACCESS_STATUS_READONLY:
          ret="ReadOnly";
          break;

        case GenTL::DEVICE_ACCESS_STATUS_NOACCESS:
          ret="NoAccess";
          break;

        case GenTL::DEVICE_ACCESS_STATUS_BUSY:
          ret="Busy";
          break;

        case GenTL::DEVICE_ACCESS_STATUS_OPEN_READWRITE:
          ret="OpenReadWrite";
          break;

        case GenTL::DEVICE_ACCESS_STATUS_OPEN_READONLY:
          ret="OpenReadWrite";
          break;

        default:
          ret="Unknown";
          break;
      }
    }
  }

  return ret;
}

std::string Device::getUserDefinedName() const
{
  return cDevGetInfo(this, gentl, GenTL::DEVICE_INFO_USER_DEFINED_NAME);
}

std::string Device::getSerialNumber() const
{
  return cDevGetInfo(this, gentl, GenTL::DEVICE_INFO_SERIAL_NUMBER);
}

std::string Device::getVersion() const
{
  return cDevGetInfo(this, gentl, GenTL::DEVICE_INFO_VERSION);
}

uint64_t Device::getTimestampFrequency() const
{
  GenTL::INFO_DATATYPE type;
  uint64_t freq=0;
  size_t size=sizeof(freq);

  if (dev != 0)
  {
    gentl->DevGetInfo(dev, GenTL::DEVICE_INFO_TIMESTAMP_FREQUENCY, &type, &freq, &size);
  }
  else if (parent->getHandle() != 0)
  {
    gentl->IFGetDeviceInfo(getParent()->getHandle(), id.c_str(),
                           GenTL::DEVICE_INFO_TIMESTAMP_FREQUENCY, &type, &freq, &size);
  }

  return freq;
}

std::shared_ptr<GenApi::CNodeMapRef> Device::getNodeMap()
{
  if (dev != 0 && !nodemap)
  {
    cport=std::shared_ptr<CPort>(new CPort(gentl, &dev));
    nodemap=allocNodeMap(gentl, dev, cport.get());
  }

  return nodemap;
}

std::shared_ptr<GenApi::CNodeMapRef> Device::getRemoteNodeMap()
{
  if (dev != 0 && !rnodemap)
  {
    if (gentl->DevGetPort(dev, &rp) == GenTL::GC_ERR_SUCCESS)
    {
      rport=std::shared_ptr<CPort>(new CPort(gentl, &rp));
      rnodemap=allocNodeMap(gentl, rp, rport.get());
    }
  }

  return rnodemap;
}

void *Device::getHandle() const
{
  return dev;
}

std::vector<std::shared_ptr<Device> > getDevices()
{
  std::vector<std::shared_ptr<Device> > ret;

  std::vector<std::shared_ptr<rcg::System> > system=rcg::System::getSystems();

  for (size_t i=0; i<system.size(); i++)
  {
    system[i]->open();

    std::vector<std::shared_ptr<rcg::Interface> > interf=system[i]->getInterfaces();

    for (size_t k=0; k<interf.size(); k++)
    {
      interf[k]->open();

      std::vector<std::shared_ptr<rcg::Device> > device=interf[k]->getDevices();

      for (size_t j=0; j<device.size(); j++)
      {
        ret.push_back(device[j]);
      }

      interf[k]->close();
    }

    system[i]->close();
  }

  return ret;
}

std::shared_ptr<Device> getDevice(const char *devid)
{
  std::shared_ptr<Device> ret;

  std::vector<std::shared_ptr<rcg::System> > system=rcg::System::getSystems();

  for (size_t i=0; i<system.size() && !ret; i++)
  {
    system[i]->open();

    std::vector<std::shared_ptr<rcg::Interface> > interf=system[i]->getInterfaces();

    for (size_t k=0; k<interf.size() && !ret; k++)
    {
      interf[k]->open();

      ret=interf[k]->getDevice(devid);

      interf[k]->close();
    }

    system[i]->close();
  }

  return ret;
}

}