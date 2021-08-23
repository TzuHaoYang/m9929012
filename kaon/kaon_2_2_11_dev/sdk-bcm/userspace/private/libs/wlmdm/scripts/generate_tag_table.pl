#!/usr/bin/perl -w
#/***********************************************************************
# *
# *  Copyright (c) 2018  Broadcom
# *  All Rights Reserved
# *
#<:label-BRCM:2018:proprietary:standard
#
#  This program is the proprietary software of Broadcom and/or its
#  licensors, and may only be used, duplicated, modified or distributed pursuant
#  to the terms and conditions of a separate, written license agreement executed
#  between you and Broadcom (an "Authorized License").  Except as set forth in
#  an Authorized License, Broadcom grants no license (express or implied), right
#  to use, or waiver of any kind with respect to the Software, and Broadcom
#  expressly reserves all rights in and to the Software and all intellectual
#  property rights therein.  IF YOU HAVE NO AUTHORIZED LICENSE, THEN YOU HAVE
#  NO RIGHT TO USE THIS SOFTWARE IN ANY WAY, AND SHOULD IMMEDIATELY NOTIFY
#  BROADCOM AND DISCONTINUE ALL USE OF THE SOFTWARE.
#
#  Except as expressly set forth in the Authorized License,
#
#  1. This program, including its structure, sequence and organization,
#     constitutes the valuable trade secrets of Broadcom, and you shall use
#     all reasonable efforts to protect the confidentiality thereof, and to
#     use this information only in connection with your use of Broadcom
#     integrated circuit products.
#
#  2. TO THE MAXIMUM EXTENT PERMITTED BY LAW, THE SOFTWARE IS PROVIDED "AS IS"
#     AND WITH ALL FAULTS AND BROADCOM MAKES NO PROMISES, REPRESENTATIONS OR
#     WARRANTIES, EITHER EXPRESS, IMPLIED, STATUTORY, OR OTHERWISE, WITH
#     RESPECT TO THE SOFTWARE.  BROADCOM SPECIFICALLY DISCLAIMS ANY AND
#     ALL IMPLIED WARRANTIES OF TITLE, MERCHANTABILITY, NONINFRINGEMENT,
#     FITNESS FOR A PARTICULAR PURPOSE, LACK OF VIRUSES, ACCURACY OR
#     COMPLETENESS, QUIET ENJOYMENT, QUIET POSSESSION OR CORRESPONDENCE
#     TO DESCRIPTION. YOU ASSUME THE ENTIRE RISK ARISING OUT OF USE OR
#     PERFORMANCE OF THE SOFTWARE.
#
#  3. TO THE MAXIMUM EXTENT PERMITTED BY LAW, IN NO EVENT SHALL BROADCOM OR
#     ITS LICENSORS BE LIABLE FOR (i) CONSEQUENTIAL, INCIDENTAL, SPECIAL,
#     INDIRECT, OR EXEMPLARY DAMAGES WHATSOEVER ARISING OUT OF OR IN ANY
#     WAY RELATING TO YOUR USE OF OR INABILITY TO USE THE SOFTWARE EVEN
#     IF BROADCOM HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES;
#     OR (ii) ANY AMOUNT IN EXCESS OF THE AMOUNT ACTUALLY PAID FOR THE
#     SOFTWARE ITSELF OR U.S. $1, WHICHEVER IS GREATER. THESE LIMITATIONS
#     SHALL APPLY NOTWITHSTANDING ANY FAILURE OF ESSENTIAL PURPOSE OF ANY
#     LIMITED REMEDY.
# :>


#
# This script reads data model files for WiFi configurations.
# It then generates various .h and .c files that represent the mapping tables
# between wifi data model parameters and the nvram configurations in c.
#
# Usage: See the usage function at the bottom of this file.
#
#

use strict;
use FindBin qw($Bin);
use lib "$Bin";
use TagEntryNode;
use DmParser;


###########################################################################
#
# This first section contains helper functions.
#
###########################################################################


my $spreadsheet1Done = 0;
my $seqOid;


sub open_filehandle_for_output {
    my $filename = $_[0];
    my $overWriteFilename = ">" . $filename;
    local *FH;

    open (FH, $overWriteFilename) || die "Could not open $filename";
    return *FH;
}

sub open_filehandle_for_input {
    my $filename = $_[0];
    local *FH;

    open (FH, $filename) || die "Could not open $filename";
    return *FH;
}

