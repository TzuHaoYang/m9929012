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
# It stores data of TagEntryNode.
# It provides below functions:
# 1. Construct a new TagEntryNode object.
# 2. Format print proper xml containing the TagEntry information.
#

package TagEntryNode;
use strict;
use parent 'Exporter';
our @EXPORT = qw(add_param format_print);

sub new {
    my $class = shift;
    my $self = {
        obj_name => shift,
        params => []
    };
    bless $self, $class;
    return $self;
}

sub add_param {
    my ($self, $param_name, $nvram, $ntype, $vmapper) = @_;
    my $new_param = {
        name => $param_name,
        nvram => $nvram,
        ntype => $ntype,
        vmapper => $vmapper
    };
    push(@{$self->{params}}, $new_param);
}

sub param_tag_string {
    my ($self, $param_name) = @_;

    for my $p (@{$self->{params}}) {
        if ($p->{name} eq $param_name) {
            return tag_string($p);
        }
    }
    return "";
}

sub tag_string {
    my $p = shift;
    my $nvram_string = "";
    my $ntype_string = "";
    my $vmapper_string = "";

    if (defined $p->{name}) {
        $nvram_string = "nvram=\"" . $p->{nvram} . "\"" . " ";
    }

    if (defined $p->{ntype}) {
        $ntype_string = "ntype=\"" . $p->{ntype} . "\"" . " ";
    }

    if (defined $p->{vmapper}) {
        $vmapper_string = "vmapper=\"" . $p->{vmapper} . "\"" . " ";
    }

    return $nvram_string . $ntype_string . $vmapper_string;
}

sub format_print {
    my ($self, $fh) = @_;
    my $tag_string;

    print $fh sprintf("<object name=\"%s\" />\n", $self->{obj_name});

    for my $p (@{$self->{params}}) {
        $tag_string = tag_string($p);
        print $fh sprintf("  <parameter name=\"%s\" %s/>\n", $p->{name}, $tag_string);
    }
}

1;
