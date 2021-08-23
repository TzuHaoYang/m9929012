#/***********************************************************************
# *
# *  Copyright (c) 2018  Broadcom
# *  All Rights Reserved
# *
#<:label-BRCM:2018:proprietary:standard
#
# This program is the proprietary software of Broadcom and/or its
# licensors, and may only be used, duplicated, modified or distributed pursuant
# to the terms and conditions of a separate, written license agreement executed
# between you and Broadcom (an "Authorized License").  Except as set forth in
# an Authorized License, Broadcom grants no license (express or implied), right
# to use, or waiver of any kind with respect to the Software, and Broadcom
# expressly reserves all rights in and to the Software and all intellectual
# property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
# NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
# BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
#
# Except as expressly set forth in the Authorized License,
#
# 1. This program, including its structure, sequence and organization,
#    constitutes the valuable trade secrets of Broadcom, and you shall use
#    all reasonable efforts to protect the confidentiality thereof, and to
#    use this information only in connection with your use of Broadcom
#    integrated circuit products.
#
# 2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#    AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#    WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#    RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#    ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#    FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#    COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#    TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#    PERFORMANCE OF THE SOFTWARE.
#
# 3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#    ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#    INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#    WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#    IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#    OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#    SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#    SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#    LIMITED REMEDY.
#:>
# *
# ************************************************************************/
#
# This is a perl class.
# It stores data a NvmParamMapping entry.
# It provides below functions:
# 1. Construct a new node of NvmMappingNode.
# 2. Format print proper C format code to be compiled in libwlmdm.so.
#

package NvmMappingNode;
use strict;
use parent 'Exporter';
our @EXPORT = qw(format_print);

#
# constructor
#
sub new {
    my $class = shift;
    my $self = {
        nv_name => shift,
        nv_type => shift,
        pv_oid => shift,
        pv_name => shift,
        pv_type => shift,
        mapper => shift
    };

    $self->{pv_type} = fixupType($self->{pv_type});

    bless $self, $class;
    return $self;
}

#
# format print out the NvmParamMapping entry.
sub format_print {
    my ($this, $fh) = @_;
    print $fh sprintf("\t{\n\t  {%-20s %s},\n\t  {%-20s %-20s %s},\n\t  %s\n\t},\n",
                      "\"".${$this}{nv_name}."\"".',', ${$this}{nv_type}, ${$this}{pv_oid}.',', "\"".${$this}{pv_name}."\"".',',
                      ${$this}{pv_type}, ${$this}{mapper});
}

#
# Take the type string from the MDM spreadsheet and transform it
# to a MdmParamTypes enumeration.
sub fixupType {
    my $type = shift;

    if ($type eq "string") {
        return "MPT_STRING";
    } elsif ($type eq "int") {
        return "MPT_INTEGER";
    } elsif ($type eq "unsignedInt") {
        return "MPT_UNSIGNED_INTEGER";
    } elsif ($type eq "long")
    {
        return "MPT_LONG64";
    } elsif ($type eq "unsignedLong") {
        return "MPT_UNSIGNED_LONG64";
    } elsif ($type eq "boolean") {
        return "MPT_BOOLEAN";
    } elsif ($type eq "dateTime" || $type eq "DateTime") {
        return "MPT_DATE_TIME";
    } elsif ($type eq "base64") {
        return "MPT_BASE64";
    } elsif ($type eq "hexBinary") {
        return "MPT_HEX_BINARY";
    } elsif ($type eq "UUID") {
        return "MPT_UUID";
    } elsif ($type eq "IPAddress") {
        return "MPT_IP_ADDR";
    } elsif ($type eq "MACAddress") {
        return "MPT_MAC_ADDR";
    } elsif ($type eq "StatsCounter32") {
        return "MPT_STATS_COUNTER32";
    } elsif ($type eq "StatsCounter64") {
        return "MPT_STATS_COUNTER64";
    } else {
        die "Unrecognized param type $type\n";
    }
}

1;