sub create_tag_table {
    my $input_ref = shift;
    my $rowHash;
    my $oid;
    my $process_param = 1;
    my @tag_table;
    my %oidToNameHash;

    my $row = <$input_ref>;
    while ($row) {
        $rowHash = parse_row($row);
        if (!defined($rowHash->{type})) {
            next;
        }

        if ($rowHash->{type} eq "object") {
            if (defined($rowHash->{oid})) {
                $oid = $rowHash->{oid};
                $seqOid = $oid;
            } else {
                $oid = $seqOid;
            }
            $seqOid++;

            if (((!defined($rowHash->{supportLevel})) || ($rowHash->{supportLevel} ne "NotSupported")) &&
                (!defined($oidToNameHash{$oid}))) {
                $oidToNameHash{$oid} = $rowHash->{name};
                $process_param = 1;
            } else {
                $process_param = 0;
            }
        } elsif ($rowHash->{type} eq "parameter") {
            if (($process_param == 1) &&
                ((!defined($rowHash->{supportLevel})) || ($rowHash->{supportLevel} ne "NotSupported"))) {
                my $tag_entry = undef;
                if (defined($rowHash->{nvram})) {
                    for my $n (@tag_table) {
                        if ($n->{obj_name} eq $oidToNameHash{$oid}) {
                            $tag_entry = $n;
                            last;
                        }
                    }
                    if (!defined($tag_entry)) {
                        $tag_entry = new TagEntryNode($oidToNameHash{$oid});
                        push(@tag_table, $tag_entry);
                    }
                    my $param_name = $rowHash->{name};
                    my $nvram = $rowHash->{nvram};
                    my $ntype = $rowHash->{ntype};
                    my $vmapper = $rowHash->{vmapper};
                    $tag_entry->add_param($param_name, $nvram, $ntype, $vmapper);
                }
            }
        }
    } continue {
        $row = <$input_ref>;
    }

    return \@tag_table;
}

sub output_tag_table {
    my $output_ref = shift;
    my $tag_table = shift;

    print $output_ref "<?xml version=\"1.0\"?>\n";
    print $output_ref "<xmlMandatorySingleRootNode copyright=\"Broadcom Corporation, 2006\" >\n";
    for my $n (@{$tag_table}) {
        $n->format_print($output_ref);
        print $output_ref "\n\n";
    }
    print $output_ref "</xmlMandatorySingleRootNode>";
}

sub merge_tag_table {
    my $dm_ref = shift;
    my $tag_table = shift;
    my $output_ref = shift;
    my $rowHash;
    my $oid;
    my $process_param = 1;
    my $tag_entry;
    my %oidToNameHash;

    my $row = <$dm_ref>;
    while ($row) {
        $rowHash = parse_row($row);
        if (!defined($rowHash->{type})) {
            next;
        }

        if ($rowHash->{type} eq "object") {
            if (defined($rowHash->{oid})) {
                $oid = $rowHash->{oid};
                $seqOid = $oid;
            } else {
                $oid = $seqOid;
            }
            $seqOid++;

            $tag_entry = undef;
            if (($rowHash->{supportLevel} ne "NotSupported") &&
                (!defined($oidToNameHash{$oid}))) {
                $oidToNameHash{$oid} = $rowHash->{name};
                $process_param = 1;
                for my $n (@{$tag_table}) {
                    if ($n->{obj_name} eq $oidToNameHash{$oid}) {
                        $tag_entry = $n;
                        last;
                    }
                }
            } else {
                $process_param = 0;
            }
        } elsif ($rowHash->{type} eq "parameter") {
            if ($process_param == 1) {
                my $param_name = $rowHash->{name};
                if ($rowHash->{supportLevel} ne "NotSupported") {
                    if (defined($rowHash->{nvram})) {
                        die "The input data model file shouldn't have nvram tags!";
                    }
                    if (defined($tag_entry)) {
                        my $param_tag_string = $tag_entry->param_tag_string($param_name);
                        $row =~ /(.*)(\/>)([\s\S])/;
                        $row = $1 . $param_tag_string . $2 . $3;
                    }
                }
            }
        }
    } continue {
        print $output_ref $row;
        $row = <$dm_ref>;
    }
}

###########################################################################
#
# Begin of main
#
###########################################################################
sub usage {
    print "Usage:\n";
    print "generate_tag_table.pl gtag tagged_DMFILE target_dir\n";
    print "generate_tag_table.pl mtag DMFILE tag_table target_dir\n";
}


if (!defined($ARGV[0]) || !defined($ARGV[1])) {
    usage();
    die "aborted.";
}

if ($ARGV[0] eq "gtag") {
    my $tagged_datamodel = $ARGV[1];
    my $target_dir = $ARGV[2];
    my $input_ref;
    my $output_ref;

    $input_ref = open_filehandle_for_input("$tagged_datamodel");
    $output_ref = open_filehandle_for_output("$target_dir/tag_table.xml.tmp");

    my $tag_table = create_tag_table($input_ref);
    output_tag_table($output_ref, $tag_table);

    close $input_ref;
    close $output_ref;
} elsif ($ARGV[0] eq "mtag") {
    my $dm_file = $ARGV[1];
    my $tag_file = $ARGV[2];
    my $target_dir = $ARGV[3];
    my $dm_file_ref;
    my $tag_file_ref;
    my $tag_table;
    my $output_ref;

    $dm_file_ref = open_filehandle_for_input("$dm_file");
    $tag_file_ref = open_filehandle_for_input("$tag_file");
    $output_ref = open_filehandle_for_output("$target_dir/gen_tagged_datamodel.xml");

    $tag_table = create_tag_table($tag_file_ref);
    merge_tag_table($dm_file_ref, $tag_table, $output_ref);

    close $dm_file_ref;
    close $tag_file_ref;
    close $output_ref;
} else {
    usage();
    die "unrecognized command";
}

