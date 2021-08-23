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
use NvmMappingNode;
use DmParser;
use VmParser;


###########################################################################
#
# This first section contains helper functions.
#
###########################################################################

my $spreadsheet1Done = 0;
my %oidToNameHash;

my $name_mapping_table = "gen_wlmdm_mapping.c";
my $name_mapping_table_header = "gen_wlmdm_mapping.h";
my $value_mapping_table = "gen_value_mapping.c";
my $value_mapping_table_header = "gen_value_mapping.h";

#
# If no short object name was supplied, then
# just clean up the fully qualified (generic) object name
# and make that the MDMOBJID_name
#
sub convert_fullyQualifiedObjectName {
    my $name = $_[0];

#    print "Converting $name\n";

# get rid of trailing dot
    $name =~ s/\.$//;

# get rid of trailing instance id specifier
    $name =~ s/\.\{i\}$//;

# convert all dots ('.') or instance specifiers
# ('.{i}.') into underscores ('_')
    $name =~ s/\.\{i\}\./_/g;
    $name =~ s/\./_/g;

# convert all letters to upper case
    $name =~ tr/[a-z]/[A-Z]/;

# if there is a _INTERNETGATEWAYDEVICE_ in the middle
# of the object name, delete it.  Its redundant.
    $name =~ s/INTERNETGATEWAYDEVICE_//;

    return $name;
}



#
# Convert a short object name from the Data Model spreadsheet
# to the "all caps with underscore separator" form.
#
sub convert_shortObjectName {
    my $name = $_[0];
    my ($line, $nextWord, $restOfLine);

#
# Strip the trailing word "Object".  All of our abbreviated
# object names end with object.
#
    $name =~ s/Object$//;

#
# check for special case where the entire abbreviated name
# is all caps, e.g. IGD.  If so, just return that.
#
    if (!($name =~ /[a-z]+/)) {
        return $name;
    }

#
# Check for word that starts with multiple capitol letters,
# e.g. PPPLinkConfig
# The first ([A-Z]) should be (^[A-Z]) to force the match to the
# beginning of the shortObjectName.  Without the ^, we will match capital
# letters in the middle of shortObjectNames, such as VoiceLineCLIR.
# But fixing the pattern now would require changes to existing object
# names.  The workaround is to avoid multiple consecutive capital letters
# especially in the middle of the shortObjectName.
#

    $name =~ /([A-Z])([A-Z]+)([A-Z])([\w]*)/;

    if (defined($2)) {
#        print "Multi cap found, 3=$3 restOfLine=$4\n";
        $line = $1 . $2;
        $restOfLine = $3 . $4;
    } else
    {
        $line = "";
        $restOfLine = $name;
    }

#    print "After all caps processing, $line <-> $restOfLine\n";

    while (!($restOfLine =~ /^[A-Z][a-z0-9]*$/)) {
        $restOfLine =~ /([A-Z][a-z0-9]*)([A-Z][\w\d]*)/;
        $nextWord = $1;
        $restOfLine = $2;

#        print "nextWord=$nextWord restOfLine=$restOfLine\n";

        $nextWord =~ tr/[a-z]/[A-Z]/;
        if ($line eq "") {
            $line = $nextWord;
        } else {
            $line = $line . "_" . $nextWord;
        }

#        print "line is now $line\n";
#        print "restOfLine = $restOfLine\n";
    }


# this is the last word
#    print "last word $restOfLine\n";

    $restOfLine =~ tr/[a-z]/[A-Z]/;
    if ($line eq "") {
        $line = $restOfLine;
    } else
    {
        $line = $line . "_" . $restOfLine;
    }

    return $line;
}


sub convert_vmapper_name {
    my $vmapper = shift;
    my $vmapper_name;

    $vmapper_name = "nvm_value_mapper_".uc($vmapper);

    return $vmapper_name;
}

