#!/usr/bin/perl

# use module
use strict;
use XML::Simple;
use Data::Dumper;
$Data::Dumper::Sortkeys  = 1;
use Getopt::Long qw(GetOptions);
#use Digest::SHA1::File qw( file_md5_hex );
use Digest::file qw(digest_file_hex);


my @metadatatypeneeded = (
                'FrequencyName',
                'Identifier',
                'ZWProductPage'
        );




my %errors = ();
my %warnings = ();

sub LogError {
if (CheckSuppression($_[0], $_[1])) {
	return;
}

my $errordetail;
$errordetail->{'file'} = $_[0];
$errordetail->{'code'} = $_[1];
$errordetail->{'description'} = $_[2];
#print Dumper($errordetail);
push(@{$errors{$_[0]}}, $errordetail);
}

sub LogWarning {
if (CheckSuppression($_[0], $_[1])) {
	return;
}

my $warningdetail;
$warningdetail->{'file'} = $_[0];
$warningdetail->{'code'} = $_[1];
$warningdetail->{'description'} = $_[2];
push(@{$warnings{$_[0]}}, $warningdetail);

}

# check common config file mistakes 
sub CheckConfig {

use strict;
use warnings;
my $file = $_[0];
my $count = 1;

# create object
my $xml = new XML::Simple;

# read XML file
my $data = $xml->XMLin($_[0], ForceArray => [ 'Group', 'MetaDataItem', 'Entry' ], KeyAttr => { CommandClass=>"id"});

# print output
#print Dumper($data->{CommandClass}->{133});

foreach my $rev ($data)
{
	#print $_[0]."-".Dumper($rev->{Revision});
	my $md5 = digest_file_hex($_[0], "SHA-512");
	if (defined($CFG::versiondb{$_[0]}))
	{
		if ($CFG::versiondb{$_[0]}{md5} ne $md5) 
		{
			my $dbr = int $CFG::versiondb{$_[0]}->{Revision};
			my $fr = int $rev->{Revision};
			if ($dbr >= $fr )
			{
				print $_[0]." - md5 does not match Database - Database Revision:";
				print $dbr." File Revision:".$fr;
				print "\n";
				LogError($_[0], 8, "Revision Number Was Not Bumped");	
			} else {
				my %versions;
				$versions{md5} = $md5;
				$versions{Revision} = $rev->{Revision};
				$CFG::versiondb{$_[0]} = \%versions;
				print($_[0]." - Updating Database\n");
			}			
		}
	} else { 
		my %versions;
		$versions{md5} = $md5;
		$versions{Revision} = $rev->{Revision};
		$CFG::versiondb{$_[0]} = \%versions;
		print($_[0]." - Adding new file to Database\n");
	}
}
#print Dumper($data->{CommandClass}->{133}->{Associations}->{Group});
foreach my $group ($data->{CommandClass}->{133}->{Associations}->{Group}) 
	{
		if (defined($group)) 
		{
			my $arrSize = @{$group};
			if ($arrSize != $data->{CommandClass}->{133}->{Associations}->{num_groups}) {
				LogError($_[0], 4, "Number of Groups does not equal Group Entries");
			}
			foreach my $entry (@{$group}) 
			{		
				if ((defined($entry->{auto}))
				 && ($entry->{index} == 1)
				  && (lc $entry->{auto} eq "true"))
				{
					LogError($_[0], 1,"Association Group 1 has auto equal to true");
				}
				if ((defined($entry->{auto}))
				&& ($entry->{index} != 1)
				&& (lc $entry->{auto} eq "false"))
				{
					LogError($_[0], 2, "Association Group $entry->{index} has auto set to False");
				}
			}
		} else {
			LogWarning($_[0], 3, "No Association Groups Defined for device");
		}
	}
foreach my $metadataitem ($data->{MetaData}) 
	{
		if (defined($metadataitem)) { 
			my $gotrev = 0;
			#Check if we have a ChangeLog Entry for this version 
			foreach my $changelog (@{$metadataitem->{ChangeLog}->{Entry}}) {
				if ($data->{Revision} == $changelog->{'revision'}) {
					$gotrev = 1;
				}
			}
			if ($gotrev == 0) {
				LogError($_[0], 9, "No Change Log Entry for this revision");
			}
			#now make sure required attributes have type/id entries
			my %params = map { $_ => 1 } @metadatatypeneeded;
			foreach my $mdi (@{$metadataitem->{MetaDataItem}}) {
				if (exists $params{$mdi->{name}}) {
					if (!defined($mdi->{type})) {
						LogError($_[0], 10, "Type Identifier Required for $mdi->{name}");
					}
					if (!defined($mdi->{id})) {
						LogError($_[0], 11, "ID Identifier Required for $mdi->{name}");
					}
				}
			}
			
		}
	}
     $data = $xml->XMLin($_[0], ForceArray => [ 'Value', 'MetaDataItem' ], KeyAttr => { CommandClass=>"id"});
     # print output
     foreach my $valueItem ($data->{CommandClass}->{112}->{Value}) {
         if (defined($valueItem)) {
             foreach my $configuration (@{$valueItem}) {
                if ((defined($configuration->{type})) && (lc $configuration->{type} eq "list") && (not defined($configuration->{size}))) {
                    LogError($_[0], 2, "Parameter: $configuration->{index} The size must be set for a list");
                }
                if ((defined($configuration->{type})) && (lc $configuration->{type} eq "bitset") && (not defined($configuration->{size}))) {
                    LogError($_[0], 2, "Parameter: $configuration->{index} The size must be set for a bitset");
                }
                if ((defined($configuration->{type})) && (lc $configuration->{type} eq "bitset") && (not defined($configuration->{bitmask}))) {
                    LogError($_[0], 2, "Parameter: $configuration->{index} The bitmask must be set for a bitset");
                }
                if ((defined($configuration->{type})) && (lc $configuration->{type} eq "byte") && (defined($configuration->{size}) && ($configuration->{size} != 1 ))) {
                    LogError($_[0], 2, "Parameter: $configuration->{index} The size is wrong for a byte");
                }
                if ((defined($configuration->{type})) && (lc $configuration->{type} eq "short") && (defined($configuration->{size}) && ($configuration->{size} != 2 ))) {
                    LogError($_[0], 2, "Parameter: $configuration->{index} The size is wrong for a short");
                }
                if ((defined($configuration->{type})) && (lc $configuration->{type} eq "int") && (defined($configuration->{size}) && ($configuration->{size} != 3 && $configuration->{size} != 4 ))) {
                    LogError($_[0], 2, "Parameter: $configuration->{index} The size is wrong for a int");
                }
             }
        }
    }
}

