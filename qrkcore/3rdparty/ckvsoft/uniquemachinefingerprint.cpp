/*
 * This file is part of QRK - Qt Registrier Kasse
 *
 * Copyright (C) 2015-2019 Christian Kvasny <chris@ckvsoft.at>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 * Button Design, and Idea for the Layout are lean out from LillePOS, Copyright 2010, Martin Koller, kollix@aon.at
 *
*/

#include "uniquemachinefingerprint.h"
#include <QHostInfo>
#include <QStorageInfo>
#include <QCryptographicHash>

#if defined(_WIN32)
#include <windows.h>
#include <intrin.h>
#else
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/utsname.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <assert.h>

#if defined(__APPLE__)
#include <IOKit/IOKitLib.h>
#include <net/if_dl.h>
#include <ifaddrs.h>
#include <net/if_types.h>
#include <mach-o/arch.h>
//#include <sys/types.h>
#include <sys/socket.h>
#else //!__APPLE__
#include <linux/if.h>
#include <libudev.h>
// #include <linux/sockios.h>
#endif //!__APPLE__
#endif

UniqueMachineFingerprint::UniqueMachineFingerprint(QObject *parent) : QObject(parent)
{

}

#if defined(_WIN32)
// we just need this for purposes of unique machine id.
// So any one or two mac's is fine.
quint16 UniqueMachineFingerprint::hashMacAddress( PIP_ADAPTER_INFO info )
{
    quint16 hash = 0;
    for ( quint32 i = 0; i < info->AddressLength; i++ ) {
        hash += ( info->Address[i] << (( i & 1 ) * 8 ));
    }
    return hash;
}

void UniqueMachineFingerprint::getMacHash( quint16& mac1, quint16& mac2 )
{
    IP_ADAPTER_INFO AdapterInfo[32];
    DWORD dwBufLen = sizeof( AdapterInfo );

    DWORD dwStatus = GetAdaptersInfo( AdapterInfo, &dwBufLen );
    if ( dwStatus != ERROR_SUCCESS )
        return; // no adapters.

    PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
    mac1 = hashMacAddress( pAdapterInfo );
    if ( pAdapterInfo->Next )
        mac2 = hashMacAddress( pAdapterInfo->Next );

    // sort the mac addresses. We don't want to invalidate
    // both macs if they just change order.
    if ( mac1 > mac2 ) {
        quint16 tmp = mac2;
        mac2 = mac1;
        mac1 = tmp;
    }
}

quint16 UniqueMachineFingerprint::getVolumeHash()
{
    DWORD serialNum = 0;

    // Determine if this volume uses an NTFS file system.
    GetVolumeInformation( (LPCWSTR) "c:\\", Q_NULLPTR, 0, &serialNum, Q_NULLPTR, Q_NULLPTR, Q_NULLPTR, 0 );
    quint16 hash = (quint16)(( serialNum + ( serialNum >> 16 )) & 0xFFFF );

    return hash;
}

quint16 UniqueMachineFingerprint::getCpuHash()
{
    int cpuinfo[4] = { 0, 0, 0, 0 };
    __cpuid( cpuinfo, 0 );
    quint16 hash = 0;
    quint16* ptr = (quint16*)(&cpuinfo[0]);
    for ( quint32 i = 0; i < 8; i++ )
        hash += ptr[i];

    return hash;
}
#else

quint16 UniqueMachineFingerprint::hashMacAddress( quint8* mac )
{
    quint16 hash = 0;

    for ( quint32 i = 0; i < 6; i++ ) {
        hash += ( mac[i] << (( i & 1 ) * 8 ));
    }
    return hash;
}