sub convert_type_name {
    my $type_name = shift;
    my $nvram_type;

    if ($type_name eq "boolean")
    {
        $nvram_type = "MPT_BOOLEAN";
    }
    elsif ($type_name eq "int")
    {
        $nvram_type = "MPT_INTEGER";
    }
    elsif ($type_name eq "string")
    {
        $nvram_type = "MPT_STRING";
    }
    elsif ($type_name eq "unsignedInt")
    {
        $nvram_type = "MPT_UNSIGNED_INTEGER";
    }
    elsif ($type_name eq "hexBinary")
    {
        $nvram_type = "MPT_HEX_BINARY";
    }
    else
    {
        $nvram_type = "MPT_STRING";
    }

    return $nvram_type;
}

#
# Open a filehandle and return a reference to it.
# This is useful when we want to output the MDM tree to multiple files.
# See Chapter 2, page 51, of Programming Perl
#
sub open_filehandle_for_output
{
    my $filename = $_[0];
    my $overWriteFilename = ">" . $filename;
    local *FH;

    open (FH, $overWriteFilename) || die "Could not open $filename";

    return *FH;
}

sub open_filehandle_for_input
{
    my $filename = $_[0];
    local *FH;

    open (FH, $filename) || die "Could not open $filename";

    return *FH;
}


sub output_wlmdm_value_mapper {
    my $input_ref = shift;
    my $output_ref = shift;
    my $output_h_ref = shift;
    my $rowHash;
    my $closePrevRow = 0;
    my $mapper_name;
    my $printParams = 0;

    print $output_h_ref "#ifndef __VALUE_MAPPER_H__\n";
    print $output_h_ref "#define __VALUE_MAPPER_H__\n\n";
    my $row = <$input_ref>;
    while ($row){
        $rowHash = parse_vm_row($row);

        if (!defined($rowHash->{type})) {
            next;
        }

        if ($rowHash->{type} =~ /mapper/i) {
            if ($rowHash->{shortObjectName} =~ /None/i) {
                $mapper_name = convert_fullyQualifiedObjectName($rowHash->{name});
            }
            else {
                $mapper_name = convert_shortObjectName($rowHash->{shortObjectName});
            }

            if ($closePrevRow) {
                print $output_ref "\t{\n\t\t{NULL, MPT_INTEGER},\n\t\t{NULL, MPT_INTEGER}\n\t}\n";
                print $output_ref "};\n\n";
            }

            if ($rowHash->{supportLevel} ne "NotSupported") {
                $printParams = 1;
                $closePrevRow = 1;
                print $output_ref "ValueMapper nvm_value_mapper_$mapper_name\[\] =\n";
                print $output_ref "{\n";
                print $output_h_ref "extern ValueMapper nvm_value_mapper_$mapper_name\[\];\n";
            } else {
                $printParams = 0;
                $closePrevRow = 0;
            }
        }
        else
        {
            if ($printParams == 1) {
                my $nvram;
                my $param_value;
                my $param_type;
                my $nvram_value;
                my $nvram_type;

                if (defined ($rowHash->{param})) {
                    $param_value = $rowHash->{param};
                    if ($param_value =~ /^".*"$/) {
                        $param_type = "MPT_STRING";
                    } else {
                        $param_value = "\"" . $param_value . "\"";
                        $param_type = "MPT_INTEGER";
                    }
                }

                if (defined ($rowHash->{nvram})) {
                    $nvram_value = $rowHash->{nvram};
                    if ($nvram_value =~ /^".*"$/) {
                        $nvram_type = "MPT_STRING";
                    } else {
                        $nvram_value = "\"" . $nvram_value . "\"";
                        $nvram_type = "MPT_INTEGER";
                    }
               }
               print $output_ref sprintf("\t{\n\t\t{%-10s, %s},\n\t\t{%-10s, %s}\n\t},\n", $param_value, $param_type, $nvram_value, $nvram_type);
            }
        }
    } continue {
        $row = <$input_ref>;
    }

    if ($closePrevRow) {
        print $output_ref "};\n";
        print $output_h_ref "\n#endif\n";
    }
}

