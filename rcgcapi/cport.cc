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

#include "cport.h"

#include "exception.h"

#include "locale.h"

namespace rcg
{

CPort::CPort(std::shared_ptr<const GenTLWrapper> _gentl, void **_port) : gentl(_gentl)
{
  port=_port;
}

void CPort::Read(void *buffer, int64_t addr, int64_t length)
{
  size_t size=static_cast<size_t>(length);

  if (*port != 0)
  {
    if (gentl->GCReadPort(*port, addr, buffer, &size) != GenTL::GC_ERR_SUCCESS)
    {
      throw GenTLException("CPort::Read()", gentl);
    }

    if (size != static_cast<size_t>(length))
    {
      throw GenTLException("CPort::Read(): Returned size not as expected");
    }
  }
  else
  {
    throw GenTLException("CPort::Read(): Port has been closed");
  }
}

void CPort::Write(const void *buffer, int64_t addr, int64_t length)
{
  size_t size=static_cast<size_t>(length);

  if (*port != 0)
  {
    if (gentl->GCWritePort(*port, addr, buffer, &size) != GenTL::GC_ERR_SUCCESS)
    {
      throw GenTLException("CPort::Write()", gentl);
    }

    if (size != static_cast<size_t>(length))
    {
      throw GenTLException("CPort::Write(): Returned size not as expected");
    }
  }
  else
  {
    throw GenTLException("CPort::Write(): Port has been closed");
  }
}

GenApi::EAccessMode CPort::GetAccessMode() const
{
  if (*port != 0)
  {
    return GenApi::RW;
  }

  return GenApi::NA;
}

namespace
{

inline std::string toLower(const std::string &s, size_t start, size_t size)
{
  std::ostringstream out;

  size_t end=std::min(s.size(), start+size);

  while (start < end)
  {
    out << static_cast<char>(std::tolower(s[start++]));
  }

  return out.str();
}

}

std::shared_ptr<GenApi::CNodeMapRef> allocNodeMap(std::shared_ptr<const GenTLWrapper> gentl,
                                                  void *port, CPort *cport)
{
  std::shared_ptr<GenApi::CNodeMapRef> nodemap(new GenApi::CNodeMapRef());

  // get number of URLS that the given port provides

  uint32_t n=0;
  if (gentl->GCGetNumPortURLs(port, &n) != GenTL::GC_ERR_SUCCESS)
  {
    throw GenTLException("allocNodeMap()", gentl);
  }

  if (n == 0)
  {
    return std::shared_ptr<GenApi::CNodeMapRef>();
  }

  // get the first URL

  GenTL::INFO_DATATYPE type;
  char tmp[1024]="";
  size_t size=sizeof(tmp);

  if (gentl->GCGetPortURLInfo(port, 0, GenTL::URL_INFO_URL, &type, tmp, &size) !=
      GenTL::GC_ERR_SUCCESS)
  {
    throw GenTLException("allocNodeMap()", gentl);
  }

  // interpret the URL and load XML File

  std::string url=tmp;
  if (toLower(url, 0, 6) == "local:")
  {
    // interpret local URL

    int i=6;
    if (url.compare(i, 3, "///") == 0)
    {
      i+=3;
    }

    std::stringstream in(url.substr(i));
    std::string name, saddress, slength;

    std::getline(in, name, ';');
    std::getline(in, saddress, ';');
    std::getline(in, slength, ';');

    uint64_t address=std::stoull(saddress, 0, 16);
    uint64_t length=std::stoull(slength, 0, 16);

    // read XML or ZIP from registers

    std::unique_ptr<char[]> buffer(new char[length+1]);

    if (gentl->GCReadPort(port, address, buffer.get(), &length) != GenTL::GC_ERR_SUCCESS)
    {
      throw GenTLException("allocNodeMap()", gentl);
    }

    buffer.get()[length]='\0';

    // load XML or ZIP from registers

    if (name.size() > 4 && toLower(name, name.size()-4, 4) == ".zip")
    {
      nodemap->_LoadXMLFromZIPData(buffer.get(), length);
    }
    else
    {
      gcstring xml=buffer.get();
      nodemap->_LoadXMLFromString(xml);
    }
  }
  else if (toLower(url, 0, 5) == "file:")
  {
    // interpret local URL

    int i=6;
    if (url.compare(i, 3, "///") == 0)
    {
      i+=3;
    }

    std::string name=url.substr(i);

    // load XML or ZIP from file

    if (name.size() > 4 && toLower(name, name.size()-4, 4) == ".zip")
    {
      gcstring file=name.c_str();
      nodemap->_LoadXMLFromZIPFile(file);
    }
    else
    {
      gcstring file=name.c_str();
      nodemap->_LoadXMLFromFile(file);
    }
  }
  else
  {
    throw GenTLException(("allocNodeMap(): Cannot interpret URL: "+url).c_str());
  }

  // get port name

  size=sizeof(tmp);

  if (gentl->GCGetPortInfo(port, GenTL::PORT_INFO_PORTNAME, &type, tmp, &size) !=
      GenTL::GC_ERR_SUCCESS)
  {
    throw GenTLException("allocNodeMap()", gentl);
  }

  gcstring portname=tmp;
  if (!nodemap->_Connect(cport, portname))
  {
    throw GenTLException((std::string("allocNodeMap(): Cannot connect port: ")+tmp).c_str());
  }

  return nodemap;
}

}