void UniqueMachineFingerprint::getMacHash( quint16& mac1, quint16& mac2 )
{
    mac1 = 0;
    mac2 = 0;

#if defined(__APPLE__)

    struct ifaddrs* ifaphead;
    if ( getifaddrs( &ifaphead ) != 0 )
        return;

    // iterate over the net interfaces
    bool foundMac1 = false;
    struct ifaddrs* ifap;
    for ( ifap = ifaphead; ifap; ifap = ifap->ifa_next ) {
        struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifap->ifa_addr;
        if ( sdl && ( sdl->sdl_family == AF_LINK ) && ( sdl->sdl_type == IFT_ETHER )) {
            if ( !foundMac1 ) {
                foundMac1 = true;
                mac1 = hashMacAddress( (quint8*)(LLADDR(sdl))); //sdl->sdl_data) + sdl->sdl_nlen) );
            } else {
                mac2 = hashMacAddress( (quint8*)(LLADDR(sdl))); //sdl->sdl_data) + sdl->sdl_nlen) );
                break;
            }
        }
    }

    freeifaddrs( ifaphead );

#else // !__APPLE__

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP );
    if ( sock < 0 ) return;

    // enumerate all IP addresses of the system
    struct ifconf conf;
    char ifconfbuf[ 128 * sizeof(struct ifreq)  ];
    memset( ifconfbuf, 0, sizeof( ifconfbuf ));
    conf.ifc_buf = ifconfbuf;
    conf.ifc_len = sizeof( ifconfbuf );
    if ( ioctl( sock, SIOCGIFCONF, &conf )) {
        assert(0);
        return;
    }

    // get MAC address
    bool foundMac1 = false;
    struct ifreq* ifr;
    for ( ifr = conf.ifc_req; reinterpret_cast<qint8*>(ifr) < reinterpret_cast<qint8*>(conf.ifc_req) + conf.ifc_len; ifr++ ) {
        if ( ifr->ifr_addr.sa_data == (ifr+1)->ifr_addr.sa_data )
            continue;  // duplicate, skip it

        if ( ioctl( sock, SIOCGIFFLAGS, ifr ))
            continue;  // failed to get flags, skip it
        if ( ioctl( sock, SIOCGIFHWADDR, ifr ) == 0 ) {
            if ( !foundMac1 ) {
                foundMac1 = true;
                mac1 = hashMacAddress( reinterpret_cast<quint8*>(&(ifr->ifr_addr.sa_data)));
            } else {
                mac2 = hashMacAddress( reinterpret_cast<quint8*>(&(ifr->ifr_addr.sa_data)));
                break;
            }
        }
    }

    close( sock );

#endif // !__APPLE__

    // sort the mac addresses. We don't want to invalidate
    // both macs if they just change order.
    if ( mac1 > mac2 ) {
        quint16 tmp = mac2;
        mac2 = mac1;
        mac1 = tmp;
    }
}

quint16 UniqueMachineFingerprint::getVolumeHash()
{
#if defined(__APPLE__)
    // osx don't have a 'volume serial number' like on windows.
    // Lets hash the system name instead.
    return getSystemSerialNumberHash();
#else // !__APPLE__

    union charunion {
       quint8* chr;
       const QChar* cchr;
    } chrptrs;

    chrptrs.cchr = getVolumeSerial().data(); // const char *
    quint8* sysname = chrptrs.chr;

    quint16 hash = 0;

    for ( quint32 i = 0; sysname[i]; i++ )
        hash += ( sysname[i] << (( i & 1 ) * 8 ));
    return hash;
#endif
}

#if defined(__APPLE__)
quint16 UniqueMachineFingerprint::getCpuHash()
{
    const NXArchInfo* info = NXGetLocalArchInfo();
    quint16 val = 0;
    val += (quint16)info->cputype;
    val += (quint16)info->cpusubtype;
    return val;
}

quint16 UniqueMachineFingerprint::getSystemSerialNumberHash()
{
    char buf[512] = "";
    int bufSize = sizeof(buf);
    io_registry_entry_t ioRegistryRoot = IORegistryEntryFromPath(kIOMasterPortDefault, "IOService:/");
    CFStringRef uuidCf = (CFStringRef) IORegistryEntryCreateCFProperty(ioRegistryRoot, CFSTR(kIOPlatformUUIDKey), kCFAllocatorDefault, 0);
    IOObjectRelease(ioRegistryRoot);
    CFStringGetCString(uuidCf, buf, bufSize, kCFStringEncodingMacRoman);
    CFRelease(uuidCf);

    QByteArray ba(buf);
    QString sha1 = QCryptographicHash::hash(ba,QCryptographicHash::Sha1).toHex();
    quint16 hash = 0;
    QChar character;
    for (int i = 0; i < sha1.size(); i++) {
        character = sha1.at(i);
        hash = (15 * hash) + (character.toLatin1());
    }

    return hash;
}

#else // !__APPLE__

void UniqueMachineFingerprint::getCpuid( quint32 *p, unsigned int ax)
{
#if defined(__arm__) || defined(__aarch64__)
        p[0] = 0xFD;
        p[1] = 0xC1;
        p[2] = 0x72;
        p[3] = 0x1D;
        return;
#else
    asm volatile
          ("cpuid" : "=a" (p[0]), "=b" (p[1]), "=c" (p[2]), "=d" (p[3])
           : "a" (ax), "c" (0));
    /* ecx is often an input as well as an output. */
#endif
}

quint16 UniqueMachineFingerprint::getCpuHash()
{
    quint32 cpuinfo[4] = { 0, 0, 0, 0 };
    getCpuid(cpuinfo, 0);
    quint16 hash = 0;
    quint32* ptr = (&cpuinfo[0]);
    for ( quint32 i = 0; i < 4; i++ )
        hash += (ptr[i] & 0xFFFF) + ( ptr[i] >> 16 );

    return hash;
}

int eq(const char *const a, const char *const b)
{
    if (a == Q_NULLPTR || b == Q_NULLPTR)
        return 0;
    else
        return !strcmp(a, b);
}