sub create_wlmdm_mapping_table  {
    my $input_ref = shift;
    my $rowHash;
    my $closePrevRow = 0;
    my $printParams=1;
    my $oid;
    my $seqOid;
    my $objName;
    my $nvram_name;
    my $nvram_type = "MPT_STRING";
    my $param_name;
    my $param_type;
    my @nvm_param_mapping_table;

    my $row = <$input_ref>;
    while ($row) {
        $rowHash = parse_row($row);

        if (!defined($rowHash->{type})) {
            next;
        }

        if ($rowHash->{type} =~ /object/i)
        {
            if (defined($rowHash->{oid})) {
                # object has specified its own OID, so use it and
                # make subsequent objects sequential to this one.
                $oid = $rowHash->{oid};
                $seqOid = $oid;
            } else {
                $oid = $seqOid;
            }
            $seqOid++;

            if (($rowHash->{supportLevel} ne "NotSupported") &&
                (!defined($oidToNameHash{$oid}))) {
                $oidToNameHash{$oid} = $rowHash->{name};
                $printParams = 1;
                $closePrevRow = 1;
            } else {
                # the current object is not supported, don't print out any
                # of its params.
                $printParams = 0;
                $closePrevRow = 0;
            }
        } elsif ($rowHash->{type} eq "parameter") {
            #
            ## we are dealing with a parameter node which is under
            # the current object node.
            if ($printParams == 1) {
                my $nvram;
                my $vmapper_name = "NULL";

                $param_name = $rowHash->{name};
                $param_type = $rowHash->{paramType};
                if ($rowHash->{supportLevel} ne "NotSupported") {
                    if(defined($rowHash->{ntype})) {
                        $nvram_type = convert_type_name($rowHash->{ntype});
                    } else {
                        # We don't have ntype specified, so nvram has the same type as the parameter.
                        $nvram_type = convert_type_name($param_type);
                    }

                    if(defined($rowHash->{vmapper})) {
                        $vmapper_name = convert_vmapper_name($rowHash->{vmapper});
                    }

                    if (defined($rowHash->{nvram})) {
                        $nvram = $rowHash->{nvram};
                    }

                    if(defined($nvram)) {
                        my $nvm_param_mapping;
                        $nvm_param_mapping = new NvmMappingNode($nvram,
                                                                $nvram_type,
                                                                $oid,
                                                                $param_name,
                                                                $param_type,
                                                                $vmapper_name);
                        push (@nvm_param_mapping_table, $nvm_param_mapping);
                    }
                }
            }
        }
    } continue {
        $row = <$input_ref>;
    }

    return \@nvm_param_mapping_table;
}

sub output_wlmdm_mapping_table {
    my $nvm_param_mapping_table = shift;
    my $output_ref = shift;
    my $output_h_ref = shift;

    autogen_warning($output_ref);
    autogen_warning($output_h_ref);

    doxygen_header($output_ref, $name_mapping_table);

    print $output_ref "#include \"wlmdm.h\"\n";
    print $output_ref "#include \"$value_mapping_table_header\"\n\n";

    print $output_h_ref "#ifndef __WLMDM_MAPPING_H__\n";
    print $output_h_ref "#define __WLMDM_MAPPING_H__\n\n";
    print $output_h_ref "extern NvmParamMapping nvm_param_mapping_table_sorted[];\n";
    print $output_h_ref "extern NvmParamMapping nvm_param_mapping_table[];\n";
    print $output_h_ref "extern const unsigned int nvm_param_mapping_table_size;\n\n";
    print $output_h_ref "#endif\n";

    print $output_ref "NvmParamMapping nvm_param_mapping_table_sorted[] =\n";
    print $output_ref "{\n";

    my @nvm_param_mapping_table_sorted = quick_sort_by_nvname(@{$nvm_param_mapping_table});

    my $href;
    # Print out the sorted mapping table
    for $href (@nvm_param_mapping_table_sorted) {
        $href->format_print($output_ref);
    }
    print $output_ref "};\n\n";

    # Print out the mapping table
    print $output_ref "NvmParamMapping nvm_param_mapping_table[] =\n";
    print $output_ref "{\n";
    for $href (@{$nvm_param_mapping_table}) {
        $href->format_print($output_ref);
    }
    print $output_ref "};\n\n";

    print $output_ref "const unsigned int nvm_param_mapping_table_size = sizeof(nvm_param_mapping_table) / sizeof(NvmParamMapping);\n";
    print $output_ref "\n\n";
}