# check files match entries in manufacture_specific.xml 

# check common config file mistakes 
sub CheckMetaDataID {

	use strict;
	use warnings;
	my $file = $_[0];
	my $type = $_[1];
	my $id = $_[2];
	my $count = 1;
	
	# create object
	my $xml = new XML::Simple;
	
	# read XML file
	my $data = $xml->XMLin($_[0], ForceArray => [ 'Group', 'MetaDataItem', 'Entry' ], KeyAttr => { CommandClass=>"id"});

	foreach my $metadataitem ($data->{MetaData}) 
	{
		if (defined($metadataitem)) { 
			#now make sure required attributes have the right type/id entries
			foreach my $param (@metadatatypeneeded) {
				my $gottype = 0;
				my $gotid = 0;
				foreach my $mdi (@{$metadataitem->{MetaDataItem}}) {
					if ($mdi->{name} eq $param) {					
						if (!defined($mdi->{type})) {
							LogError($_[0], 10, "Type Identifier Required for $mdi->{name}");
						} else {
							if ($mdi->{type} eq $type) {
								$gottype = 1; 
							}
						}
					
						if (!defined($mdi->{id})) {
							LogError($_[0], 11, "ID Identifier Required for $mdi->{name}");
						} else {
							if ($mdi->{id} eq $id) {
								$gotid = 1;
							}
						}
					}
				}
				if ($gottype == 0) {
					LogWarning($_[0], 12, "No Matching Type Entry in Metadata $param for manufacturer_specific entry $type:$id");
				
				}
				if ($gotid == 0) {
					LogWarning($_[0], 12, "No Matching ID Entry in Metadata $param for manufacturer_specific entry $type:$id");
				}
			}
			
		}
	}


}


