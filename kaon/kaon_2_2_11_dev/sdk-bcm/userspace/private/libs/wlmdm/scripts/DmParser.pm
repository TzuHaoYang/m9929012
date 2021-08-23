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
# A perl package to provide functions to parse a tagged data model file.
#

package DmParser;
use strict;
use parent 'Exporter';
our @EXPORT = qw(parse_row);
my $gWlOid = 0;

sub parse_object_line {
    my ($hashref, $line) = @_;

    $hashref->{type} = "object";

    $line =~ /name="([\w.\{\}]+)"/;
    $hashref->{name} = $1;

    $line =~ /supportLevel="([\w]+)"/;
    $hashref->{supportLevel} = $1;

    if ($line =~ /oid="([\d]+)"/) {
        $hashref->{oid} = $1;
        # Remember the OID that was specified for subsequent objects
        $gWlOid = $1;
    } else {
        # No OID was specified, so base it on previously specified OID
        $gWlOid++;
        $hashref->{oid} = $gWlOid;
    }
}

sub parse_parameter_line {
    my ($hashref, $line) = @_;

    $hashref->{type} = "parameter";

    $line =~ /name="([\w.\{\}\-_]+)"/;
    $hashref->{name} = $1;

    $line =~ /type="([\w]+)"/;
    $hashref->{paramType} = $1;

    $line =~ /supportLevel="([\w]+)"/;
    $hashref->{supportLevel} = $1;

    if ($line =~ /mapper="([\w._]+)"/) {
        $hashref->{mapper} = $1;
    }

    if ($line =~ /nvram="([\w._]+)"/) {
        $hashref->{nvram} = $1;
    }

    if ($line =~ /ntype="([\w._]+)"/) {
        $hashref->{ntype} = $1;
    }

    if ($line =~ /vmapper="([\w._]+)"/) {
        $hashref->{vmapper} = $1;
    }
}

sub parse_description_line {
    my ($hashref, $line) = @_;

    $hashref->{type} = "description";

    $line =~ /source="([\w]+)"/;
    $hashref->{source} = $1;

    $hashref->{desc} = $line;
}

#
# Parse a line in section 1 of the data model file.
# Fill in the given reference to a hash with attributes found in the line.
#
sub parse_row {
    my $row = shift;
    my $hashref = {};

    #
    # Call appropriate functions to parse object, parameter, and
    # description blocks.  This code assumes everything is on a single
    # line.  This assumption may break, especially for descriptions.
    #
    if ($row =~ /[\w]*<object/) {
        parse_object_line($hashref, $row);
    }

    if ($row =~ /[\w]*<parameter/) {
        parse_parameter_line($hashref, $row);
    }

    if ($row =~ /[\w]*<description/) {
        parse_description_line($hashref, $row);
    }

    return $hashref;
}

1;