sub quick_sort_by_nvname {
    my @list = @_;

    if ( scalar @list <= 1 ) {
        return (@list);
    }

    my $pivot   = shift @list;

    my @less    = grep($_->{nv_name} lt $pivot->{nv_name}, @list);
    my @greater = grep($_->{nv_name} ge $pivot->{nv_name}, @list);

    return (quick_sort_by_nvname(@less), $pivot, quick_sort_by_nvname(@greater));
}

sub autogen_warning {
    my $fileRef = shift;

    print $fileRef "/*\n";
    print $fileRef " * This file is automatically generated from the data-model spreadsheet.\n";
    print $fileRef " * Do not modify this file directly - You will lose all your changes the\n";
    print $fileRef " * next time this file is generated!\n";
    print $fileRef " */\n\n\n";
}

sub doxygen_header {
    my $fileRef = shift;

    print $fileRef "/*!\\file $_[0]\n";
    print $fileRef " * \\brief Automatically generated header file $_[0]\n";
    print $fileRef " */\n\n\n";
}



###########################################################################
#
# Begin of main
#
###########################################################################

sub usage {
    print "Usage: generate_from_dm.pl <command> <DMFILE> <path_to_CommEngine_dir>\n";
    print "command is one of: nmap, vmap\n";
}


if (!defined($ARGV[0])) {
    usage();
    die "need cmd (arg0)";
}

if (!defined($ARGV[1])) {
    usage();
    die "need rootdir (arg1)";
}

if ($ARGV[0] eq "nmap") {
    my $wifi_datamodel = $ARGV[1];
    my $target_dir = $ARGV[2];
    my $input_ref;
    my $output_ref;
    my $output_h_ref;

    $input_ref = open_filehandle_for_input("$wifi_datamodel");
    $output_ref = open_filehandle_for_output("$target_dir/$name_mapping_table");
    $output_h_ref = open_filehandle_for_output("$target_dir/$name_mapping_table_header");

    my $nvm_param_mapping_table = create_wlmdm_mapping_table($input_ref);
    output_wlmdm_mapping_table($nvm_param_mapping_table, $output_ref, $output_h_ref);
    close $input_ref;
    close $output_ref;
    close $output_h_ref;
} elsif ($ARGV[0] eq "vmap") {
    my $schema = $ARGV[1];
    my $target_dir = $ARGV[2];
    my $schema_ref;
    my $output_ref;
    my $output_h_ref;

    $output_ref = open_filehandle_for_output("$target_dir/$value_mapping_table");
    $output_h_ref = open_filehandle_for_output("$target_dir/$value_mapping_table_header");

    autogen_warning($output_ref);
    autogen_warning($output_h_ref);

    doxygen_header($output_ref, $value_mapping_table);

    print $output_ref "#include \"wlmdm.h\"\n";
    print $output_ref "#include \"wifi_constants.h\"\n\n";

    $schema_ref = open_filehandle_for_input("$schema");
    output_wlmdm_value_mapper($schema_ref, $output_ref, $output_h_ref);

    close $schema_ref;
    print $output_ref "\n\n";
    close $output_ref;
} else {
    usage();
    die "unrecognized command";
}