sub CheckFileExists {
my %configfiles = map { lc $_ => 1} @{$_[0]};
# create object
my $xml = new XML::Simple;

# read XML file
my $data = $xml->XMLin("config/manufacturer_specific.xml", KeyAttr => "", ForceArray => [ 'Product' ] );
# do a check of MFS Revision etc 
my $md5 = digest_file_hex("config/manufacturer_specific.xml", "SHA-512");
if (defined($CFG::versiondb{"config/manufacturer_specific.xml"}))
	{
	if ($CFG::versiondb{"config/manufacturer_specific.xml"}{md5} != $md5) 
		{
		my $dbr = $CFG::versiondb{"config/manufacturer_specific.xml"}->{Revision};
		my $fr = $data->{Revision};
		if ($dbr ge $fr )
		{
			print "config/manufacturer_specific.xml"." - md5 does not match Database - Database Revision:";
			print $CFG::versiondb{"config/manufacturer_specific.xml"}->{Revision}." File Revision:".int $data->{Revision};
			print "\n";
			LogError("config/manufacturer_specific.xml", 8, "Revision Number Was Not Bumped");	
		} else {
			my %versions;
			$versions{md5} = $md5;
			$versions{Revision} = $data->{Revision};
			$CFG::versiondb{"config/manufacturer_specific.xml"} = \%versions;
			print("config/manufacturer_specific.xml"." - Updating Database\n");
		}			
	}
} else { 
	my %versions;
	$versions{md5} = $md5;
	$versions{Revision} = $data->{Revision};
	$CFG::versiondb{"config/manufacturer_specific.xml"} = \%versions;
	print("config/manufacturer_specific.xml"." - Adding new file to Database\n");
}


foreach my $manu (@{$data->{Manufacturer}}) 
{
	if (defined($manu->{Product}))
	{
		foreach my $config (@{$manu->{Product}})
		{		
			if (defined($config->{config}))
			{
				#print Dumper($config->{config});
				if (!-e "config/$config->{config}") 
				{
					LogError("manufacturer_specific.xml", 5, "Config File config/$config->{config} Specified in manufacturer_specific.xml doesn't exist");
				} else {
					delete $configfiles{lc "config/$config->{config}"}; 
				}
				CheckMetaDataID("config/".$config->{config}, $config->{type}, $config->{id});
			}
		}
	}
}
#anything left in $configfiles hasn't been specified in manufacturer_specific.xml
#print Dumper(%configfiles);
foreach my $unreffile (keys %configfiles) 
{
	LogWarning("manufacturer_specific.xml", 7, "Unreferenced Config File $unreffile present on file system");
}
}

sub CheckLocalization {
my %configfiles = map { lc $_ => 1} @{$_[0]};
# create object
my $xml = new XML::Simple;
my $data = $xml->XMLin("config/Localization.xml", KeyAttr => "", ForceArray => [ 'Localization' ] );
# do a check of MFS Revision etc 
my $md5 = digest_file_hex("config/Localization.xml", "SHA-512");
if (defined($CFG::versiondb{"config/Localization.xml"}))
	{
	if ($CFG::versiondb{"config/Localization.xml"}{md5} != $md5) 
		{
		my $dbr = $CFG::versiondb{"config/Localization.xml"}->{Revision};
		my $fr = $data->{Revision};
		if ($dbr ge $fr )
		{
			print "config/Localization.xml"." - md5 does not match Database - Database Revision:";
			print $CFG::versiondb{"config/Localization.xml"}->{Revision}." File Revision:".int $data->{Revision};
			print "\n";
			LogError("config/Localization.xml", 8, "Revision Number Was Not Bumped");	
		} else {
			my %versions;
			$versions{md5} = $md5;
			$versions{Revision} = $data->{Revision};
			$CFG::versiondb{"config/Localization.xml"} = \%versions;
			print("config/Localization.xml"." - Updating Database\n");
		}			
	}
} else { 
	my %versions;
	$versions{md5} = $md5;
	$versions{Revision} = $data->{Revision};
	$CFG::versiondb{"config/Localization.xml"} = \%versions;
	print("config/Localization.xml"." - Adding new file to Database\n");
}
}

sub CheckNotificationCCTypes {
my %configfiles = map { lc $_ => 1} @{$_[0]};
# create object
my $xml = new XML::Simple;
my $data = $xml->XMLin("config/NotificationCCTypes.xml", KeyAttr => "", ForceArray => [ 'NotificationTypes' ] );
# do a check of MFS Revision etc 
my $md5 = digest_file_hex("config/NotificationCCTypes.xml", "SHA-512");
if (defined($CFG::versiondb{"config/NotificationCCTypes.xml"}))
	{
	if ($CFG::versiondb{"config/NotificationCCTypes.xml"}{md5} != $md5) 
		{
		my $dbr = $CFG::versiondb{"config/NotificationCCTypes.xml"}->{Revision};
		my $fr = $data->{Revision};
		if ($dbr ge $fr )
		{
			print "config/NotificationCCTypes.xml"." - md5 does not match Database - Database Revision:";
			print $CFG::versiondb{"config/NotificationCCTypes.xml"}->{Revision}." File Revision:".int $data->{Revision};
			print "\n";
			LogError("config/NotificationCCTypes.xml", 8, "Revision Number Was Not Bumped");	
		} else {
			my %versions;
			$versions{md5} = $md5;
			$versions{Revision} = $data->{Revision};
			$CFG::versiondb{"config/NotificationCCTypes.xml"} = \%versions;
			print("config/NotificationCCTypes.xml"." - Updating Database\n");
		}			
	}
} else { 
	my %versions;
	$versions{md5} = $md5;
	$versions{Revision} = $data->{Revision};
	$CFG::versiondb{"config/NotificationCCTypes.xml"} = \%versions;
	print("config/NotificationCCTypes.xml"." - Adding new file to Database\n");
}
}