const QString UniqueMachineFingerprint::getVolumeSerial()
{
    QStorageInfo info("/");
    QString root = info.device();
    QString serial;
    struct udev *udev;
    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        return QString();
    }
    enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "block");
    udev_enumerate_scan_devices(enumerate);
    devices = udev_enumerate_get_list_entry(enumerate);
    udev_list_entry_foreach(dev_list_entry, devices) {
        const char *path;
        struct udev_device *dev;

        /* Get the filename of the /sys entry for the device
         * and create a udev_device object (dev) representing it */
        path = udev_list_entry_get_name(dev_list_entry);
        dev = udev_device_new_from_syspath(udev, path);
        if (eq("disk", udev_device_get_devtype(dev))) {
            if (root.contains(udev_device_get_devnode(dev)))
                serial = udev_device_get_property_value(dev, "ID_SERIAL");
//            udev_device_unref(dev);
        }
        udev_device_unref(dev);
    }
    /* Free the enumerator object */
    udev_enumerate_unref(enumerate);
    udev_unref(udev);
    return serial;
}

#endif // !__APPLE__
#endif

const QString UniqueMachineFingerprint::getMachineName()
{
    QString nodename = QHostInfo::localHostName();

    QString sha1;
    if ( nodename.isEmpty() ) {
        sha1 = QCryptographicHash::hash(QString("unknown").toUtf8(), QCryptographicHash::Sha1).toHex();
    } else {
        sha1 = QCryptographicHash::hash(nodename.toUtf8(), QCryptographicHash::Sha1).toHex();
    }


    quint32 hash = 0;
    union charunion {
       char chr;
       uchar uchr;
    } chrptrs;

    for (int i = 0; i < sha1.size(); i++) {
        chrptrs.chr = sha1.at(i).toLatin1();
        hash = (31 * hash) + (chrptrs.uchr);
    }
    return QString::number(hash, 16);
}

static quint16 mask[5] = { 0x4e25, 0xf4a1, 0x5437, 0xab41, 0x0000 };

void UniqueMachineFingerprint::smear( quint16* id )
{
    for ( quint32 i = 0; i < 5; i++ )
        for ( quint32 j = i; j < 5; j++ )
            if ( i != j )
                id[i] ^= id[j];

    for ( quint32 i = 0; i < 5; i++ )
        id[i] ^= mask[i];
}

void UniqueMachineFingerprint::unsmear( quint16* id )
{
    for ( quint32 i = 0; i < 5; i++ )
        id[i] ^= mask[i];

    for ( quint32 i = 0; i < 5; i++ )
        for ( quint32 j = 0; j < i; j++ )
            if ( i != j )
                id[4-i] ^= id[4-j];
}

quint16 *UniqueMachineFingerprint::computeSystemUniqueId()
{
    static quint16 id[5];
    // produce a number that uniquely identifies this system.
    id[0] = getCpuHash();
    id[1] = getVolumeHash();
    getMacHash( id[2], id[3] );

    // fifth block is some checkdigits
    id[4] = 0;
    for ( quint32 i = 0; i < 4; i++ )
        id[4] += id[i];

    smear( id );

    return id;
}

const QString UniqueMachineFingerprint::getSystemUniqueId()
{
    QByteArray buf;
    buf.append(getMachineName());

    quint16* id = computeSystemUniqueId();
    for ( quint32 i = 0; i < 5; i++ )
    {
        char num[16];
        snprintf( num, 16, "%x", id[i] );
        buf.append("-");
        switch( strlen( num ))
        {
        case 1: buf.append("000"); break;
        case 2: buf.append("00");  break;
        case 3: buf.append("0");   break;
        }
        buf.append(num);
    }

    return buf.toUpper();
}

bool UniqueMachineFingerprint::validate( QString testIdString )
{
    // unpack the given string. parse failures return false.
    testIdString = testIdString.replace("-", "");

    if (testIdString.size() != 28)
        return false;

    int step = 4;
    for (int i = 8; i < testIdString.size(); i+=step+1)
            testIdString.insert(i, "-");

    QStringList testStringList = testIdString.split("-");    
    QString testname = testStringList.takeFirst();
    if ( testname.isEmpty() ) return false;

    quint16 testId[5];
    for ( quint32 i = 0; i < 5; i++ ) {
        QString testNum = testStringList.takeFirst();
        if ( testNum.isEmpty() ) return false;
        testId[i] = testNum.toUShort(Q_NULLPTR, 16 );
    }
    unsmear( testId );

    quint16 check = 0;
    for ( quint32 i = 0; i < 4; i++ )
        check += testId[i];
    if ( check != testId[4] ) return false;

    quint16 systemId[5];
    memcpy( systemId, computeSystemUniqueId(), sizeof( systemId ));
    unsmear( systemId );

    quint32 score = 0;

    for ( quint32 i = 0; i < 4; i++ )
        if ( testId[i] == systemId[i] )
            score++;

    if ( getMachineName().toUpper().compare(testname) == 0)
        score++;

    return ( score >= 3 ) ? true : false;
}