sub CheckSensorMultiLevelCCTypes {
    my %configfiles = map { lc $_ => 1} @{$_[0]};
    # create object
    my $xml = new XML::Simple;
    my $data = $xml->XMLin("config/SensorMultiLevelCCTypes.xml", KeyAttr => "", ForceArray => [ 'SensorTypes' ] );
    # do a check of MFS Revision etc
    my $md5 = digest_file_hex("config/SensorMultiLevelCCTypes.xml", "SHA-512");
    if (defined($CFG::versiondb{"config/SensorMultiLevelCCTypes.xml"}))
    {
        if ($CFG::versiondb{"config/SensorMultiLevelCCTypes.xml"}{md5} != $md5)
        {
            my $dbr = $CFG::versiondb{"config/SensorMultiLevelCCTypes.xml"}->{Revision};
            my $fr = $data->{Revision};
            if ($dbr ge $fr )
            {
                print "config/SensorMultiLevelCCTypes.xml"." - md5 does not match Database - Database Revision:";
                print $CFG::versiondb{"config/SensorMultiLevelCCTypes.xml"}->{Revision}." File Revision:".int $data->{Revision};
                print "\n";
                LogError("config/SensorMultiLevelCCTypes.xml", 8, "Revision Number Was Not Bumped");
            } else {
                my %versions;
                $versions{md5} = $md5;
                $versions{Revision} = $data->{Revision};
                $CFG::versiondb{"config/SensorMultiLevelCCTypes.xml"} = \%versions;
                print("config/SensorMultiLevelCCTypes.xml"." - Updating Database\n");
            }
        }
    } else {
        my %versions;
        $versions{md5} = $md5;
        $versions{Revision} = $data->{Revision};
        $CFG::versiondb{"config/SensorMultiLevelCCTypes.xml"} = \%versions;
        print("config/SensorMultiLevelCCTypes.xml"." - Adding new file to Database\n");
    }
}


sub PrettyPrintErrors() {
	if (length(%errors) > 1) {
		print "\n\nErrors: (Please Correct before Submitting to OZW)\n";
		while ((my $key, my $value) = each %errors) 
		{
			foreach my $detail (@{$value}) 
			{
				print $key.": ".$detail->{description}." - Error Code $detail->{code}\n";
			}
			print "\n";
		}
	} else {
		print "\n\nNo errors detected (You can submit your changes to OZW)\n";
	}
}

sub PrettyPrintWarnings() {
	print "\n\nWarnings: (Can be Ignored)\n";
	while ((my $key, my $value) = each %warnings) 
	{
		foreach my $detail (@{$value}) 
		{
			print $key.": ".$detail->{description}." - Warning Code $detail->{code}\n";
		}
		print "\n";
	}
}

sub XMLPrintErrors() {
	my $numerrs  = 0;
	while ((my $key, my $value) = each %errors) 
	{
		foreach my $detail (@{$value}) 
		{
			$numerrs++;
		}
	}
	open(my $fh, '>', 'results/OZW_CheckConfig.xml') or die "Could not open file results\OZW_CheckConfig.xml $!";
	print $fh "<testsuite failures=\"0\" assertions=\"\" name=\"OZW_CheckConfig\" tests=\"1\" errors=\"$numerrs\" time=\"\">\n";
	while ((my $key, my $value) = each %errors) 
	{
		foreach my $detail (@{$value}) 
		{
			print $fh "\t<testcase assertions=\"1\" name=\"$detail->{code}-$detail->{file}\" time=\"\">\n";
			print $fh "\t\t<failure type=\"ScriptError\" message=\"$detail->{description}\"></failure>\n";
			print $fh "\t\t<system-out>\n";
			print $fh "\t\t<![CDATA[File: $detail->{file}\nDescription: $detail->{description}\nError Code: $detail->{code}]]>\n";
			print $fh "\t\t</system-out>\n";
			print $fh "\t</testcase>\n";
		}
	}
	print $fh "</testsuite>\n";
	close $fh;
}

sub XMLPrintWarnings() {
	my $numerrs  = 0;
	while ((my $key, my $value) = each %warnings) 
	{
		foreach my $detail (@{$value}) 
		{
			$numerrs++;
		}
	}
	open(my $fh, '>', 'results/OZW_CheckConfigWarnings.xml') or die "Could not open file results\OZW_CheckConfig.xml $!";
	print $fh "<testsuite failures=\"0\" assertions=\"\" name=\"OZW_CheckConfigWarnings\" tests=\"1\" errors=\"$numerrs\" time=\"\">\n";
	while ((my $key, my $value) = each %warnings) 
	{
		foreach my $detail (@{$value}) 
		{
			print $fh "\t<testcase assertions=\"1\" name=\"$detail->{code}-$detail->{file}\" time=\"\">\n";
			print $fh "\t\t<failure type=\"ScriptError\" message=\"$detail->{description}\"></failure>\n";
			print $fh "\t\t<system-out>\n";
			print $fh "\t\t<![CDATA[File: $detail->{file}\nDescription: $detail->{description}\nError Code: $detail->{code}]]>\n";
			print $fh "\t\t</system-out>\n";
			print $fh "\t</testcase>\n";
		}
	}
	print $fh "</testsuite>\n";
	close $fh;
}

# Read a configuration file
#   The arg can be a relative or full path, or
#   it can be a file located somewhere in @INC.
sub ReadCfg {
    my $file = "./cpp/build/testconfigsuppressions.cfg";
    our $err;

    {   # Put config data into a separate namespace
        package CFG;

        # Process the contents of the config file
        my $rc = do($file);

        # Check for errors
        if ($@) {
            $::err = "ERROR: Failure compiling '$file' - $@";
        } elsif (! defined($rc)) {
            $::err = "ERROR: Failure reading '$file' - $!";
        } elsif (! $rc) {
            $::err = "ERROR: Failure processing '$file'";
        }
    }

    return ($err);
}

# Read a configuration file
#   The arg can be a relative or full path, or
#   it can be a file located somewhere in @INC.
sub ReadVersions
{
    my $file = "./cpp/build/testconfigversions.cfg";

    our $err;

    {   # Put config data into a separate namespace
        package CFG;
        my %versiondb;

        # Process the contents of the config file
        my $rc = do($file);

        # Check for errors
        if ($@) {
            $::err = "ERROR: Failure compiling '$file' - $@";
        } elsif (! defined($rc)) {
            $::err = "ERROR: Failure reading '$file' - $!";
        } elsif (! $rc) {
            $::err = "ERROR: Failure processing '$file'";
        }
    }

    return ($err);
}



sub CheckSuppression 
{
	my $file = $_[0];
	my $code = $_[1];
	if (defined($CFG::CFG{$file}) && $CFG::CFG{$file}{'code'} == $code) {
		return 1
	}
	return;
}	
	


my $doxml;
my $printwarnings;

GetOptions(	"printwarnings" => \$printwarnings,
			"outputxml" => \$doxml
			) or die("Error in Command Line arguements\n");

if (my $err = ReadCfg()) {
    print(STDERR $err, "\n");
    exit(1);
}
if (my $err = ReadVersions()) {
    print(STDERR $err, "\n");
}


print "Checking Config Files... Please Wait\n";
my $dirname="config";
opendir(DIR, $dirname);
my @dirs = readdir(DIR);
closedir DIR;
my @filelist;



foreach my $key (@dirs)
{
 	next if ($key eq "."); 
  	next if ($key eq "..");
  	if(-d "$dirname/$key")
  	{
  		my @files = glob("$dirname/$key/*.xml");
   		foreach my $file (@files)
   		{
    		next if ($file eq "."); 
    		next if ($file eq "..");
			push(@filelist, $file);
    		#print "Checking $file\n";    
    		CheckConfig("$file");
   		}
  	}
} 
CheckFileExists(\@filelist);
CheckLocalization();
CheckNotificationCCTypes();
CheckSensorMultiLevelCCTypes();

if ($doxml == 0) { 
	PrettyPrintErrors();
}
if ($doxml == 0 && $printwarnings == 1) {
	PrettyPrintWarnings();
}
if ($doxml == 1) {
	XMLPrintErrors();
}
if ($doxml == 1 && $printwarnings == 1) {
	XMLPrintWarnings();
}

my $errorsize = keys %errors;
if ($errorsize == 0) 
{
	print "\nSaving Revision Database\n";
	open my $FH, '>', 'cpp/build/testconfigversions.cfg';
	print $FH Data::Dumper->Dump([\%CFG::versiondb], ['*versiondb']);
	close $FH;	
}
